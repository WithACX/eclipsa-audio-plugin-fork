#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_core/juce_core.h>

#include "processors/file_output/iamf_export_utils/IAMFFileReader.h"

/**
 * @brief Audio source wrapper for the IAMF decoder. Populates frames on demand.
 * Includes an internal FIFO buffer to handle varying sample request sizes.
 * TODO: Maybe this is where all the accessibility methods should go? Since the
 * decoder and this source are tightly coupled.
 *
 */
class IAMFDecoderSource : public juce::AudioSource {
 public:
  IAMFDecoderSource(const std::filesystem::path iamfPath)
      : decoder_(iamfPath) {}

  IAMFFileReader& getDecoder() { return decoder_; }

  void prepareToPlay(int, double) override {
    streamData_ = decoder_.getStreamData();
    buffer_.setSize(streamData_.numChannels, streamData_.frameSize);
    fifoBuffer_.setSize(streamData_.numChannels, kFifoSize);
    fifoBuffer_.clear();
  }

  void releaseResources() override {}

  void getNextAudioBlock(const juce::AudioSourceChannelInfo& info) override {
    const int samplesNeeded = info.numSamples;
    int samplesRemaining = samplesNeeded;

    // First try to read from the FIFO
    const int samplesAvailable = fifo_.getNumReady();
    if (samplesAvailable > 0) {
      const int samplesToRead = juce::jmin(samplesAvailable, samplesRemaining);
      readSamplesFromFifo(*info.buffer, info.startSample, samplesToRead);
      samplesRemaining -= samplesToRead;
    }

    // If we need more samples, read them from the decoder
    while (samplesRemaining > 0) {
      const size_t samplesRead = decoder_.readFrame(buffer_);
      if (samplesRead == 0) {
        // No more samples available, zero-pad the rest
        for (int ch = 0; ch < info.buffer->getNumChannels(); ++ch) {
          info.buffer->clear(
              ch, info.startSample + (samplesNeeded - samplesRemaining),
              samplesRemaining);
        }
        break;
      }

      // Write to FIFO
      writeSamplesToFifo(buffer_, samplesRead);

      // Read what we need from the FIFO
      const int samplesToRead = juce::jmin((int)samplesRead, samplesRemaining);
      readSamplesFromFifo(*info.buffer,
                          info.startSample + (samplesNeeded - samplesRemaining),
                          samplesToRead);
      samplesRemaining -= samplesToRead;
    }
  }

 private:
  // Write decoded samples to the FIFO buffer
  void writeSamplesToFifo(const juce::AudioBuffer<float>& source,
                          int numSamples) {
    juce::AbstractFifo::ScopedWrite write(fifo_, numSamples);

    for (int ch = 0; ch < source.getNumChannels(); ++ch) {
      if (write.blockSize1 > 0)
        fifoBuffer_.copyFrom(ch, write.startIndex1, source, ch, 0,
                             write.blockSize1);
      if (write.blockSize2 > 0)
        fifoBuffer_.copyFrom(ch, write.startIndex2, source, ch,
                             write.blockSize1, write.blockSize2);
    }
  }

  // Read samples from the FIFO buffer
  void readSamplesFromFifo(juce::AudioBuffer<float>& dest, int startSample,
                           int numSamples) {
    juce::AbstractFifo::ScopedRead read(fifo_, numSamples);

    for (int ch = 0; ch < dest.getNumChannels(); ++ch) {
      if (read.blockSize1 > 0)
        dest.copyFrom(ch, startSample, fifoBuffer_, ch, read.startIndex1,
                      read.blockSize1);
      if (read.blockSize2 > 0)
        dest.copyFrom(ch, startSample + read.blockSize1, fifoBuffer_, ch,
                      read.startIndex2, read.blockSize2);
    }
  }

  IAMFFileReader decoder_;
  IAMFFileReader::StreamData streamData_;
  juce::AudioBuffer<float> buffer_;

  // FIFO buffer for sample storage
  static constexpr size_t kFifoSize = 16384;
  juce::AbstractFifo fifo_{kFifoSize};
  juce::AudioBuffer<float> fifoBuffer_;
};