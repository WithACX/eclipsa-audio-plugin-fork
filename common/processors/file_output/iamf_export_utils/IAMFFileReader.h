#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

#include <filesystem>
#include <memory>

#include "iamf/include/iamf_tools/iamf_decoder_factory.h"
#include "iamf/include/iamf_tools/iamf_decoder_interface.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"

class IAMFFileReader {
 public:
  using Settings = iamf_tools::api::IamfDecoderFactory::Settings;
  using Decoder = iamf_tools::api::IamfDecoderInterface;

  // Data about the stream to be read from the IAMF file
  struct StreamData {
    int numChannels = 0;
    unsigned sampleRate = 0;
    unsigned frameSize = 0;
    Speakers::AudioElementSpeakerLayout playbackLayout = Speakers::kUnknown;
    bool valid = false;
  };

  static const Settings kDefaultReaderSettings;

  IAMFFileReader(const std::filesystem::path& iamfFilePath,
                 const Settings& settings = kDefaultReaderSettings);
  ~IAMFFileReader() = default;

  StreamData getStreamData() const { return streamData_; }
  size_t readFrame(juce::AudioBuffer<float>& buffer);
  size_t readFrame(juce::AudioBuffer<double>& buffer);
  bool seekToFrame(unsigned frameIndex);

 private:
  StreamData getStreamData(std::unique_ptr<Decoder>& decoder,
                           std::filesystem::path filePath);
  IAMFFileReader::StreamData parseOBUs(
      std::unique_ptr<Decoder>& decoder,
      std::unique_ptr<std::ifstream>& fileStream);
  bool prepareTemporalUnit(std::unique_ptr<Decoder>& decoder);

  const std::filesystem::path kFilePath_;
  Settings settings_;
  StreamData streamData_;
  std::unique_ptr<Decoder> iamfDecoder_;
  std::unique_ptr<std::ifstream> fileStream_;
};
