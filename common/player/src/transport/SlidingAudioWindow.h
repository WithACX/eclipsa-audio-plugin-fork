#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

class SlidingAudioWindow {
 public:
  SlidingAudioWindow()
      : buffer_(0, 0), start_(0), middle_(0), end_(0), absSamplePos_(0) {}

  SlidingAudioWindow(const unsigned numChannels, const unsigned numSamples)
      : buffer_(numChannels, numSamples),
        start_(0),
        middle_(0),
        end_(0),
        absSamplePos_(0),
        bufferZone_(numSamples) {}

  bool writeSamples(const unsigned numSamples,
                    const juce::AudioBuffer<float>& in) {
    // Check if the circular buffer has room to write the samples.
    // If so write them and return true.
    // Otherwise return false.
  }

  unsigned readSamples(const unsigned numSamples,
                       juce::AudioBuffer<float>& out) {
    // Check if we can return the required samples. Samples are read from the
    // middle + numSamples.
    // Reading samples advances:
    // 1. The middle pointer.
    // 2. The start pointer if the buffer zone has been filled.
    // Return the number of samples read.
  }

  void setAbsPos(const unsigned newAbsSamplePos) {
    // There are 3 possible cases here:
    // 1. The position is behind us
    // a. The position is in range.
    // OR
    // 2. The position is ahead of us
    // a. The position is in range.
    // The position is not in range. Clear everything.
  }

  unsigned getCapacity() const { return buffer_.getNumSamples(); }

  // Returns the number of samples available for writing in the circular buffer
  unsigned getAvailableWriteSamples() const {
    if (end_ >= start_) {
      return getCapacity() - (end_ - start_) - 1;
    } else {
      return start_ - end_ - 1;
    }
  }

 private:
  juce::AudioBuffer<float> buffer_;
  // Position pointers
  unsigned start_, end_, middle_;
  unsigned absSamplePos_;
  unsigned bufferZone_;
};