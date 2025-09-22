#include "IAMFFileReader.h"

#include <cstdint>
#include <fstream>
#include <memory>

IAMFFileReader::IAMFFileReader(
    const iamf_tools::api::IamfDecoderFactory::Settings& settings)
    : kSettings_(settings) {}

IAMFFileReader::~IAMFFileReader() { close(); }

// Parse Descriptor OBUs from the IAMF file to determine stream params
static IAMFFileReader::StreamData parseOBUs(
    const std::unique_ptr<iamf_tools::api::IamfDecoderInterface>& decoder,
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
      streamData.valid = true;
      // Debug
      std::cout << "Succeeded parse params" << std::endl;
      break;
    }
  }

  return streamData;
}

IAMFFileReader::StreamData IAMFFileReader::open(const std::string& filename) {
  StreamData streamData{.valid = false};
  // Existing decoder instance
  if (iamfDecoder_ != nullptr) {
    return streamData;
  }

  // Attempt to get a file stream handle
  std::unique_ptr<std::ifstream> fileStream_(
      new std::ifstream(filename, std::ios::binary));
  if (!fileStream_->is_open()) {
    // Debug
    std::cout << "Failed to open file: " << filename << std::endl;
    return streamData;
  }

  iamfDecoder_ = iamf_tools::api::IamfDecoderFactory::Create(kSettings_);
  if (!iamfDecoder_) {
    return streamData;
  }

  streamData = parseOBUs(iamfDecoder_, fileStream_);
  return streamData;
}

bool IAMFFileReader::close() {
  if (fileStream_) {
    fileStream_->close();
    fileStream_.reset();
  }
  if (iamfDecoder_) {
    iamfDecoder_.reset();
  }
  return true;
}

bool IAMFFileReader::readFrame(juce::AudioBuffer<double>& buffer) {
  if (!iamfDecoder_) {
    return false;
  }

  // Parse Descriptor OBUs to get stream information

  // Read and decode samples

  return true;
}