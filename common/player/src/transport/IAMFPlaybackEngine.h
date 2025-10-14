#pragma once

#include <juce_audio_devices/juce_audio_devices.h>

#include <filesystem>
#include <memory>

#include "data_repository/implementation/FilePlaybackRepository.h"
#include "player/src/transport/IAMFDecoderSource.h"
#include "processors/file_output/iamf_export_utils/IAMFFileReader.h"

class IAMFPlaybackEngine : public juce::ValueTree::Listener {
 public:
  IAMFPlaybackEngine(const std::filesystem::path iamfPath,
                     FilePlaybackRepository& filePlaybackRepo);
  ~IAMFPlaybackEngine();

  IAMFFileReader::StreamData getStreamData() const;
  void play();
  void pause();
  void stop();
  // Position is between 0.0 and 1.0
  void seek(const float position);
  void setVolume(const float volume);

 private:
  bool configureSourcePlayer(const unsigned sampleRate,
                             const unsigned frameSize,
                             const unsigned numChannels);
  bool configurePlaybackDevice(const unsigned sampleRate,
                               const unsigned frameSize,
                               const unsigned numChannels);
  void valueTreePropertyChanged(juce::ValueTree& tree,
                                const juce::Identifier& property) override;

  std::unique_ptr<IAMFDecoderSource> decoderSource_;
  std::unique_ptr<juce::ResamplingAudioSource> resampler_;
  juce::AudioSourcePlayer sourcePlayer_;
  juce::AudioDeviceManager deviceManager_;
  FilePlaybackRepository& fpbr_;
};