#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>

#include "AudioPlayer.h"
#include "components/src/ColouredSlider.h"
#include "components/src/Icons.h"
#include "components/src/RoundImageButton.h"
#include "data_repository/implementation/FilePlaybackRepository.h"
#include "data_structures/src/FilePlayback.h"
#include "player/src/transport/IAMFPlaybackEngine.h"
#include "processors/file_output/iamf_export_utils/IAMFFileReader.h"

class AudioFilePlayer : public juce::Component, private juce::Timer {
 public:
  AudioFilePlayer(FilePlaybackRepository& filePlaybackRepo)
      : playButton_("Play", IconStore::getInstance().getPlayIcon()),
        pauseButton_("Pause", IconStore::getInstance().getPauseIcon()),
        stopButton_("Stop", IconStore::getInstance().getStopIcon()),
        timeLabel_("timeLabel", "00:00 / 00:00"),
        fpbr_(filePlaybackRepo),
        // Debug: Hardcoding this during testing
        playbackEngine_(std::make_unique<IAMFPlaybackEngine>(
            "/Users/joelm/Desktop/FIOTests/Reference2x2.wav.iamf",
            filePlaybackRepo)) {
    playButton_.onClick = [this]() {
      auto fpb = fpbr_.get();
      fpb.setPlayState(FilePlayback::kPlay);
      fpbr_.update(fpb);
    };
    pauseButton_.onClick = [this]() {
      auto fpb = fpbr_.get();
      fpb.setPlayState(FilePlayback::kPause);
      fpbr_.update(fpb);
    };
    stopButton_.onClick = [this]() {
      auto fpb = fpbr_.get();
      fpb.setPlayState(FilePlayback::kStop);
      fpbr_.update(fpb);
    };

    playbackSlider_.setRange(0.0, 1.0);
    playbackSlider_.setValue(0.0);
    playbackSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    playbackSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    playbackSlider_.onValueChange = [this]() {
      std::cout << "Hit slider change: " << playbackSlider_.getValue()
                << std::endl;
      auto fpb = fpbr_.get();
      fpb.setSeekPosition(static_cast<float>(playbackSlider_.getValue()));
      fpbr_.update(fpb);
    };
    addAndMakeVisible(playbackSlider_);

    volumeSlider_.setRange(0, 1);
    volumeSlider_.setValue(0.5);  // Default volume
    volumeSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    volumeSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(volumeSlider_);

    addAndMakeVisible(playButton_);
    addAndMakeVisible(pauseButton_);
    addAndMakeVisible(stopButton_);
    addAndMakeVisible(timeLabel_);

    update();
    startTimerHz(10);  // Update UI at 10 Hz
  }

  void paint(juce::Graphics& g) override { auto bounds = getLocalBounds(); }

  void resized() override {
    auto bounds = getLocalBounds();
    playbackBounds_ = bounds.removeFromTop(bounds.getHeight() * 0.5f);
    volumeBounds_ = bounds;

    int kButtonSz = (25);
    int kButtonMargin = (10);
    playButton_.setBounds(playbackBounds_.removeFromLeft(kButtonSz));
    playbackBounds_.removeFromLeft(kButtonMargin);
    stopButton_.setBounds(playbackBounds_.removeFromLeft(kButtonSz));
    timeLabel_.setBounds(playbackBounds_.removeFromLeft(100));
    playbackSlider_.setBounds(playbackBounds_);

    speakerIcon_.setBounds(volumeBounds_.removeFromLeft(kButtonSz));
    volumeSlider_.setBounds(
        volumeBounds_.removeFromLeft(100).reduced(kButtonMargin));
  }

  void update() {
    if (playbackEngine_) {
      const IAMFFileReader::StreamData kData = playbackEngine_->getStreamData();
      const float kDuration_s =
          kData.numFrames * kData.frameSize / (float)kData.sampleRate;
      const float kPosition_s =
          kData.currentFrameIdx * kData.frameSize / (float)kData.sampleRate;

      const int durationMins = (int)(kDuration_s / 60);
      const int durationSecs = (int)(kDuration_s) % 60;
      const int currentMins = (int)(kPosition_s / 60);
      const int currentSecs = (int)(kPosition_s) % 60;
      timeLabel_.setText(
          juce::String::formatted("%02d:%02d / %02d:%02d", currentMins,
                                  currentSecs, durationMins, durationSecs),
          juce::dontSendNotification);
      playbackSlider_.setValue(
          kDuration_s > 0.0f ? (kPosition_s / kDuration_s) : 0.0f,
          juce::dontSendNotification);
    } else {
      timeLabel_.setText("00:00 / 00:00", juce::dontSendNotification);
    }
  }

  void timerCallback() override {
    update();
    playbackEngine_->setVolume(volumeSlider_.getValue());
  }

 private:
  RoundImageButton playButton_, pauseButton_, stopButton_;
  juce::Label timeLabel_;
  ColouredSlider playbackSlider_, volumeSlider_;
  SpeakerImageComponent speakerIcon_;
  juce::Rectangle<int> playbackBounds_, volumeBounds_;
  FilePlaybackRepository& fpbr_;
  std::unique_ptr<class IAMFPlaybackEngine> playbackEngine_;
};