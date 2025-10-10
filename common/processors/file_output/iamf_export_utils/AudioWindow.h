#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

class AudioWindow {
 public:
  AudioWindow() : buffer_(0, 0) {}

  AudioWindow(const unsigned numChannels, const unsigned numPadSamples)
      : buffer_(numChannels, numPadSamples * 2),
        padding_(numPadSamples),
        start_(0),
        middle_(0),
        end_(0),
        count_(0),
        absPos_(0) {}

  // Modifiers

  bool writeSamples(const unsigned numSamples,
                    const juce::AudioBuffer<float>& in) {
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
    count_ += numSamples;
    return true;
  }

  unsigned readSamples(const unsigned startSample, const unsigned numSamples,
                       juce::AudioBuffer<float>& out) {
    const unsigned kAvailableSamples = size();
    const unsigned kSamplesToRead = std::min(numSamples, kAvailableSamples);
    if (kSamplesToRead == 0) {
      return 0;
    }

    const unsigned kCapacity = capacity();
    const unsigned kNumChannels = out.getNumChannels();

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

    updateDelay(kSamplesToRead);

    // Update absolute sample position
    absPos_ += kSamplesToRead;

    return kSamplesToRead;
  }

  bool setAbsPos(const unsigned newAbsSamplePos) {}

  // Accessors

  unsigned capacity() const { return buffer_.getNumSamples(); }

  unsigned getAvailableWriteSamples() const { return capacity() - size(); }

  unsigned size() const { return count_; }

 private:
  void updateDelay(const unsigned sampsRead) {
    // If we read the entire buffer delay should be the padding.
    // Calculate delay buffer size (samples from start to middle)
    unsigned delayBufferSize;
    if (sampsRead == capacity()) {
      // TODO: Is there a better way to catch the case we read the entirety of
      // the buffer?
      delayBufferSize = capacity();
    } else if (middle_ >= start_) {
      delayBufferSize = middle_ - start_;
    } else {
      delayBufferSize = capacity() - start_ + middle_;
    }

    // If delay buffer exceeds padding, advance start and free up space
    if (delayBufferSize > padding_) {
      unsigned excessSamples = delayBufferSize - padding_;
      start_ = (start_ + excessSamples) % capacity();
      count_ -= excessSamples;
    }
  }

  juce::AudioBuffer<float> buffer_;
  unsigned padding_;
  unsigned start_, end_, middle_, count_;
  size_t absPos_;
};