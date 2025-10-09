#include "IAMFDecoderSource.h"

IAMFDecoderSource::IAMFDecoderSource(const std::filesystem::path iamfPath)
    : decoder_(iamfPath), isPlaying_(false) {
  streamData_ = decoder_.getStreamData();
}

IAMFFileReader& IAMFDecoderSource::getDecoder() { return decoder_; }

void IAMFDecoderSource::play() {
  const juce::SpinLock::ScopedLockType lock(stateLock_);
  isPlaying_ = true;
}

void IAMFDecoderSource::pause() {
  const juce::SpinLock::ScopedLockType lock(stateLock_);
  isPlaying_ = false;
}

void IAMFDecoderSource::stop() {
  const juce::SpinLock::ScopedLockType lock(stateLock_);
  isPlaying_ = false;
  if (buffer_) {
    buffer_->seek(0);
  }
}

bool IAMFDecoderSource::seek(size_t frameIndex) {
  const juce::SpinLock::ScopedLockType lock(stateLock_);
  if (!buffer_) {
    return false;
  }
  return buffer_->seek(frameIndex);
}

void IAMFDecoderSource::prepareToPlay(int, double) {
  const juce::SpinLock::ScopedLockType lock(stateLock_);

  // Create IAMFBuffer which will start background thread for buffering
  buffer_ = std::make_unique<IAMFBuffer>(decoder_);

  // Wait for buffer to be ready before starting playback
  // This ensures we have enough buffered audio
  while (!buffer_->isReady()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

void IAMFDecoderSource::releaseResources() {
  const juce::SpinLock::ScopedLockType lock(stateLock_);
  // Destructor of IAMFBuffer will gracefully stop the background thread
  buffer_.reset();
}

void IAMFDecoderSource::getNextAudioBlock(
    const juce::AudioSourceChannelInfo& info) {
  const juce::SpinLock::ScopedLockType lock(stateLock_);

  if (!isPlaying_ || !buffer_) {
    info.clearActiveBufferRegion();
    return;
  }

  // Read directly from IAMFBuffer which is maintained by background thread
  buffer_->read(*info.buffer, info.startSample, info.numSamples);
}
