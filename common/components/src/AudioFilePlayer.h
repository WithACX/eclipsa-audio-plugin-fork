#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "components/src/AudioPlayer.h"
#include "components/src/ColouredSlider.h"
#include "components/src/Icons.h"
#include "components/src/RoundImageButton.h"

class AudioFilePlayer : public juce::Component {
 public:
  AudioFilePlayer()
      : playButton_("Play", IconStore::getInstance().getPlayIcon()),
        pauseButton_("Pause", IconStore::getInstance().getPauseIcon()),
        stopButton_("Stop", IconStore::getInstance().getStopIcon()),
        timeLabel_("timeLabel", "00:00 / 00:00") {
    playbackSlider_.setRange(0.0, 1.0);
    playbackSlider_.setValue(0.0);
    playbackSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    playbackSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
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
  }

  void paint(juce::Graphics& g) override {
    auto bounds = getLocalBounds();

    // g.setColour(juce::Colours::darkgrey);
    // g.fillRoundedRectangle(bounds.toFloat(), 4.0f);
  }

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

 private:
  RoundImageButton playButton_, pauseButton_, stopButton_;
  juce::Label timeLabel_;
  ColouredSlider playbackSlider_, volumeSlider_;
  SpeakerImageComponent speakerIcon_;
  juce::Rectangle<int> playbackBounds_, volumeBounds_;
};