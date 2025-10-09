#pragma once
#include <condition_variable>
#include <mutex>

#include "SlidingAudioWindow.h"
#include "processors/file_output/iamf_export_utils/IAMFFileReader.h"

class IAMFBuffer {
 public:
  IAMFBuffer(const unsigned paddingSeconds, IAMFFileReader& decoder)
      : decoder_(decoder) {
    const auto streamData = decoder_.getStreamData();
    const unsigned numChannels = streamData.numChannels;
    const unsigned sampleRate = streamData.sampleRate;
    const unsigned numSamples = paddingSeconds * sampleRate;
    slidingWindow_ = SlidingAudioWindow(numChannels, numSamples);
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

  size_t readSamples(const unsigned numSamples, juce::AudioBuffer<float>& out) {
    const size_t kSamplesRead = slidingWindow_.readSamples(numSamples, out);

    // Zero-pad if necessary.
    if (kSamplesRead < numSamples) {
      out.clear(kSamplesRead, numSamples - kSamplesRead);
    }

    wakeDecodeTask();

    return kSamplesRead;
  }

  void seek(const size_t absSampleIdx) {
    // Set the absolute position of the sliding window.
    slidingWindow_.setAbsPos(absSampleIdx);

    wakeDecodeTask();
  }

  void wakeDecodeTask() { cv_.notify_one(); }

  void decodeTask() {
    while (!stopThread_) {
      std::unique_lock<std::mutex> lock(stateLock_);
      cv_.wait(lock, [this] { return !stopThread_.load(); });

      if (stopThread_) return;

      // If the sliding window has space, decode more samples into it.
      unsigned numWriteAvailable = slidingWindow_.getAvailableWriteSamples();
      while (numWriteAvailable > 0) {
        juce::AudioBuffer<float> tempBuffer(
            decoder_.getStreamData().numChannels, numWriteAvailable);
        const size_t kSamplesDecoded = decoder_.readFrame(tempBuffer);
        if (kSamplesDecoded == 0) {
          break;
        }

        slidingWindow_.writeSamples(kSamplesDecoded, tempBuffer);
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