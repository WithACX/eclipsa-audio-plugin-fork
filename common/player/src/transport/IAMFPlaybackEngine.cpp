#include "IAMFPlaybackEngine.h"

#include "logger/logger.h"
#include "processors/file_output/iamf_export_utils/IAMFFileReader.h"

IAMFPlaybackEngine::IAMFPlaybackEngine(const std::filesystem::path iamfPath)
    : decoderSource_(std::make_unique<IAMFDecoderSource>(iamfPath)),
      resampler_(nullptr) {
  // Attempt to configure playback based on the file's properties
  IAMFFileReader::StreamData streamData = decoderSource_->getStreamData();
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
    return decoderSource_->getStreamData();
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
  decoderSource_->pause();
  resampler_->flushBuffers();
  IAMFFileReader::StreamData streamData = decoderSource_->getStreamData();
  if (!decoderSource_->seek(position * streamData.numFrames)) {
    LOG_WARNING(0, "IAMFPlaybackEngine: Seek operation failed");
  }
  decoderSource_->play();
}

IAMFPlaybackEngine::~IAMFPlaybackEngine() {
  // Stop playback and clean up
  stop();
  sourcePlayer_.setSource(nullptr);
  deviceManager_.removeAudioCallback(&sourcePlayer_);
}