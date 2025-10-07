#include "IAMFFileReader.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>

#include "iamf_tools_api_types.h"
#include "logger/logger.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"

// Parse descriptors to determine audio stream params for the selected mix
// presentation
static IAMFFileReader::StreamData parseOBUs(
    std::unique_ptr<IAMFFileReader::Decoder>& decoder,
    std::unique_ptr<std::ifstream>& fileStream) {
  const size_t kBufferSize = 4096;
  IAMFFileReader::StreamData streamData;
  std::unique_ptr<char[]> buffer = std::make_unique<char[]>(kBufferSize);

  while (fileStream->read(buffer.get(), kBufferSize) ||
         fileStream->gcount() > 0) {
    decoder->Decode(reinterpret_cast<const uint8_t*>(buffer.get()),
                    fileStream->gcount());

    if (decoder->IsDescriptorProcessingComplete()) {
      streamData.valid = true;
      decoder->GetNumberOfOutputChannels(streamData.numChannels);
      decoder->GetSampleRate(streamData.sampleRate);
      decoder->GetFrameSize(streamData.frameSize);
      // Requested playback layout may differ from actual output layout.
      iamf_tools::api::SelectedMix selectedMix;
      decoder->GetOutputMix(selectedMix);
      streamData.playbackLayout =
          Speakers::AudioElementSpeakerLayout(selectedMix.output_layout);
      return streamData;
    }
  }

  return streamData;
}

static IAMFFileReader::StreamData parseStreamData(
    std::unique_ptr<IAMFFileReader::Decoder>& decoder,
    std::unique_ptr<std::ifstream>& fileStream) {
  const IAMFFileReader::StreamData kStreamData = parseOBUs(decoder, fileStream);

  return kStreamData;
}

IAMFFileReader::IAMFFileReader(const std::filesystem::path& iamfFilePath)
    : IAMFFileReader(iamfFilePath, kDefaultReaderSettings) {}

IAMFFileReader::IAMFFileReader(const std::filesystem::path& iamfFilePath,
                               const Settings& settings)
    : kFilePath_(iamfFilePath), settings_(settings) {
  if (!std::filesystem::exists(iamfFilePath)) {
    throw std::runtime_error("IAMFFileReader: IAMF file does not exist");
  }

  // Create an initial decoder to parse Descriptor OBUs
  iamfDecoder_ = iamf_tools::api::IamfDecoderFactory::Create(settings_);
  if (!iamfDecoder_) {
    LOG_ERROR(0, "IAMFFileReader: Failed to create IAMF decoder");
    throw std::runtime_error("IAMFFileReader: Failed to create IAMF decoder");
  }

  fileStream_ = std::make_unique<std::ifstream>(kFilePath_, std::ios::binary);
  if (!fileStream_->is_open()) {
    LOG_ERROR(0, "IAMFFileReader: Failed to open IAMF file");
    throw std::runtime_error("IAMFFileReader: Failed to create IAMF decoder");
  }

  streamData_ = parseStreamData(iamfDecoder_, fileStream_);
  if (!streamData_.valid) {
    LOG_ERROR(0, "IAMFFileReader: Failed to parse IAMF file");
    throw std::runtime_error("IAMFFileReader: Failed to parse IAMF file");
  }

  // Build index of frame positions for future file seeking
  frameIdxs_ = buildFrameIndices(iamfDecoder_, fileStream_);
}

IAMFFileReader::~IAMFFileReader() {
  if (fileStream_ && fileStream_->is_open()) {
    fileStream_->close();
  }
}

bool IAMFFileReader::prepareTemporalUnit(std::unique_ptr<Decoder>& decoder) {
  // Development note: Try to avoid reallocations of this buffer
  const size_t kBufferSize = 4096;
  std::unique_ptr<char[]> bufferData = std::make_unique<char[]>(kBufferSize);

  while (!iamfDecoder_->IsTemporalUnitAvailable()) {
    if (fileStream_->read(bufferData.get(), kBufferSize) ||
        fileStream_->gcount() > 0) {
      iamfDecoder_->Decode(reinterpret_cast<const uint8_t*>(bufferData.get()),
                           fileStream_->gcount());
    } else {
      // End of file reached, signal decoder to flush remaining temporal units
      iamfDecoder_->SignalEndOfDecoding();
      if (!iamfDecoder_->IsTemporalUnitAvailable()) {
        return false;
      } else {
        return true;
      }
    }
  }
  return true;
}

void convertAndCopyChannelMajor(const int32_t* input,
                                juce::AudioBuffer<float>& output,
                                int numSamples, int numChannels) {
  constexpr float kScale = 1.0f / INT32_MAX;

  for (int channel = 0; channel < numChannels; ++channel) {
    float* out = output.getWritePointer(channel);

    // Stride through input to pick out only this channel
    const int32_t* kIn = input + channel;

    for (int sample = 0; sample < numSamples; ++sample) {
      out[sample] = static_cast<float>(*kIn) * kScale;
      kIn += numChannels;  // Jump to next sample for this channel
    }
  }
}

size_t IAMFFileReader::readFrame(juce::AudioBuffer<float>& buffer) {
  if (buffer.getNumChannels() != streamData_.numChannels ||
      buffer.getNumSamples() != streamData_.frameSize) {
    LOG_ERROR(0, "IAMFFileReader: Buffer size does not match stream data");
    return 0;
  }

  return parseFrame(&buffer);
}

size_t IAMFFileReader::parseFrame(juce::AudioBuffer<float>* buffer) {
  if (!prepareTemporalUnit(iamfDecoder_)) {
    return 0;
  }

  const size_t kPCMSampleBufferSize =
      streamData_.frameSize * streamData_.numChannels * sizeof(int32_t);
  std::unique_ptr<char[]> sampleBuffer = std::make_unique<char[]>(
      streamData_.frameSize * streamData_.numChannels * sizeof(int32_t));

  size_t bytesRead = 0;
  iamfDecoder_->GetOutputTemporalUnit(
      reinterpret_cast<uint8_t*>(sampleBuffer.get()), kPCMSampleBufferSize,
      bytesRead);
  size_t samplesRead = 0;
  if (bytesRead > 0) {
    // Samples are interleaved 32-bit ints to be parsed out
    ++streamData_.currentFrameIdx;
    const size_t kSampsTotal = bytesRead / sizeof(int32_t);
    const size_t kSampsPerCh = kSampsTotal / streamData_.numChannels;
    if (kSampsTotal / streamData_.numChannels != streamData_.frameSize) {
      LOG_INFO(0, "IAMFFileReader: Incomplete frame");
    }

    if (buffer) {
      for (int i = 0; i < streamData_.numChannels; ++i) {
        convertAndCopyChannelMajor(
            reinterpret_cast<int32_t*>(sampleBuffer.get()), *buffer,
            kSampsPerCh, streamData_.numChannels);
      }
    }
    samplesRead = kSampsPerCh;
  }
  return samplesRead;
}

/**
 * @brief To be called after parsing OBUs.
 * Populate frameIdxs_ with positions of each frame in the file.
 * Iterate through temporal units, recording the file position of each frame.
 * Reset file position and reparse OBUs to prepare for actual reading as
 * cleanup.
 *
 * @param decoder
 * @param fileStream
 * @return std::vector<IAMFFileReader::IdxEntry>
 */
std::vector<IAMFFileReader::IdxEntry> IAMFFileReader::buildFrameIndices(
    std::unique_ptr<Decoder>& decoder,
    std::unique_ptr<std::ifstream>& fileStream) {
  jassert(decoder->IsDescriptorProcessingComplete());

  std::vector<IdxEntry> frameIdxs;
  size_t frameCount = 0;
  std::streampos pos = fileStream->tellg();
  frameIdxs.push_back({pos});
  while (parseFrame()) {
    pos = fileStream->tellg();
    frameIdxs.push_back({pos});
    frameCount++;
  }
  streamData_.numFrames = frameCount;

  fileStream->clear();
  fileStream->seekg(0, std::ios::beg);
  decoder = iamf_tools::api::IamfDecoderFactory::Create(settings_);
  if (!decoder) {
    LOG_ERROR(0,
              "IAMFFileReader: Failed to recreate IAMF decoder after indexing");
    throw std::runtime_error(
        "IAMFFileReader: Failed to recreate IAMF decoder after indexing");
  }
  parseStreamData(decoder, fileStream);
  streamData_.currentFrameIdx = 0;
  return frameIdxs;
}

bool IAMFFileReader::seekFrame(const size_t frameIdx) {
  if (frameIdx >= frameIdxs_.size()) {
    LOG_WARNING(0, "IAMFFileReader: Frame index out of range");
    return false;
  }

  // If seeking backward, reset decoder and file position, then advance
  if (frameIdx < streamData_.currentFrameIdx) {
    // Reset file position
    fileStream_->clear();
    fileStream_->seekg(0, std::ios::beg);

    // Recreate decoder
    iamfDecoder_ = iamf_tools::api::IamfDecoderFactory::Create(settings_);
    if (!iamfDecoder_) {
      LOG_ERROR(0, "IAMFFileReader: Failed to recreate decoder during seek");
      return false;
    }

    // Reparse stream data
    const StreamData kStreamData = parseStreamData(iamfDecoder_, fileStream_);
    if (!kStreamData.valid) {
      LOG_ERROR(0, "IAMFFileReader: Failed to reparse stream data during seek");
      return false;
    }

    streamData_.currentFrameIdx = 0;
  }

  // Advance to the requested frame
  while (streamData_.currentFrameIdx < frameIdx) {
    if (parseFrame() == 0) {
      return false;
    }
  }

  return true;
}