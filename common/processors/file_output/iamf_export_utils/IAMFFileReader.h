#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

#include "iamf/include/iamf_tools/iamf_decoder_factory.h"
#include "iamf/include/iamf_tools/iamf_decoder_interface.h"

class IAMFFileReader {
 public:
  using Settings = iamf_tools::api::IamfDecoderFactory::Settings;

  // Data about the stream to be read from the IAMF file
  struct StreamData {
    int numChannels = 0;
    unsigned sampleRate = 0;
    unsigned frameSize = 0;
    bool valid = false;
  };

  IAMFFileReader(const iamf_tools::api::IamfDecoderFactory::Settings& settings);
  ~IAMFFileReader();

  StreamData open(const std::string& filename);
  bool close();
  bool readFrame(juce::AudioBuffer<double>& buffer);

 private:
  const Settings kSettings_;
  std::unique_ptr<iamf_tools::api::IamfDecoderInterface> iamfDecoder_;
  std::unique_ptr<std::ifstream> fileStream_;
};
