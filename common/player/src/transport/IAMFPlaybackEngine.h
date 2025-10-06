#pragma once

#include <juce_audio_devices/juce_audio_devices.h>

#include <filesystem>
#include <memory>

#include "player/src/transport/IAMFDecoderSource.h"

class IAMFPlaybackEngine {
 public:
  IAMFPlaybackEngine(const std::filesystem::path iamfPath);
  ~IAMFPlaybackEngine();

  bool configureSourcePlayer(const unsigned sampleRate,
                             const unsigned frameSize,
                             const unsigned numChannels);
  bool configurePlaybackDevice(const unsigned sampleRate,
                               const unsigned frameSize,
                               const unsigned numChannels);

  void play();
  void pause();
  void stop();

 private:
  std::unique_ptr<IAMFDecoderSource> decoderSource_;
  std::unique_ptr<juce::ResamplingAudioSource> resampler_;
  juce::AudioSourcePlayer sourcePlayer_;
  juce::AudioDeviceManager deviceManager_;
};