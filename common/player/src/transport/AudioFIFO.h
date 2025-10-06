#pragma once

#include <juce_audio_devices/juce_audio_devices.h>

/**
 * @brief A generic audio FIFO buffer implementation using JUCE's AbstractFifo.
 * Provides thread-safe reading and writing of audio samples.
 */
class AudioFIFO {
 public:
  /**
   * @brief Constructs an AudioFIFO with the specified buffer size.
   * @param fifoSize The size of the FIFO buffer in samples.
   * @param numChannels The number of audio channels.
   */
  AudioFIFO(size_t fifoSize, int numChannels)
      : fifo_(fifoSize), buffer_(numChannels, fifoSize) {
    buffer_.clear();
  }

  /**
   * @brief Write samples to the FIFO buffer.
   * @param source The source buffer containing samples to write.
   * @param numSamples The number of samples to write.
   */
  void write(const juce::AudioBuffer<float>& source, int numSamples) {
    juce::AbstractFifo::ScopedWrite write(fifo_, numSamples);

    for (int ch = 0; ch < source.getNumChannels(); ++ch) {
      if (write.blockSize1 > 0)
        buffer_.copyFrom(ch, write.startIndex1, source, ch, 0,
                         write.blockSize1);
      if (write.blockSize2 > 0)
        buffer_.copyFrom(ch, write.startIndex2, source, ch, write.blockSize1,
                         write.blockSize2);
    }
  }

  /**
   * @brief Read samples from the FIFO buffer.
   * @param dest The destination buffer to write samples to.
   * @param startSample The starting sample position in the destination buffer.
   * @param numSamples The number of samples to read.
   */
  void read(juce::AudioBuffer<float>& dest, int startSample, int numSamples) {
    juce::AbstractFifo::ScopedRead read(fifo_, numSamples);

    for (int ch = 0; ch < dest.getNumChannels(); ++ch) {
      if (read.blockSize1 > 0)
        dest.copyFrom(ch, startSample, buffer_, ch, read.startIndex1,
                      read.blockSize1);
      if (read.blockSize2 > 0)
        dest.copyFrom(ch, startSample + read.blockSize1, buffer_, ch,
                      read.startIndex2, read.blockSize2);
    }
  }

  /**
   * @brief Get the number of samples available to read.
   * @return The number of samples that can be read from the FIFO.
   */
  int getNumReady() const { return fifo_.getNumReady(); }

  /**
   * @brief Get the total size of the FIFO buffer in samples.
   * @return The size of the FIFO buffer.
   */
  int getSize() const { return fifo_.getTotalSize(); }

 private:
  juce::AbstractFifo fifo_;
  juce::AudioBuffer<float> buffer_;
};
