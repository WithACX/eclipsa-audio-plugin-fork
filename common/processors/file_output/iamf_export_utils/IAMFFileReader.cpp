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

const IAMFFileReader::Settings IAMFFileReader::kDefaultReaderSettings = {
    .requested_mix =
        {.output_layout =
             iamf_tools::api::OutputLayout::kItu2051_SoundSystemA_0_2_0},
    .requested_profile_versions =
        {iamf_tools::api::ProfileVersion::kIamfBaseEnhancedProfile},
    .requested_output_sample_type =
        iamf_tools::api::OutputSampleType::kInt32LittleEndian,
};

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

  streamData_ = getStreamData(iamfDecoder_, iamfFilePath);
  if (!streamData_.valid) {
    LOG_ERROR(0, "IAMFFileReader: Failed to parse IAMF file");
    throw std::runtime_error("IAMFFileReader: Failed to parse IAMF file");
  }
}

IAMFFileReader::StreamData IAMFFileReader::getStreamData(
    std::unique_ptr<Decoder>& decoder, std::filesystem::path filePath) {
  IAMFFileReader::StreamData streamData{.valid = false};

  fileStream_ = std::make_unique<std::ifstream>(filePath, std::ios::binary);

  if (!fileStream_->is_open()) {
    return streamData;
  }

  return parseOBUs(decoder, fileStream_);
}

// Parse descriptors to determine audio stream params for the selected mix
// presentation
IAMFFileReader::StreamData IAMFFileReader::parseOBUs(
    std::unique_ptr<Decoder>& decoder,
    std::unique_ptr<std::ifstream>& fileStream) {
  const size_t kBufferSize = 4096;
  IAMFFileReader::StreamData streamData;
  std::unique_ptr<char[]> buffer = std::make_unique<char[]>(kBufferSize);

  while (fileStream->read(buffer.get(), kBufferSize) ||
         fileStream->gcount() > 0) {
    decoder->Decode(reinterpret_cast<const uint8_t*>(buffer.get()),
                    fileStream->gcount());

    if (decoder->IsDescriptorProcessingComplete()) {
      decoder->GetNumberOfOutputChannels(streamData.numChannels);
      decoder->GetSampleRate(streamData.sampleRate);
      decoder->GetFrameSize(streamData.frameSize);
      // Requested playback layout may differ from actual output layout.
      iamf_tools::api::SelectedMix selectedMix;
      decoder->GetOutputMix(selectedMix);
      streamData.playbackLayout =
          Speakers::AudioElementSpeakerLayout(selectedMix.output_layout);
      streamData.valid = true;
      break;
    }
  }

  return streamData;
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
    const int32_t* in = input + channel;

    for (int sample = 0; sample < numSamples; ++sample) {
      out[sample] = static_cast<float>(*in) * kScale;
      in += numChannels;  // Jump to next sample for this channel
    }
  }
}

size_t IAMFFileReader::readFrame(juce::AudioBuffer<float>& buffer) {
  jassert(buffer.getNumChannels() == streamData_.numChannels);
  jassert(buffer.getNumSamples() == streamData_.frameSize);
  if (!streamData_.valid || !iamfDecoder_) {
    return 0;
  }

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
    const size_t kSampsTotal = bytesRead / sizeof(int32_t);
    const size_t kSampsPerCh = kSampsTotal / streamData_.numChannels;

    if (kSampsTotal / streamData_.numChannels != streamData_.frameSize) {
      // Do we do anything if we don't have a complete frame?
    }

    for (int i = 0; i < streamData_.numChannels; ++i) {
      convertAndCopyChannelMajor(reinterpret_cast<int32_t*>(sampleBuffer.get()),
                                 buffer, kSampsPerCh, streamData_.numChannels);
    }
    samplesRead = kSampsPerCh;
  }
  return samplesRead;
}