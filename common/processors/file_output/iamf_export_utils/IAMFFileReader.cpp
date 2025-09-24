#include "IAMFFileReader.h"

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>

#include "iamf_tools_api_types.h"

IAMFFileReader::IAMFFileReader(const std::filesystem::path& iamfFilePath,
                               const Settings& settings)
    : kStreamData_(), kFilePath_(iamfFilePath), settings_(settings) {
  if (!std::filesystem::exists(iamfFilePath)) {
    throw std::runtime_error("IAMFFileReader: IAMF file does not exist");
  }

  // Create an initial decoder to parse Descriptor OBUs
  // Once Descriptor OBUs are parsed, a more dynamic decoder can be created
  iamfDecoder_ = iamf_tools::api::IamfDecoderFactory::Create(settings_);

  // Parse and save descriptor OBUs
  const StreamData kStreamData = getDescriptorOBUs(iamfDecoder_, kFilePath_);

  // Debug
  iamf_tools::api::RequestedMix requested_mix = {
      .output_layout =
          iamf_tools::api::OutputLayout::kItu2051_SoundSystemA_0_2_0};
  iamf_tools::api::SelectedMix selected_mix;
  const auto result =
      iamfDecoder_->ResetWithNewMix(requested_mix, selected_mix);
  if (!result.ok()) {
    throw std::runtime_error("IAMFFileReader: Failed to set requested mix");
  }

  // // Recreate decoder
  // iamfDecoder_ = iamf_tools::api::IamfDecoderFactory::CreateFromDescriptors(
  //     settings_, reinterpret_cast<const
  //     uint8_t*>(kStreamData.descriptorOBUs),
  //     sizeof(kStreamData.descriptorOBUs));

  // if (!iamfDecoder_) {
  //   throw std::runtime_error("IAMFFileReader: Failed to create IAMF
  //   decoder");
  // }
}

IAMFFileReader::StreamData IAMFFileReader::getDescriptorOBUs(
    const std::unique_ptr<iamf_tools::api::IamfDecoderInterface>& decoder,
    std::filesystem::path filePath) {
  IAMFFileReader::StreamData streamData{.valid = false};

  std::unique_ptr<std::ifstream> fileStream_(
      new std::ifstream(filePath, std::ios::binary));

  if (!fileStream_->is_open()) {
    return streamData;
  }

  return parseOBUs(decoder, fileStream_);
}

// Parse Descriptor OBUs from the IAMF file to determine stream params
IAMFFileReader::StreamData IAMFFileReader::parseOBUs(
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
      memcpy(streamData.descriptorOBUs, buffer.get(), kBufferSize);
      streamData.valid = true;
      break;
    }
  }

  return streamData;
}

// IAMFFileReader::StreamData IAMFFileReader::open(const std::string& filename)
// {
//   StreamData streamData{.valid = false};
//   // Existing decoder instance
//   if (iamfDecoder_ != nullptr) {
//     return streamData;
//   }

//   // Attempt to get a file stream handle

//   iamfDecoder_ = iamf_tools::api::IamfDecoderFactory::Create(kSettings_);
//   if (!iamfDecoder_) {
//     return streamData;
//   }

//   streamData = parseOBUs(iamfDecoder_, fileStream_);
//   return streamData;
// }

// bool IAMFFileReader::close() {
//   if (fileStream_) {
//     fileStream_->close();
//     fileStream_.reset();
//   }
//   if (iamfDecoder_) {
//     iamfDecoder_.reset();
//   }
//   return true;
// }

// bool IAMFFileReader::readFrame(juce::AudioBuffer<double>& buffer) {
//   if (!iamfDecoder_) {
//     return false;
//   }

//   // Parse Descriptor OBUs to get stream information

//   // Read and decode samples

//   return true;
// }

const IAMFFileReader::Settings IAMFFileReader::kDefaultReaderSettings = {
    .requested_mix =
        {.output_layout =
             iamf_tools::api::OutputLayout::kItu2051_SoundSystemA_0_2_0},
    .requested_profile_versions =
        {iamf_tools::api::ProfileVersion::kIamfBaseEnhancedProfile},
    .requested_output_sample_type =
        iamf_tools::api::OutputSampleType::kInt32LittleEndian,
};