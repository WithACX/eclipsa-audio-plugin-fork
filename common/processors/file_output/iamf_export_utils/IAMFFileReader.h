#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

#include <filesystem>

#include "iamf/include/iamf_tools/iamf_decoder_factory.h"
#include "iamf/include/iamf_tools/iamf_decoder_interface.h"

class IAMFFileReader {
 public:
  using Settings = iamf_tools::api::IamfDecoderFactory::Settings;

  // Data about the stream to be read from the IAMF file
  struct StreamData {
    std::vector<void*> mixes;
    int numChannels = 0;
    unsigned sampleRate = 0;
    unsigned frameSize = 0;
    char descriptorOBUs[4096] = {0};
    bool valid = false;
  };

  static const Settings kDefaultReaderSettings;

  IAMFFileReader(const std::filesystem::path& iamfFilePath,
                 const Settings& settings = kDefaultReaderSettings);
  ~IAMFFileReader() = default;

  StreamData getStreamData() const { return kStreamData_; }
  void* getCurrentMix() const;
  bool setCurrentMix(const void* mix);
  unsigned readFrame(juce::AudioBuffer<float>& buffer);
  unsigned readFrame(juce::AudioBuffer<double>& buffer);
  bool seekToFrame(unsigned frameIndex);

  // StreamData open(const std::string& filename);
  // bool close();
  // bool readFrame(juce::AudioBuffer<double>& buffer);

 private:
  StreamData getDescriptorOBUs(
      const std::unique_ptr<iamf_tools::api::IamfDecoderInterface>& decoder,
      std::filesystem::path filePath);
  IAMFFileReader::StreamData parseOBUs(
      const std::unique_ptr<iamf_tools::api::IamfDecoderInterface>& decoder,
      std::unique_ptr<std::ifstream>& fileStream);

  const StreamData kStreamData_;
  const std::filesystem::path kFilePath_;
  Settings settings_;
  std::unique_ptr<iamf_tools::api::IamfDecoderInterface> iamfDecoder_;
  std::unique_ptr<std::ifstream> fileStream_;
};
