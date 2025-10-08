#include "IAMFDecoderSource.h"

IAMFDecoderSource::IAMFDecoderSource(const std::filesystem::path iamfPath)
    : decoder_(iamfPath), isPlaying_(false) {}

IAMFFileReader& IAMFDecoderSource::getDecoder() { return decoder_; }

void IAMFDecoderSource::play() {
  const juce::SpinLock::ScopedLockType lock(decoderLock_);
  isPlaying_ = true;
}

void IAMFDecoderSource::pause() {
  const juce::SpinLock::ScopedLockType lock(decoderLock_);
  isPlaying_ = false;
}

void IAMFDecoderSource::stop() {
  const juce::SpinLock::ScopedLockType lock(decoderLock_);
  isPlaying_ = false;
  decoder_.seekFrame(0);
}

bool IAMFDecoderSource::seek(size_t frameIndex) {
  const juce::SpinLock::ScopedLockType lock(decoderLock_);
  if (!decoder_.seekFrame(frameIndex)) {
    return false;
  }
  clearFifoBuffer();
  return true;
}

void IAMFDecoderSource::clearFifoBuffer() {
  if (fifo_ != nullptr) {
    fifo_ = std::make_unique<AudioFIFO>(kFifoSamples_, streamData_.numChannels);
    buffer_.clear();
  }
}

void IAMFDecoderSource::prepareToPlay(int, double) {
  const juce::SpinLock::ScopedLockType lock(decoderLock_);
  streamData_ = decoder_.getStreamData();
  buffer_.setSize(streamData_.numChannels, streamData_.frameSize);
  fifo_ = std::make_unique<AudioFIFO>(kFifoSamples_, streamData_.numChannels);
}

void IAMFDecoderSource::releaseResources() {
  const juce::SpinLock::ScopedLockType lock(decoderLock_);
  fifo_.reset();
}

void IAMFDecoderSource::getNextAudioBlock(
    const juce::AudioSourceChannelInfo& info) {
  const juce::SpinLock::ScopedLockType lock(decoderLock_);

  if (!isPlaying_) {
    info.clearActiveBufferRegion();
    return;
  }

  const int kSamplesNeeded = info.numSamples;
  int samplesRemaining = kSamplesNeeded;

  // First try to read from the FIFO
  const int kSamplesAvailable = fifo_->getNumReady();
  if (kSamplesAvailable > 0) {
    const int samplesToRead = juce::jmin(kSamplesAvailable, samplesRemaining);
    fifo_->read(*info.buffer, info.startSample, samplesToRead);
    samplesRemaining -= samplesToRead;
  }

  // If we need more samples, read them from the decoder
  while (samplesRemaining > 0) {
    const size_t samplesRead = decoder_.readFrame(buffer_);
    if (samplesRead == 0) {
      // No more samples available, zero-pad the rest
      for (int ch = 0; ch < info.buffer->getNumChannels(); ++ch) {
        info.buffer->clear(
            ch, info.startSample + (kSamplesNeeded - samplesRemaining),
            samplesRemaining);
      }
      break;
    }

    // Write to FIFO
    fifo_->write(buffer_, samplesRead);

    // Read what we need from the FIFO
    const int samplesToRead = juce::jmin((int)samplesRead, samplesRemaining);
    fifo_->read(*info.buffer,
                info.startSample + (kSamplesNeeded - samplesRemaining),
                samplesToRead);
    samplesRemaining -= samplesToRead;
  }
}
