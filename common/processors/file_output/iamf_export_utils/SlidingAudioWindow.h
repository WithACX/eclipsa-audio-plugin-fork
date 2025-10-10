#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

class SlidingAudioWindow {
 public:
  SlidingAudioWindow() : buffer_(0, 0) {}

  SlidingAudioWindow(const unsigned numChannels, const unsigned numPadSamples)
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
    // Record the difference between end and middle.
    // Record how many samples we can read from between them whether it's equal
    // or less.
    // Read those samples into the buffer.
    // Increment middle by the number of samples read.
    // If the difference between end and mid becomes less than the padding,
    // increment start by that amount. Decrement count by that amount as that
    // space becomes free for writing.
  }

  bool setAbsPos(const unsigned newAbsSamplePos) {}

  // Accessors

  unsigned capacity() const { return buffer_.getNumSamples(); }

  unsigned getAvailableWriteSamples() const { return capacity() - size(); }

  unsigned size() const { return count_; }

 private:
  juce::AudioBuffer<float> buffer_;
  unsigned padding_;
  unsigned start_, end_, middle_, count_;
  size_t absPos_;
};