#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <algorithm>
#include <cstddef>

class PbRingBuffer {
 public:
  using Buffer = juce::AudioBuffer<float>;

  PbRingBuffer(const int num_channels, const size_t pad_samples = 1024)
      : kPad_(pad_samples),
        kCapacity_(3 * kPad_),
        buffer_(num_channels, kCapacity_),
        head_(0),
        tail_(0),
        count_(0) {}

  size_t availReadSamples() const { return count_; }

  size_t availWriteSamples() const {
    return std::min(kPad_, available(tail_, head_));
  }

  bool writeSamples(const size_t num_samples, const Buffer& in) {
    if (num_samples > availWriteSamples()) {
      return false;
    }

    if (tail_ + num_samples < kCapacity_) {
      // Enough space without wrapping
      for (int ch = 0;
           ch < std::min(buffer_.getNumChannels(), in.getNumChannels()); ++ch) {
        for (size_t i = 0; i < num_samples; ++i) {
          buffer_.setSample(ch, tail_ + i, in.getSample(ch, i));
        }
      }
      tail_ = (tail_ + num_samples) % kCapacity_;
    } else {
      // Need to wrap around
      const size_t first_chunk = kCapacity_ - tail_;
      const size_t second_chunk = num_samples - first_chunk;
      for (int ch = 0;
           ch < std::min(buffer_.getNumChannels(), in.getNumChannels()); ++ch) {
        for (size_t i = 0; i < first_chunk; ++i) {
          buffer_.setSample(ch, tail_ + i, in.getSample(ch, i));
        }
        for (size_t i = 0; i < second_chunk; ++i) {
          buffer_.setSample(ch, i, in.getSample(ch, first_chunk + i));
        }
      }

      tail_ = second_chunk;
    }
    count_ += num_samples;
    return true;
  }

  size_t readSamples(const size_t start_sample, const size_t num_samples,
                     Buffer& out) {
    const size_t kToRead = std::min(num_samples, availReadSamples());
    if (head_ + kToRead < kCapacity_) {
      // Can read in one continuous chunk
      for (int ch = 0;
           ch < std::min(buffer_.getNumChannels(), out.getNumChannels());
           ++ch) {
        for (size_t i = 0; i < kToRead; ++i) {
          out.setSample(ch, start_sample + i, buffer_.getSample(ch, head_ + i));
        }
      }
      head_ = (head_ + kToRead) % kCapacity_;
    } else {
      // Need to wrap around
      const size_t first_chunk = kCapacity_ - head_;
      const size_t second_chunk = kToRead - first_chunk;
      for (int ch = 0;
           ch < std::min(buffer_.getNumChannels(), out.getNumChannels());
           ++ch) {
        for (size_t i = 0; i < first_chunk; ++i) {
          out.setSample(ch, start_sample + i, buffer_.getSample(ch, head_ + i));
        }
        for (size_t i = 0; i < second_chunk; ++i) {
          out.setSample(ch, start_sample + first_chunk + i,
                        buffer_.getSample(ch, i));
        }
      }
      head_ = second_chunk;
    }

    count_ -= kToRead;
    return kToRead;
  }

  [[maybe_unused]] bool seek(const size_t num_samples, const bool forwards) {
    bool pos_in_buff;
    // Check if we can seek forwards or backwards by the requested sample count.
    // Do required pointer accounting.
    if (forwards && num_samples <= distance(head_, tail_)) {
      head_ = (head_ + num_samples) % kCapacity_;
      count_ -= num_samples;
      pos_in_buff = true;
    } else if (!forwards && num_samples <= kPad_) {
      head_ = (head_ + kCapacity_ - num_samples) % kCapacity_;
      count_ += num_samples;
      pos_in_buff = true;
    } else {
      buffer_.clear();
      head_ = tail_ = count_ = 0;
      pos_in_buff = false;
    }
    return pos_in_buff;
  }

 private:
  size_t available(const size_t head, const size_t tail) const {
    return kCapacity_ - distance(head, tail);
  }

  size_t distance(const size_t head, const size_t tail) const {
    if (tail >= head)
      return tail - head;
    else
      return kCapacity_ - head + tail;
  }

  const size_t kPad_, kCapacity_;
  Buffer buffer_;
  size_t head_, tail_, count_;
};
