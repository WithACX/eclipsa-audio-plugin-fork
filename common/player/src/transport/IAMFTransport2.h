#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <cstddef>
#include <memory>

#include "processors/file_output/iamf_export_utils/IAMFFileReader.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"

class IAMFAudioSource : public juce::AudioSource {
 public:
  enum PlaybackState {
    kPlay,
    kPause,
    kStop,
  };

  IAMFAudioSource(const std::filesystem::path& iamfFilePath);
  ~IAMFAudioSource() = default;

  void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
  void releaseResources() override;
  void getNextAudioBlock(
      const juce::AudioSourceChannelInfo& bufferToFill) override;

  IAMFFileReader::StreamData getStreamData() const {
    if (reader_) return reader_->getStreamData();
    return {};
  }
  void setPlaybackLayout(const Speakers::AudioElementSpeakerLayout layout);

  PlaybackState getPlaybackState() const { return state_; }
  void play() { state_ = kPlay; }
  void pause() { state_ = kPause; }
  void stop();

  size_t getTrackPosition() const { return currentFrameIdx_; }
  void setTrackPosition(const size_t frameIdx);

 private:
  const std::filesystem::path kFilePath_;
  PlaybackState state_ = kStop;
  // Layout we attempt to decode from the IAMF file
  Speakers::AudioElementSpeakerLayout decodeLayout_ = Speakers::kStereo;
  size_t currentFrameIdx_ = 0, totalFrames_ = 0;
  std::unique_ptr<IAMFFileReader> reader_;
};