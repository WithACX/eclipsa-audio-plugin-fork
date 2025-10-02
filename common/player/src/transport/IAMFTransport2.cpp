#include "IAMFTransport2.h"

#include "processors/file_output/iamf_export_utils/IAMFFileReader.h"

IAMFAudioSource::IAMFAudioSource(const std::filesystem::path& iamfFilePath)
    : kFilePath_(iamfFilePath),
      reader_(std::make_unique<IAMFFileReader>(iamfFilePath)) {}

void IAMFAudioSource::prepareToPlay(int samplesPerBlockExpected,
                                    double sampleRate) {
  // Call base class implementation first
  AudioSource::prepareToPlay(samplesPerBlockExpected, sampleRate);

  // Reset the reader to ensure we start from the beginning
  reader_ = std::make_unique<IAMFFileReader>(
      kFilePath_,
      IAMFFileReader::Settings{.requested_mix.output_layout =
                                   playbackLayout_.getIamfOutputLayout()});
  currentFrameIdx_ = 0;
}

void IAMFAudioSource::releaseResources() {
  // Call base class implementation
  AudioSource::releaseResources();
}

// Debug: This is the interesting one to implement
void IAMFAudioSource::getNextAudioBlock(
    const juce::AudioSourceChannelInfo& bufferToFill) {
  if (state_ != PlaybackState::kPlay) {
    bufferToFill.buffer->clear(bufferToFill.startSample,
                               bufferToFill.numSamples);
    return;
  }

  IAMFFileReader::StreamData streamData = reader_->getStreamData();
  juce::AudioBuffer<float> buffer(streamData.numChannels,
                                  bufferToFill.numSamples);
  size_t samplesDecoded = reader_->readFrame(buffer);

  if (samplesDecoded > 0 && buffer.getNumChannels() > 0) {
    const int numChannels =
        std::min(playbackLayout_.getNumChannels(), buffer.getNumChannels());
    for (int i = 0; i < numChannels; ++i) {
      bufferToFill.buffer->copyFrom(i, bufferToFill.startSample, buffer, i, 0,
                                    samplesDecoded);
    }
  }

  // Fill any remaining samples with silence
  if (samplesDecoded < bufferToFill.numSamples)
    for (int i = 0; i < playbackLayout_.getNumChannels(); ++i)
      bufferToFill.buffer->clear(i, bufferToFill.startSample + samplesDecoded,
                                 bufferToFill.numSamples - samplesDecoded);
}

void IAMFAudioSource::setPlaybackLayout(
    const Speakers::AudioElementSpeakerLayout layout) {
  playbackLayout_ = layout;
  reader_ = std::make_unique<IAMFFileReader>(
      kFilePath_, IAMFFileReader::Settings{.requested_mix.output_layout =
                                               layout.getIamfOutputLayout()});
}

void IAMFAudioSource::stop() {
  state_ = kStop;
  currentFrameIdx_ = 0;
  // TODO: Add a reset function to IAMFFileReader
  reader_ = std::make_unique<IAMFFileReader>(
      kFilePath_,
      IAMFFileReader::Settings{.requested_mix.output_layout =
                                   playbackLayout_.getIamfOutputLayout()});
}