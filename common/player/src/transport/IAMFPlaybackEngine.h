#pragma once

#include <juce_audio_devices/juce_audio_devices.h>

#include <filesystem>
#include <memory>

#include "player/src/transport/IAMFDecoderSource.h"
#include "processors/file_output/iamf_export_utils/IAMFFileReader.h"

class IAMFPlaybackEngine {
 public:
  IAMFPlaybackEngine(const std::filesystem::path iamfPath);
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

  std::unique_ptr<IAMFDecoderSource> decoderSource_;
  std::unique_ptr<juce::ResamplingAudioSource> resampler_;
  juce::AudioSourcePlayer sourcePlayer_;
  juce::AudioDeviceManager deviceManager_;
};