#pragma once
#include <condition_variable>
#include <mutex>
#include <stdexcept>

#include "SlidingAudioWindow.h"
#include "processors/file_output/iamf_export_utils/IAMFFileReader.h"

class IAMFBuffer {
 public:
  IAMFBuffer(const unsigned paddingSeconds, IAMFFileReader& decoder)
      : decoder_(decoder) {
    const auto kStreamData = decoder_.getStreamData();
    const unsigned kNumChannels = kStreamData.numChannels;
    const unsigned kSampleRate = kStreamData.sampleRate;
    const unsigned kFrameSize = kStreamData.frameSize;
    const unsigned kNumSamples =
        (paddingSeconds * kSampleRate / kFrameSize) * kFrameSize;
    slidingWindow_ = SlidingAudioWindow(kNumChannels, kNumSamples);
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
    std::unique_lock<std::mutex> lock(stateLock_);
    return slidingWindow_.size() > 0;
  }

  size_t availableSamples() const {
    std::unique_lock<std::mutex>(stateLock_);
    return slidingWindow_.size();
  }

  size_t readSamples(juce::AudioBuffer<float>& out, const unsigned startSample,
                     const unsigned numSamples) {
    stateLock_.lock();
    const size_t kSamplesRead =
        slidingWindow_.readSamples(startSample, numSamples, out);

    // Zero-pad if necessary.
    if (kSamplesRead < numSamples) {
      out.clear(kSamplesRead, numSamples - kSamplesRead);
    }
    stateLock_.unlock();
    wakeDecodeTask();

    return kSamplesRead;
  }

  bool seek(const size_t absSampleIdx) {
    stateLock_.lock();
    const bool kPosInBuff = slidingWindow_.setAbsPos(absSampleIdx);
    stateLock_.unlock();

    wakeDecodeTask();

    return kPosInBuff;
  }

  void wakeDecodeTask() { cv_.notify_one(); }

  void decodeTask() {
    while (!stopThread_) {
      std::unique_lock<std::mutex> lock(stateLock_);
      cv_.wait(lock, [this] {
        return stopThread_.load() ||
               slidingWindow_.getAvailableWriteSamples() >=
                   decoder_.getStreamData().frameSize;
      });

      if (stopThread_) return;

      // If the sliding window has space for a frame, decode more samples into
      // it.
      unsigned numWriteAvailable = slidingWindow_.getAvailableWriteSamples();
      while (numWriteAvailable >= decoder_.getStreamData().frameSize) {
        juce::AudioBuffer<float> tempBuffer(
            decoder_.getStreamData().numChannels,
            decoder_.getStreamData().frameSize);
        const size_t kSamplesDecoded = decoder_.readFrame(tempBuffer);
        if (kSamplesDecoded == 0) {
          break;
        }

        const bool kWriteSuccess =
            slidingWindow_.writeSamples(kSamplesDecoded, tempBuffer);
        if (!kWriteSuccess) {
          throw std::runtime_error(
              "Write to buffer failed when there was available room!");
        }
        numWriteAvailable -= kSamplesDecoded;
      }
    }
  }

 private:
  IAMFFileReader& decoder_;
  SlidingAudioWindow slidingWindow_;
  std::thread decodeThread_;
  std::atomic<bool> stopThread_ = false;
  std::condition_variable cv_;
  std::mutex stateLock_;
};