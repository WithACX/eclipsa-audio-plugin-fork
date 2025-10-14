#pragma once
#include <condition_variable>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <stdexcept>

#include "PbRingBuffer.h"
#include "processors/file_output/iamf_export_utils/IAMFFileReader.h"

class IAMFBuffer {
 public:
  IAMFBuffer(const unsigned paddingSeconds, IAMFFileReader& decoder)
      : decoder_(decoder) {
    const auto kStreamData = decoder_.getStreamData();
    const unsigned kNumChannels = kStreamData.numChannels;
    const unsigned kSampleRate = kStreamData.sampleRate;
    const unsigned kFrameSize = kStreamData.frameSize;
    const unsigned kNumSamples = paddingSeconds * kSampleRate;
    decoder_.seekFrame(0);
    window_ = std::make_unique<PbRingBuffer>(kNumChannels, kNumSamples);
    decodeThread_ = std::thread(&IAMFBuffer::decodeTask, this);
    wakeDecodeTask();
  }

  ~IAMFBuffer() {
    stopThread_ = true;
    cv_.notify_one();
    if (decodeThread_.joinable()) {
      decodeThread_.join();
    }
  }

  bool isReady() {
    const juce::SpinLock::ScopedLockType lock(bufferLock_);
    return window_->availReadSamples() > 0;
  }

  size_t availableSamples() const {
    const juce::SpinLock::ScopedLockType lock(bufferLock_);
    return window_->availReadSamples();
  }

  void wakeDecodeTask() {
    if (!reachedEOF_) {
      cv_.notify_one();
    }
  }

  bool hasReachedEOF() const { return reachedEOF_; }

  size_t readSamples(juce::AudioBuffer<float>& out, const unsigned startSample,
                     const unsigned numSamples) {
    size_t kSamplesRead;
    {
      const juce::SpinLock::ScopedLockType lock(bufferLock_);

      // If we've reached EOF and no samples are available, return 0
      if (reachedEOF_ && window_->availReadSamples() == 0) {
        out.clear(startSample, numSamples);
        return 0;
      }

      // TODO: / WARNING: with new API I'm disregarding startSample for now.
      kSamplesRead = window_->readSamples(numSamples, out);

      if (kSamplesRead < numSamples) {
        std::cout << "End of file in sight - read " << kSamplesRead << " of "
                  << numSamples << " samples, EOF: " << reachedEOF_.load()
                  << "\n";
      }

      // Zero-pad if necessary.
      if (kSamplesRead < numSamples) {
        out.clear(startSample + kSamplesRead, numSamples - kSamplesRead);
      }
    }
    absSamplePos_ += kSamplesRead;

    wakeDecodeTask();

    return kSamplesRead;
  }

  bool seek(const size_t newAbsPos) {
    {
      const juce::SpinLock::ScopedLockType lock(bufferLock_);
      size_t diff;
      if (newAbsPos > absSamplePos_) {
        diff = newAbsPos - absSamplePos_;
        window_->seek(diff, true);
      } else {
        diff = absSamplePos_ - newAbsPos;
        window_->seek(diff, false);
      }
      absSamplePos_ = newAbsPos;
      reachedEOF_ = false;
    }

    wakeDecodeTask();
    return true;
  }

  void decodeTask() {
    while (!stopThread_) {
      std::unique_lock<std::mutex> cvLock(cvMutex_);
      cv_.wait(cvLock, [this] {
        const juce::SpinLock::ScopedLockType lock(bufferLock_);
        return stopThread_.load() ||
               (!reachedEOF_.load() && window_->availWriteSamples() >=
                                           decoder_.getStreamData().frameSize);
      });

      if (stopThread_) return;
      if (reachedEOF_) continue;

      // If the sliding window has space for a frame, decode more samples into
      // it.
      const juce::SpinLock::ScopedLockType lock(bufferLock_);
      while (window_->availWriteSamples() >=
             decoder_.getStreamData().frameSize) {
        juce::AudioBuffer<float> tempBuffer(
            decoder_.getStreamData().numChannels,
            decoder_.getStreamData().frameSize);
        const size_t kSamplesDecoded = decoder_.readFrame(tempBuffer);
        if (kSamplesDecoded == 0) {
          reachedEOF_ = true;
          break;
        }

        const bool kWriteSuccess =
            window_->writeSamples(kSamplesDecoded, tempBuffer);
        if (!kWriteSuccess) {
          throw std::runtime_error(
              "IAMFBuffer: Write to buffer failed when there was available "
              "room!");
        }
      }
    }
  }

 private:
  IAMFFileReader& decoder_;
  std::unique_ptr<PbRingBuffer> window_;
  std::thread decodeThread_;
  std::atomic<bool> stopThread_ = false;
  std::atomic<bool> reachedEOF_ = false;
  std::condition_variable cv_;
  std::mutex cvMutex_;
  juce::SpinLock bufferLock_;
  size_t absSamplePos_ = 0;
};