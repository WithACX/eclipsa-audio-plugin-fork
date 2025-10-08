#pragma once
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

#include "AudioFIFO.h"
#include "processors/file_output/iamf_export_utils/IAMFFileReader.h"

/**
 * @brief The IAMFBuffer is a FIFO extension that maintains a ready buffer of
 * samples for the audio thread to pull from. Sample sourcing is done by an
 * independent thread. This buffer contains 30 seconds of audio with 15 seconds
 * of past/future audio padding maintained around the current playback position.
 * This buffer keeps track of the first and last frames of audio it contains.
 *
 */
class IAMFBuffer {
 public:
  IAMFBuffer(IAMFFileReader& decoder)
      : decoder_(decoder),
        startFrame_(0),
        endFrame_(0),
        currentPlaybackFrame_(0),
        shouldStop_(false),
        streamData_(decoder_.getStreamData()) {
    // Calculate number of frames for 30 seconds of audio samples
    const unsigned kTotalSeconds = 30;
    const unsigned kNumDecodedFrames =
        kTotalSeconds * streamData_.sampleRate / streamData_.frameSize;

    samplesPerFrame_ = streamData_.frameSize;
    paddingFrames_ = 15 * streamData_.sampleRate / streamData_.frameSize;

    fifo_ = std::make_unique<AudioFIFO>(kNumDecodedFrames * samplesPerFrame_,
                                        streamData_.numChannels);
    buffer_.setSize(streamData_.numChannels, samplesPerFrame_);

    // Start background thread for maintaining buffer
    ioThread_ = std::thread(&IAMFBuffer::bufferMaintenanceLoop, this);
  }

  ~IAMFBuffer() {
    // Signal thread to stop and wait for it to finish
    shouldStop_ = true;
    cv_.notify_all();
    if (ioThread_.joinable()) {
      ioThread_.join();
    }
  }

  // Deleted copy/move constructors and assignment operators to prevent issues
  // with thread management
  IAMFBuffer(const IAMFBuffer&) = delete;
  IAMFBuffer& operator=(const IAMFBuffer&) = delete;
  IAMFBuffer(IAMFBuffer&&) = delete;
  IAMFBuffer& operator=(IAMFBuffer&&) = delete;

  template <typename T>
  void read(juce::AudioBuffer<T>& buffer, int startSample, int numSamples) {
    // Read from FIFO (AbstractFifo is lock-free and thread-safe)
    fifo_->read(buffer, startSample, numSamples);

    // Update current playback position (in frames)
    const size_t framesConsumed = numSamples / samplesPerFrame_;
    currentPlaybackFrame_ += framesConsumed;

    // Notify background thread that we consumed data
    cv_.notify_one();
  }

  /**
   * @brief Get the current playback frame index.
   * @return Current frame being played.
   */
  size_t getCurrentFrame() const { return currentPlaybackFrame_.load(); }

  /**
   * @brief Get the range of frames currently buffered.
   * @return Pair of (startFrame, endFrame).
   */
  std::pair<size_t, size_t> getBufferedRange() const {
    std::lock_guard<std::mutex> lock(stateMutex_);
    return {startFrame_, endFrame_};
  }

  /**
   * @brief Check if buffer is ready for playback.
   * @return True if buffer has enough data to start playback.
   */
  bool isReady() const {
    // AbstractFifo::getNumReady() is thread-safe
    return fifo_->getNumReady() >=
           static_cast<int>(paddingFrames_ * samplesPerFrame_);
  }

  /**
   * @brief Seek to a specific frame position.
   * @param frameIdx The frame index to seek to.
   * @return True if seek was successful.
   */
  bool seek(size_t frameIdx) {
    std::lock_guard<std::mutex> lock(stateMutex_);

    if (!decoder_.seekFrame(frameIdx)) {
      return false;
    }

    currentPlaybackFrame_ = frameIdx;
    startFrame_ = frameIdx;
    endFrame_ = frameIdx;

    // Clear FIFO and notify background thread to refill
    // Note: This is a simplified approach; a more robust implementation
    // might use a different FIFO or reset mechanism
    cv_.notify_one();

    return true;
  }

 private:
  /**
   * @brief Background thread loop that maintains the buffer with 15-second
   * padding on either side of current playback position.
   */
  void bufferMaintenanceLoop() {
    while (!shouldStop_) {
      // Check if we need to buffer more data
      const size_t currentFrame = currentPlaybackFrame_.load();
      const size_t targetEndFrame = currentFrame + paddingFrames_;

      size_t localEndFrame;
      {
        std::lock_guard<std::mutex> lock(stateMutex_);
        localEndFrame = endFrame_;
      }

      // If we're getting low on buffered future audio, decode more
      if (localEndFrame < targetEndFrame &&
          localEndFrame < streamData_.numFrames) {
        const size_t framesToBuffer =
            std::min(targetEndFrame - localEndFrame,
                     streamData_.numFrames - localEndFrame);

        if (!decodeFrames(framesToBuffer)) {
          // End of file or error occurred
          break;
        }
      } else {
        // Wait for signal that more data is needed
        std::unique_lock<std::mutex> lock(stateMutex_);
        cv_.wait_for(lock, std::chrono::milliseconds(100));
      }
    }
  }

  /**
   * @brief Decode and buffer a specified number of frames.
   * @param framesToRead Number of frames to decode and buffer.
   * @return True if successful, false on error or end of file.
   */
  bool decodeFrames(size_t framesToRead) {
    for (size_t framesRead = 0; framesRead < framesToRead; ++framesRead) {
      const size_t samplesRead = decoder_.readFrame(buffer_);
      if (samplesRead == 0) {
        // End of file reached
        return false;
      }

      // Write to FIFO (AbstractFifo is lock-free and thread-safe)
      fifo_->write(buffer_, samplesRead);

      // Update frame tracking with lock protection
      {
        std::lock_guard<std::mutex> lock(stateMutex_);
        endFrame_++;
      }
    }
    return true;
  }

  IAMFFileReader& decoder_;
  size_t startFrame_;
  size_t endFrame_;
  std::atomic<size_t> currentPlaybackFrame_;
  size_t paddingFrames_;
  size_t samplesPerFrame_;

  std::unique_ptr<AudioFIFO> fifo_;
  juce::AudioBuffer<float> buffer_;
  IAMFFileReader::StreamData streamData_;

  // Thread synchronization
  std::thread ioThread_;
  mutable std::mutex stateMutex_;  // Protects startFrame_, endFrame_ only
  std::condition_variable cv_;
  std::atomic<bool> shouldStop_;
};