#pragma once

#include <juce_audio_devices/juce_audio_devices.h>

#include <filesystem>

#include "logger/logger.h"
#include "player/src/transport/IAMFTransport2.h"
#include "processors/file_output/iamf_export_utils/IAMFFileReader.h"

class FilePlaybackEngine {
 public:
  FilePlaybackEngine(const std::filesystem::path iamfPath) : source_(iamfPath) {
    streamData_ = source_.getStreamData();
    const auto kFrameSize = streamData_.frameSize;
    const auto kNumChannels = streamData_.numChannels;
    const auto kSampleRate = streamData_.sampleRate;

    configureSourcePlayer(kSampleRate, kFrameSize, kNumChannels);
    configurePlaybackDevice(kSampleRate, kFrameSize, kNumChannels);

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
    setup.outputChannels =
        3;  // Todo: Take the layout and convert to juce chset.
    const auto err = deviceManager_.setAudioDeviceSetup(setup, true);

    // Validate settings updated.
    setup = deviceManager_.getAudioDeviceSetup();
    if (err.isNotEmpty() || setup.sampleRate != sampleRate ||
        setup.bufferSize != frameSize
        // setup.outputChannels.toInteger() != numChannels // TODO: Fix this
        // check
    ) {
      LOG_WARNING(0,
                  "FilePlaybackEngine: Failed to configure playback device as "
                  "requested. Audio source may not play as expected.");
    }
  }

  ~FilePlaybackEngine() {
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