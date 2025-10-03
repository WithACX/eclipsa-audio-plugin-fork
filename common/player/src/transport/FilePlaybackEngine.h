#pragma once

#include <juce_audio_devices/juce_audio_devices.h>

#include <filesystem>

#include "logger/logger.h"
#include "player/src/transport/IAMFAudioSource.h"
#include "processors/file_output/iamf_export_utils/IAMFFileReader.h"

class FilePlaybackEngine {
 public:
  FilePlaybackEngine(const std::filesystem::path iamfPath) : source_(iamfPath) {
    // Attempt to configure playback based on the file's properties
    streamData_ = source_.getStreamData();
    auto frameSize = streamData_.frameSize;
    auto numChannels = streamData_.numChannels;
    auto sampleRate = streamData_.sampleRate;

    configureSourcePlayer(sampleRate, frameSize, numChannels);
    configurePlaybackDevice(sampleRate, frameSize, numChannels);

    // Other configuration for playback
    deviceManager_.addAudioCallback(&sourcePlayer_);
    source_.play();
  }

  bool configureSourcePlayer(const unsigned sampleRate,
                             const unsigned frameSize,
                             const unsigned numChannels) {
    sourcePlayer_.setSource(&source_);
    sourcePlayer_.prepareToPlay(sampleRate, frameSize);
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

  ~FilePlaybackEngine() {
    // Stop playback and clean up
    source_.stop();
    source_.releaseResources();
    sourcePlayer_.setSource(nullptr);
    deviceManager_.removeAudioCallback(&sourcePlayer_);
  }

 private:
  IAMFAudioSource source_;
  IAMFFileReader::StreamData streamData_;
  juce::AudioSourcePlayer sourcePlayer_;
  juce::AudioDeviceManager deviceManager_;
};