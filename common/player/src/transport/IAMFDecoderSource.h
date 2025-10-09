#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_core/juce_core.h>

#include <memory>

#include "IAMFBuffer.h"
#include "processors/file_output/iamf_export_utils/IAMFFileReader.h"

/**
 * @brief Audio source wrapper for the IAMF decoder. Uses IAMFBuffer for
 * automatic background buffering with 15-second padding. Thread-safe for
 * concurrent access from UI and audio threads.
 *
 */
class IAMFDecoderSource : public juce::AudioSource {
 public:
  explicit IAMFDecoderSource(const std::filesystem::path iamfPath);

  void play();
  void pause();
  void stop();
  bool seek(size_t frameIndex);

  void prepareToPlay(int, double) override;

  void releaseResources() override;

  void getNextAudioBlock(const juce::AudioSourceChannelInfo& info) override;

  bool isPlaying() const {
    const juce::SpinLock::ScopedLockType lock(stateLock_);
    return isPlaying_;
  }

  IAMFFileReader::StreamData getStreamData() {
    IAMFFileReader::StreamData data = streamData_;
    data.currentFrameIdx = actualFrameCount_;
    return data;
  }

 private:
  IAMFFileReader decoder_;
  IAMFFileReader::StreamData streamData_;
  std::unique_ptr<IAMFBuffer> buffer_;
  size_t actualFrameCount_;
  bool isPlaying_ = false;
  mutable juce::SpinLock stateLock_;
};