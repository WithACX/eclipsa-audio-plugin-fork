#include "IAMFTransport2.h"

#include "processors/file_output/iamf_export_utils/IAMFFileReader.h"

IAMFAudioSource::IAMFAudioSource(const std::filesystem::path& iamfFilePath)
    : kFilePath_(iamfFilePath),
      reader_(std::make_unique<IAMFFileReader>(iamfFilePath)) {}

void IAMFAudioSource::prepareToPlay(int samplesPerBlockExpected,
                                    double sampleRate) {
  // Reset the reader to ensure we start from the beginning
  reader_ = std::make_unique<IAMFFileReader>(
      kFilePath_,
      IAMFFileReader::Settings{.requested_mix.output_layout =
                                   decodeLayout_.getIamfOutputLayout()});
  currentFrameIdx_ = 0;
}

void IAMFAudioSource::releaseResources() {}

void IAMFAudioSource::getNextAudioBlock(
    const juce::AudioSourceChannelInfo& bufferToFill) {
  if (state_ != PlaybackState::kPlay) {
    bufferToFill.buffer->clear(bufferToFill.startSample,
                               bufferToFill.numSamples);
    return;
  }

  // Stream data may be one size, and the playback buffer another.
  // Currently taking as many of the decoded channels as will fit.
  IAMFFileReader::StreamData streamData = reader_->getStreamData();
  const unsigned kNumChannelsToCopy =
      std::min(bufferToFill.buffer->getNumChannels(), streamData.numChannels);
  unsigned numSamplesToCopy =
      std::min(bufferToFill.numSamples, (int)streamData.frameSize);

  juce::AudioBuffer<float> buffer(streamData.numChannels, streamData.frameSize);

  size_t samplesDecoded = reader_->readFrame(buffer);
  if (samplesDecoded > 0) {
    numSamplesToCopy = std::min(numSamplesToCopy, (unsigned)samplesDecoded);
    for (int i = 0; i < kNumChannelsToCopy; ++i) {
      bufferToFill.buffer->copyFrom(i, bufferToFill.startSample, buffer, i, 0,
                                    numSamplesToCopy);
    }
  }

  // Fill any remaining samples with silence
  if (numSamplesToCopy < bufferToFill.numSamples)
    for (int i = 0; i < decodeLayout_.getNumChannels(); ++i)
      bufferToFill.buffer->clear(i, bufferToFill.startSample + numSamplesToCopy,
                                 bufferToFill.numSamples - numSamplesToCopy);
}

void IAMFAudioSource::setPlaybackLayout(
    const Speakers::AudioElementSpeakerLayout layout) {
  decodeLayout_ = layout;
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
                                   decodeLayout_.getIamfOutputLayout()});
}