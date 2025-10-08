#pragma once
#include <memory>

#include "AudioFIFO.h"
#include "processors/file_output/iamf_export_utils/IAMFFileReader.h"

/**
 * @brief The IAMFBuffer is a FIFO extension that maintains a ready buffer of
 * samples for the audio thread to pull from. Sample sourcing is done by an
 * independent thread. This buffer contains 15 seconds of past/future audio.
 * This buffer keeps track of the first and last frames of audio it contains.
 *
 */
class IAMFBuffer {
 public:
  IAMFBuffer(IAMFFileReader& decoder)
      : decoder_(decoder), startFrame(0), endFrame(0) {
    // Floor of number of frames for 30 seconds of audio samples
    const IAMFFileReader::StreamData kStreamData = decoder_.getStreamData();
    const unsigned kNumDecodedFrames =
        30 * kStreamData.sampleRate / kStreamData.frameSize;

    fifo_ = std::make_unique<AudioFIFO>(
        kNumDecodedFrames * kStreamData.frameSize, kStreamData.numChannels);
    buffer_.setSize(kStreamData.numChannels, kStreamData.frameSize);

    // Populate FIFO at construction time
    std::thread ioThread(decodeFrames(kNumDecodedFrames, fifo_));
    ioThread.detach();
  }

  template <typename T>
  void read(juce::AudioBuffer<T>& buffer, int startSample, int numSamples) {
    // Add some logic here for maintaining 15 seconds of padding on either side
    // of the current active frame.

    fifo_->read(buffer, startSample, numSamples);
  }

 private:
  bool decodeFrames(const unsigned framesToRead,
                    std::unique_ptr<AudioFIFO>& fifo) {
    for (int framesRead = 0; framesRead < framesRead; ++framesRead) {
      const size_t kSamplesRead = decoder_.readFrame(buffer_);
      if (kSamplesRead == 0) {
        return false;
      }
      fifo_->write(buffer_, kSamplesRead);
    }
    return true;
  }

  IAMFFileReader& decoder_;
  size_t startFrame, endFrame;
  std::unique_ptr<AudioFIFO> fifo_;
  juce::AudioBuffer<float> buffer_;
};