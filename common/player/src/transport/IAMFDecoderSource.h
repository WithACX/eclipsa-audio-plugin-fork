#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_core/juce_core.h>

#include <memory>

#include "AudioFIFO.h"
#include "processors/file_output/iamf_export_utils/IAMFFileReader.h"

/**
 * @brief Audio source wrapper for the IAMF decoder. Populates frames on demand.
 * Includes an internal FIFO buffer to handle varying sample request sizes.
 *
 */
class IAMFDecoderSource : public juce::AudioSource {
 public:
  IAMFDecoderSource(const std::filesystem::path iamfPath)
      : decoder_(iamfPath), isPlaying_(false) {}

  IAMFFileReader& getDecoder() { return decoder_; }

  void play() { isPlaying_ = true; }
  void pause() { isPlaying_ = false; }
  void stop() {
    isPlaying_ = false;
    decoder_.seekFrame(0);
  }

  void prepareToPlay(int, double) override {
    streamData_ = decoder_.getStreamData();
    buffer_.setSize(streamData_.numChannels, streamData_.frameSize);
    fifo_ = std::make_unique<AudioFIFO>(kFifoSize, streamData_.numChannels);
  }

  void releaseResources() override { fifo_.reset(); }

  void getNextAudioBlock(const juce::AudioSourceChannelInfo& info) override {
    if (!isPlaying_) {
      info.clearActiveBufferRegion();
      return;
    }

    const int samplesNeeded = info.numSamples;
    int samplesRemaining = samplesNeeded;

    // First try to read from the FIFO
    const int samplesAvailable = fifo_->getNumReady();
    if (samplesAvailable > 0) {
      const int samplesToRead = juce::jmin(samplesAvailable, samplesRemaining);
      fifo_->read(*info.buffer, info.startSample, samplesToRead);
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
      fifo_->write(buffer_, samplesRead);

      // Read what we need from the FIFO
      const int samplesToRead = juce::jmin((int)samplesRead, samplesRemaining);
      fifo_->read(*info.buffer,
                  info.startSample + (samplesNeeded - samplesRemaining),
                  samplesToRead);
      samplesRemaining -= samplesToRead;
    }
  }

 private:
  IAMFFileReader decoder_;
  IAMFFileReader::StreamData streamData_;
  juce::AudioBuffer<float> buffer_;
  // FIFO buffer for sample storage
  static constexpr size_t kFifoSize = 16384;
  std::unique_ptr<AudioFIFO> fifo_;
  bool isPlaying_ = false;
};