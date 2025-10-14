#include "IAMFPlaybackEngine.h"

#include "data_structures/src/FilePlayback.h"
#include "logger/logger.h"
#include "processors/file_output/iamf_export_utils/IAMFFileReader.h"

IAMFPlaybackEngine::IAMFPlaybackEngine(const std::filesystem::path iamfPath,
                                       FilePlaybackRepository& filePlaybackRepo)
    : decoderSource_(std::make_unique<IAMFDecoderSource>(iamfPath)),
      resampler_(nullptr),
      fpbr_(filePlaybackRepo) {
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
    // It's possible we didn't get the frame size we requested. This is fine as
    // we can request an arbitrary number of samples from the audio source
    if (kPlaybackSetup.bufferSize != frameSize) {
    }
  }
  configureSourcePlayer(sampleRate, frameSize, numChannels);

  // Other configuration for playback
  deviceManager_.addAudioCallback(&sourcePlayer_);
  fpbr_.registerListener(this);
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
  // sourcePlayer_.prepareToPlay(sampleRate, frameSize);
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

void IAMFPlaybackEngine::play() { decoderSource_->play(); }

void IAMFPlaybackEngine::pause() { decoderSource_->pause(); }

void IAMFPlaybackEngine::stop() { decoderSource_->stop(); }

void IAMFPlaybackEngine::setVolume(const float volume) {
  sourcePlayer_.setGain(volume);
}

void IAMFPlaybackEngine::seek(const float position) {
  jassert(position >= 0.0f && position <= 1.0f);

  decoderSource_->pause();
  if (resampler_) {
    resampler_->flushBuffers();
  }
  IAMFFileReader::StreamData streamData = decoderSource_->getStreamData();
  if (!decoderSource_->seek(position * streamData.numFrames)) {
    LOG_WARNING(0, "IAMFPlaybackEngine: Seek operation failed");
  }
  decoderSource_->play();
}

void IAMFPlaybackEngine::valueTreePropertyChanged(
    juce::ValueTree& tree, const juce::Identifier& property) {
  const FilePlayback fpb = fpbr_.get();
  if (property == FilePlayback::kPlayState) {
    switch (fpb.getPlayState()) {
      case FilePlayback::kPlay:
        play();
        break;
      case FilePlayback::kPause:
        pause();
        break;
      case FilePlayback::kStop:
        stop();
        break;
      default:
        stop();
    }
  } else if (property == FilePlayback::kVolume) {
    setVolume(fpb.getVolume());
  }
  // File to playback could change
  else if (false) {
  }
  // Requested playback layout could change
  else if (false) {
  } else if (property == FilePlayback::kSeekPosition) {
    seek(fpb.getSeekPosition());
  }
}

IAMFPlaybackEngine::~IAMFPlaybackEngine() {
  stop();
  sourcePlayer_.setSource(nullptr);
  deviceManager_.removeAudioCallback(&sourcePlayer_);
  fpbr_.deregisterListener(this);
}