#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <cstddef>
#include <memory>

#include "processors/file_output/iamf_export_utils/IAMFFileReader.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"

class MyDecoderAudioSource : public juce::PositionableAudioSource {
 public:
  MyDecoderAudioSource(const std::filesystem::path& iamfFilePath)
      : decoder_(std::make_unique<IAMFFileReader>(iamfFilePath)) {}

  void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override {}

  void releaseResources() override {}

  void getNextAudioBlock(const juce::AudioSourceChannelInfo& info) override {
    // Clear buffer in case decoder gives fewer samples
    info.clearActiveBufferRegion();

    IAMFFileReader::StreamData streamData = decoder_->getStreamData();
    juce::AudioBuffer<float> buffer(streamData.numChannels,
                                    streamData.frameSize);

    size_t samplesDecoded = decoder_->readFrame(buffer);

    if (samplesDecoded > 0 && buffer.getNumChannels() > 0) {
      for (int i = 0; i < streamData.numChannels; ++i) {
        info.buffer->copyFrom(i, 0, buffer, i, 0, samplesDecoded);
      }
    }

    // Fill any remaining samples with silence
    if (samplesDecoded < streamData.frameSize) {
      for (int i = 0; i < streamData.numChannels; ++i) {
        info.buffer->clear(i, 0, streamData.frameSize - samplesDecoded);
      }
    }
  }

  void setNextReadPosition(juce::int64 newPosition) override {
    // decoder.seek(newPosition);
  }

  juce::int64 getNextReadPosition() const override {
    // return decoder.getPosition();
  }
  juce::int64 getTotalLength() const override { return -1; }
  bool isLooping() const override { return false; }

 private:
  std::unique_ptr<IAMFFileReader> decoder_;
};
