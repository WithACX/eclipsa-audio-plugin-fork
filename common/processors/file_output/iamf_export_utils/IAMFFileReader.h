#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

#include <filesystem>
#include <iosfwd>
#include <memory>

#include "iamf/include/iamf_tools/iamf_decoder_factory.h"
#include "iamf/include/iamf_tools/iamf_decoder_interface.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"

class IAMFFileReader {
 public:
  using Settings = iamf_tools::api::IamfDecoderFactory::Settings;
  using Decoder = iamf_tools::api::IamfDecoderInterface;

  struct StreamData {
    int numChannels = 0;
    unsigned sampleRate = 0, frameSize = 0;
    size_t durationSecs = 0;
    Speakers::AudioElementSpeakerLayout playbackLayout = Speakers::kUnknown;
    bool valid = false;
  };

  static const inline IAMFFileReader::Settings kDefaultReaderSettings = {
      .requested_mix =
          {.output_layout =
               iamf_tools::api::OutputLayout::kItu2051_SoundSystemA_0_2_0},
      .requested_profile_versions =
          {iamf_tools::api::ProfileVersion::kIamfBaseEnhancedProfile},
      .requested_output_sample_type =
          iamf_tools::api::OutputSampleType::kInt32LittleEndian,
  };

  IAMFFileReader(const std::filesystem::path& iamfFilePath);
  IAMFFileReader(const std::filesystem::path& iamfFilePath,
                 const Settings& settings);
  ~IAMFFileReader();

  // Delete copy constructor and assignment
  IAMFFileReader(const IAMFFileReader&) = delete;
  IAMFFileReader& operator=(const IAMFFileReader&) = delete;

  // Enable move constructor and assignment
  IAMFFileReader(IAMFFileReader&&) noexcept = default;
  IAMFFileReader& operator=(IAMFFileReader&&) noexcept = default;

  StreamData getStreamData() const { return streamData_; }
  size_t readFrame(juce::AudioBuffer<float>& buffer);
  size_t readFrame(juce::AudioBuffer<double>& buffer);
  bool seekFrame(const size_t frameIdx);

 private:
  struct IdxEntry {
    std::streampos filePos;
  };

  bool prepareTemporalUnit(std::unique_ptr<Decoder>& decoder);
  size_t parseFrame(juce::AudioBuffer<float>* buffer = nullptr);
  std::vector<IdxEntry> buildFrameIndices(
      std::unique_ptr<Decoder>& decoder,
      std::unique_ptr<std::ifstream>& fileStream);

  size_t currentFrameIdx_ = 0;

  const std::filesystem::path kFilePath_;
  Settings settings_;
  StreamData streamData_;
  std::unique_ptr<Decoder> iamfDecoder_;
  std::unique_ptr<std::ifstream> fileStream_;
  std::vector<IdxEntry> frameIdxs_;
};