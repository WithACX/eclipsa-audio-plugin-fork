#include "IAMFDecoderSource.h"

#include <chrono>
#include <iostream>

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
  auto start = std::chrono::high_resolution_clock::now();
  std::cout << "  [DecoderSource] Acquiring lock..." << std::endl;
  
  const juce::SpinLock::ScopedLockType lock(decoderLock_);
  auto after_lock = std::chrono::high_resolution_clock::now();
  auto lock_duration = std::chrono::duration_cast<std::chrono::microseconds>(
      after_lock - start).count();
  std::cout << "  [DecoderSource] Lock acquired in: " << lock_duration << " μs" << std::endl;
  
  if (!decoder_.seekFrame(frameIndex)) {
    return false;
  }
  auto after_decoder_seek = std::chrono::high_resolution_clock::now();
  auto decoder_seek_duration = std::chrono::duration_cast<std::chrono::microseconds>(
      after_decoder_seek - after_lock).count();
  std::cout << "  [DecoderSource] Decoder seekFrame took: " << decoder_seek_duration << " μs" << std::endl;
  
  clearFifoBuffer();
  auto after_clear = std::chrono::high_resolution_clock::now();
  auto clear_duration = std::chrono::duration_cast<std::chrono::microseconds>(
      after_clear - after_decoder_seek).count();
  std::cout << "  [DecoderSource] Clear FIFO took: " << clear_duration << " μs" << std::endl;
  
  return true;
}

void IAMFDecoderSource::clearFifoBuffer() {
  if (fifo_ != nullptr) {
    fifo_ = std::make_unique<AudioFIFO>(kFifoSize, streamData_.numChannels);
    buffer_.clear();
  }
}

void IAMFDecoderSource::prepareToPlay(int, double) {
  const juce::SpinLock::ScopedLockType lock(decoderLock_);
  streamData_ = decoder_.getStreamData();
  buffer_.setSize(streamData_.numChannels, streamData_.frameSize);
  fifo_ = std::make_unique<AudioFIFO>(kFifoSize, streamData_.numChannels);
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

  const int samplesNeeded = info.numSamples;
  int samplesRemaining = samplesNeeded;

  // First try to read from the FIFO
  const int samplesAvailable = fifo_->getNumReady();
  if (samplesAvailable > 0) {
    const int samplesToRead = juce::jmin(samplesAvailable, samplesRemaining);
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
            ch, info.startSample + (samplesNeeded - samplesRemaining),
            samplesRemaining);
      }
      break;
    }

    // Write to FIFO
    fifo_->write(buffer_, samplesRead);

    // Read what we need from the FIFO
    const int samplesToRead = juce::jmin((int)samplesRead, samplesRemaining);
    fifo_->read(*info.buffer,
                info.startSample + (samplesNeeded - samplesRemaining),
                samplesToRead);
    samplesRemaining -= samplesToRead;
  }
}
