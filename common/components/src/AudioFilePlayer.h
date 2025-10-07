#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "components/src/ColouredSlider.h"
#include "components/src/Icons.h"
#include "components/src/RoundImageButton.h"

class AudioFilePlayer : public juce::Component {
 public:
  AudioFilePlayer()
      : playButton_("Play", IconStore::getInstance().getPlayIcon()),
        pauseButton_("Pause", IconStore::getInstance().getPauseIcon()),
        stopButton_("Stop", IconStore::getInstance().getStopIcon()) {
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

    const int kButtonSz = 15;
    const int kButtonMargin = 10;
    playbackBounds_.removeFromLeft(kButtonMargin);
    juce::FlexBox playbackFlex;
    playbackFlex.flexDirection = juce::FlexBox::Direction::row;
    playbackFlex.justifyContent = juce::FlexBox::JustifyContent::spaceBetween;
    playbackFlex.alignItems = juce::FlexBox::AlignItems::center;
    playbackFlex.items.add(juce::FlexItem(playButton_)
                               .withFlex(.33f)
                               .withMinHeight(kButtonSz)
                               .withMinWidth(kButtonSz));
    playbackFlex.items.add(juce::FlexItem(pauseButton_)
                               .withFlex(.33f)
                               .withMinHeight(kButtonSz)
                               .withMinWidth(kButtonSz));
    playbackFlex.items.add(juce::FlexItem(stopButton_)
                               .withFlex(.33f)
                               .withMinHeight(kButtonSz)
                               .withMinWidth(kButtonSz));
    playbackFlex.performLayout(playbackBounds_);

    // juce::FlexBox volumeFlex;
    // volumeFlex.flexDirection = juce::FlexBox::Direction::row;
    // volumeFlex.justifyContent = juce::FlexBox::JustifyContent::spaceBetween;
    // volumeFlex.alignItems = juce::FlexBox::AlignItems::center;
    // volumeFlex.items.add(juce::FlexItem(volumeSlider_).withFlex(1.0f));
    // volumeFlex.performLayout(volumeBounds_);
  }

 private:
  RoundImageButton playButton_, pauseButton_, stopButton_;
  ColouredSlider playbackSlider_, volumeSlider_;
  //   SpeakerImageComponent speakerIcon_;
  juce::Rectangle<int> playbackBounds_, volumeBounds_;
};