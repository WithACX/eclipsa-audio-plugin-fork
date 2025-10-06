#pragma once

#include <juce_audio_devices/juce_audio_devices.h>

#include <filesystem>
#include <memory>

#include "logger/logger.h"
#include "player/src/transport/IAMFDecoderSource.h"
#include "processors/file_output/iamf_export_utils/IAMFFileReader.h"

class IAMFPlaybackEngine {
 public:
  IAMFPlaybackEngine(const std::filesystem::path iamfPath)
      : decoderSource_(std::make_unique<IAMFDecoderSource>(iamfPath)),
        resampler_(nullptr) {
    // Attempt to configure playback based on the file's properties
    IAMFFileReader::StreamData streamData =
        decoderSource_->getDecoder().getStreamData();
    auto frameSize = streamData.frameSize;
    auto numChannels = streamData.numChannels;
    auto sampleRate = streamData.sampleRate;

    if (!configurePlaybackDevice(sampleRate, frameSize, numChannels)) {
      const juce::AudioDeviceManager::AudioDeviceSetup kPlaybackSetup =
          deviceManager_.getAudioDeviceSetup();
      if (kPlaybackSetup.sampleRate != sampleRate) {
        resampler_ = std::make_unique<juce::ResamplingAudioSource>(
            decoderSource_.get(), false,
            kPlaybackSetup.outputChannels.countNumberOfSetBits());
        resampler_->setResamplingRatio(kPlaybackSetup.sampleRate / sampleRate);
      }
      // TODO: It's possible we didn't get the frame size we requested. What
      // happens here?
      if (kPlaybackSetup.bufferSize != frameSize) {
      }
    }
    configureSourcePlayer(sampleRate, frameSize, numChannels);

    // Other configuration for playback
    deviceManager_.addAudioCallback(&sourcePlayer_);
    // source_.play();
  }

  bool configureSourcePlayer(const unsigned sampleRate,
                             const unsigned frameSize,
                             const unsigned numChannels) {
    juce::AudioSource* source;
    if (resampler_) {
      source = resampler_.get();
    } else {
      source = decoderSource_.get();
    }
    sourcePlayer_.setSource(source);
    sourcePlayer_.prepareToPlay(sampleRate, frameSize);
    return true;
  }

  bool configurePlaybackDevice(const unsigned sampleRate,
                               const unsigned frameSize,
                               const unsigned numChannels) {
    deviceManager_.initialiseWithDefaultDevices(0, numChannels);
    auto setup = deviceManager_.getAudioDeviceSetup();
    setup.sampleRate = sampleRate;
    setup.bufferSize = frameSize;
    setup.useDefaultOutputChannels = true;
    const auto err = deviceManager_.setAudioDeviceSetup(setup, true);

    // Validate settings updated.
    setup = deviceManager_.getAudioDeviceSetup();
    if (setup.sampleRate != sampleRate || setup.bufferSize != frameSize ||
        setup.outputChannels.countNumberOfSetBits() != numChannels) {
      LOG_WARNING(0,
                  "FilePlaybackEngine: Failed to configure playback device as "
                  "requested. Audio source may not play as expected.");
    }
  }

  ~IAMFPlaybackEngine() {
    // Stop playback and clean up
    // source_.stop();
    // source_.releaseResources();
    sourcePlayer_.setSource(nullptr);
    deviceManager_.removeAudioCallback(&sourcePlayer_);
  }

 private:
  std::unique_ptr<IAMFDecoderSource> decoderSource_;
  std::unique_ptr<juce::ResamplingAudioSource> resampler_;
  juce::AudioSourcePlayer sourcePlayer_;
  juce::AudioDeviceManager deviceManager_;
};