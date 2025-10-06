#pragma once

#include <juce_audio_devices/juce_audio_devices.h>

#include "processors/file_output/iamf_export_utils/IAMFFileReader.h"

/**
 * @brief Audio source wrapper for the IAMF decoder. Populates frames on demand.
 * TODO: Maybe this is where all the accessibility methods should go? Since the
 * decoder and this source are tightly coupled.
 *
 */
class IAMFDecoderSource : public juce::AudioSource {
 public:
  IAMFDecoderSource(const std::filesystem::path iamfPath) : decoder_(iamfPath) {
    streamData_ = decoder_.getStreamData();
  }

  IAMFFileReader& getDecoder() { return decoder_; }

  void prepareToPlay(int, double) override {
    buffer_.setSize(streamData_.numChannels, streamData_.frameSize);
  }

  void releaseResources() override {}

  void getNextAudioBlock(const juce::AudioSourceChannelInfo& info) override {
    const size_t kSamplesRead = decoder_.readFrame(buffer_);

    // Zero pad if less samples were received than expected
    if (kSamplesRead < info.numSamples) {
      for (int ch = 0; ch < info.buffer->getNumChannels(); ++ch) {
        info.buffer->clear(ch, info.startSample + kSamplesRead,
                           info.numSamples - kSamplesRead);
      }
    }
  }

 private:
  IAMFFileReader decoder_;
  IAMFFileReader::StreamData streamData_;
  juce::AudioBuffer<float> buffer_;
};