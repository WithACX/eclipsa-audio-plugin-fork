#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

class SlidingAudioWindow {
 public:
  SlidingAudioWindow()
      : buffer_(0, 0), start_(0), middle_(0), end_(0), absSamplePos_(0) {}

  SlidingAudioWindow(const unsigned numChannels, const unsigned numPadSamples)
      : buffer_(numChannels, numPadSamples * 2),
        start_(0),
        middle_(0),
        end_(0),
        absSamplePos_(0) {}

  bool writeSamples(const unsigned numSamples,
                    const juce::AudioBuffer<float>& in) {
    // Check if the circular buffer has room to write the samples.
    // If so write them, advance the end pointer, and return true.
    // Otherwise return false.
    if (numSamples > getAvailableWriteSamples()) {
      return false;
    }

    const unsigned kCapacity = capacity();
    const unsigned kNumChannels = buffer_.getNumChannels();

    // Handle wrap-around if necessary
    if (end_ + numSamples <= kCapacity) {
      // No wrap-around needed
      for (unsigned ch = 0; ch < kNumChannels; ++ch) {
        buffer_.copyFrom(ch, end_, in, ch, 0, numSamples);
      }
    } else {
      // Need to wrap around
      const unsigned kChunk1Sz = kCapacity - end_;
      const unsigned kChunk2Sz = numSamples - kChunk1Sz;

      for (unsigned ch = 0; ch < kNumChannels; ++ch) {
        buffer_.copyFrom(ch, end_, in, ch, 0, kChunk1Sz);
        buffer_.copyFrom(ch, 0, in, ch, kChunk1Sz, kChunk2Sz);
      }
    }

    end_ = (end_ + numSamples) % kCapacity;
    return true;
  }

  unsigned readSamples(const unsigned startSample, const unsigned numSamples,
                       juce::AudioBuffer<float>& out) {
    // Check if we can return the required samples. Samples are read from the
    // middle + numSamples.
    // Reading samples advances:
    // 1. The middle pointer.
    // 2. The start pointer if the buffer zone has been filled.
    // Return the number of samples read.

    // Calculate available samples from middle to end
    unsigned availableSamples;
    if (end_ >= middle_) {
      availableSamples = end_ - middle_;
    } else {
      availableSamples = capacity() - middle_ + end_;
    }

    const unsigned kSamplesToRead = std::min(numSamples, availableSamples);
    if (kSamplesToRead == 0) {
      return 0;
    }

    const unsigned kCapacity = capacity();
    const unsigned kNumChannels = buffer_.getNumChannels();

    // Copy samples from middle pointer
    if (middle_ + kSamplesToRead <= kCapacity) {
      // No wrap-around needed
      for (unsigned ch = 0; ch < kNumChannels; ++ch) {
        out.copyFrom(ch, startSample, buffer_, ch, middle_, kSamplesToRead);
      }
    } else {
      // Need to wrap around
      const unsigned firstChunkSize = kCapacity - middle_;
      const unsigned secondChunkSize = kSamplesToRead - firstChunkSize;

      for (unsigned ch = 0; ch < kNumChannels; ++ch) {
        out.copyFrom(ch, startSample, buffer_, ch, middle_, firstChunkSize);
        out.copyFrom(ch, startSample + firstChunkSize, buffer_, ch, 0,
                     secondChunkSize);
      }
    }

    // Advance middle pointer
    middle_ = (middle_ + kSamplesToRead) % kCapacity;

    // Calculate how many samples are in the delay buffer
    unsigned delayBufferSize;
    if (middle_ >= start_) {
      delayBufferSize = middle_ - start_;
    } else {
      delayBufferSize = kCapacity - start_ + middle_;
    }

    const unsigned kNumPadSamples = kCapacity / 2;

    // Advance start pointer if delay buffer is filled
    if (delayBufferSize >= kNumPadSamples) {
      start_ = (start_ + kSamplesToRead) % kCapacity;
    }

    // Update absolute sample position
    absSamplePos_ += kSamplesToRead;

    return kSamplesToRead;
  }

  void setAbsPos(const unsigned newAbsSamplePos) {
    // Reset all pointers and set the absolute position
    start_ = 0;
    middle_ = 0;
    end_ = 0;
    absSamplePos_ = newAbsSamplePos;
  }

  unsigned capacity() const { return buffer_.getNumSamples(); }

  // Returns the number of samples available for writing in the circular buffer
  unsigned getAvailableWriteSamples() const {
    if (end_ >= start_) {
      return capacity() - (end_ - start_) - 1;
    } else {
      return start_ - end_ - 1;
    }
  }

  // Total number of samples in the buffer
  unsigned size() const { return end_ - start_; }

 private:
  juce::AudioBuffer<float> buffer_;
  // Position pointers
  unsigned start_, end_, middle_;
  // The absolute sample count. While start end and middle pointers are
  // relative, this variable does accounting for the number of samples read out
  // via the middle pointer.
  unsigned absSamplePos_;
};