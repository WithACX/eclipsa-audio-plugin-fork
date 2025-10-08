#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_core/juce_core.h>

#include <memory>

#include "AudioFIFO.h"
#include "processors/file_output/iamf_export_utils/IAMFFileReader.h"

/**
 * @brief Audio source wrapper for the IAMF decoder. Populates frames on demand.
 * Includes an internal FIFO buffer to handle varying sample request sizes.
 * Thread-safe for concurrent access from UI and audio threads.
 *
 */
class IAMFDecoderSource : public juce::AudioSource {
 public:
  explicit IAMFDecoderSource(const std::filesystem::path iamfPath);

  IAMFFileReader& getDecoder();

  void play();
  void pause();
  void stop();
  bool seek(size_t frameIndex);

  void prepareToPlay(int, double) override;

  void releaseResources() override;

  void getNextAudioBlock(const juce::AudioSourceChannelInfo& info) override;

  bool isPlaying() const {
    const juce::SpinLock::ScopedLockType lock(decoderLock_);
    return isPlaying_;
  }

 private:
  void clearFifoBuffer();

  // 30 seconds at 48kHz
  static constexpr size_t kFifoSamples_ = 30 * 48e3;

  IAMFFileReader decoder_;
  IAMFFileReader::StreamData streamData_;
  juce::AudioBuffer<float> buffer_;
  std::unique_ptr<AudioFIFO> fifo_;
  bool isPlaying_ = false;
  juce::SpinLock decoderLock_;
};