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
        samplePos_(0) {}

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
    const unsigned kAvailableSamples = getAvailableReadSamples();
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

    middle_ = (middle_ + kSamplesToRead) % kCapacity;

    updateDelay(kSamplesToRead);

    samplePos_ += kSamplesToRead;

    return kSamplesToRead;
  }

  bool setSamplePos(const unsigned newSamplePos) {
    bool reachable;
    const unsigned kFutureSamps = getAvailableReadSamples();
    const unsigned kPastSamps = bufferDistance(middle_, start_, capacity());
    // Check if reachable within buffer
    if (newSamplePos >= samplePos_ &&
        newSamplePos <= samplePos_ + kFutureSamps) {
      const unsigned kAdvance = newSamplePos - samplePos_;
      middle_ = (middle_ + kAdvance) % capacity();
      updateDelay(kAdvance);
      count_ -= kAdvance;
      reachable = true;
    } else if (newSamplePos <= samplePos_ &&
               newSamplePos >= samplePos_ - kPastSamps) {
      const unsigned kReverse = samplePos_ - newSamplePos;
      middle_ = (middle_ - kReverse) % capacity();
      reachable = true;
    } else {
      reachable = false;
      start_ = middle_ = end_ = count_ = 0;
      buffer_.clear();
    }
    samplePos_ = newSamplePos;
    return reachable;
  }

  // Accessors

  unsigned capacity() const { return buffer_.getNumSamples(); }

  unsigned getAvailableWriteSamples() const { return capacity() - size(); }

  unsigned getAvailableReadSamples() const {
    if (end_ == middle_) {
      return count_;
    }
    return bufferDistance(end_, middle_, capacity());
  }

  // Total samples in the buffer
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

  size_t bufferDistance(const size_t head, const size_t tail,
                        const size_t capacity) const {
    return (head >= tail) ? (head - tail) : (capacity - (tail - head));
  }

  juce::AudioBuffer<float> buffer_;
  unsigned padding_;
  unsigned start_, end_, middle_, count_;
  size_t samplePos_;
};