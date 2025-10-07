#include "IAMFPlaybackEngine.h"

#include <chrono>
#include <iostream>

#include "logger/logger.h"
#include "processors/file_output/iamf_export_utils/IAMFFileReader.h"

IAMFPlaybackEngine::IAMFPlaybackEngine(const std::filesystem::path iamfPath)
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
      resampler_->setResamplingRatio(sampleRate / kPlaybackSetup.sampleRate);
      LOG_INFO(0, "IAMFPlaybackEngine: Resampling IAMF " +
                      std::to_string(sampleRate) + " to " +
                      std::to_string(kPlaybackSetup.sampleRate));
    }
    // TODO: It's possible we didn't get the frame size we requested. What
    // happens here?
    if (kPlaybackSetup.bufferSize != frameSize) {
    }
  }
  configureSourcePlayer(sampleRate, frameSize, numChannels);

  // Other configuration for playback
  deviceManager_.addAudioCallback(&sourcePlayer_);
}

bool IAMFPlaybackEngine::configureSourcePlayer(const unsigned sampleRate,
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

bool IAMFPlaybackEngine::configurePlaybackDevice(const unsigned sampleRate,
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

IAMFFileReader::StreamData IAMFPlaybackEngine::getStreamData() const {
  if (decoderSource_) {
    return decoderSource_->getDecoder().getStreamData();
  }
  return {};
}

void IAMFPlaybackEngine::play() {
  if (decoderSource_) {
    decoderSource_->play();
  }
}

void IAMFPlaybackEngine::pause() {
  if (decoderSource_) {
    decoderSource_->pause();
  }
}

void IAMFPlaybackEngine::stop() {
  if (decoderSource_) {
    decoderSource_->stop();
  }
}

void IAMFPlaybackEngine::setVolume(const float volume) {
  sourcePlayer_.setGain(volume);
}

void IAMFPlaybackEngine::seek(const float position) {
  auto start_total = std::chrono::high_resolution_clock::now();
  std::cout << "=== SEEK BENCHMARK START ===" << std::endl;
  
  decoderSource_->pause();
  auto after_pause = std::chrono::high_resolution_clock::now();
  auto pause_duration = std::chrono::duration_cast<std::chrono::microseconds>(
      after_pause - start_total).count();
  std::cout << "Pause took: " << pause_duration << " μs" << std::endl;
  
  resampler_->flushBuffers();
  auto after_flush = std::chrono::high_resolution_clock::now();
  auto flush_duration = std::chrono::duration_cast<std::chrono::microseconds>(
      after_flush - after_pause).count();
  std::cout << "Flush buffers took: " << flush_duration << " μs" << std::endl;
  
  IAMFFileReader::StreamData streamData =
      decoderSource_->getDecoder().getStreamData();
  auto after_get_data = std::chrono::high_resolution_clock::now();
  auto get_data_duration = std::chrono::duration_cast<std::chrono::microseconds>(
      after_get_data - after_flush).count();
  std::cout << "Get stream data took: " << get_data_duration << " μs" << std::endl;
  
  if (!decoderSource_->seek(position * streamData.numFrames)) {
    LOG_WARNING(0, "IAMFPlaybackEngine: Seek operation failed");
  }
  auto after_seek = std::chrono::high_resolution_clock::now();
  auto seek_duration = std::chrono::duration_cast<std::chrono::microseconds>(
      after_seek - after_get_data).count();
  std::cout << "Decoder source seek took: " << seek_duration << " μs" << std::endl;
  
  decoderSource_->play();
  auto after_play = std::chrono::high_resolution_clock::now();
  auto play_duration = std::chrono::duration_cast<std::chrono::microseconds>(
      after_play - after_seek).count();
  std::cout << "Play resume took: " << play_duration << " μs" << std::endl;
  
  auto total_duration = std::chrono::duration_cast<std::chrono::microseconds>(
      after_play - start_total).count();
  std::cout << "TOTAL SEEK TIME: " << total_duration << " μs (" 
            << (total_duration / 1000.0) << " ms)" << std::endl;
  std::cout << "=== SEEK BENCHMARK END ===" << std::endl;
}

IAMFPlaybackEngine::~IAMFPlaybackEngine() {
  // Stop playback and clean up
  stop();
  sourcePlayer_.setSource(nullptr);
  deviceManager_.removeAudioCallback(&sourcePlayer_);
}