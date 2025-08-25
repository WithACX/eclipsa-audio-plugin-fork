#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <sys/errno.h>

#include "components/src/ColouredSlider.h"
#include "components/src/EclipsaColours.h"
#include "components/src/Icons.h"
#include "components/src/RoundImageButton.h"
#include "data_repository/implementation/FilePlaybackRepository.h"

class SpeakerImageComponent : public juce::Component {
 public:
  SpeakerImageComponent() {
    setSize(50, 50);
    speakerIcon_ = IconStore::getInstance().getSpeakerIcon();
  }

  void paint(juce::Graphics& g) override {
    g.drawImage(speakerIcon_, getLocalBounds().toFloat());
  }

 private:
  juce::Image speakerIcon_;
};

inline juce::String convertToMinutes(int seconds) {
  int minutes = seconds / 60;
  seconds = seconds % 60;
  return juce::String::formatted("%02d:%02d", minutes, seconds);
}

/*
  Creates an audio player object
*/
class AudioPlayerComponent : public juce::Component,
                             public juce::Slider::Listener,
                             public juce::ValueTree::Listener,
                             public juce::Timer {
 public:
  AudioPlayerComponent(FilePlaybackRepository* filePlaybackRepository,
                       PlaybackMonitorData* playbackMonitorData)
      : playButton("Play", IconStore::getInstance().getPlayIcon()),
        pauseButton("Pause", IconStore::getInstance().getPauseIcon()),
        stopButton("Stop", IconStore::getInstance().getStopIcon()),
        filePlaybackRepository_(filePlaybackRepository),
        playbackMonitorData_(playbackMonitorData) {
    playButton.setEnabled(false);

    // Configure button events
    playButton.onClick = [this]() {
      auto repo = filePlaybackRepository_->get();
      repo.setPlayState(PLAY);
      filePlaybackRepository_->update(repo);
    };
    pauseButton.onClick = [this]() {
      auto repo = filePlaybackRepository_->get();
      repo.setPlayState(PAUSE);
      filePlaybackRepository_->update(repo);
    };
    stopButton.onClick = [this]() {
      auto repo = filePlaybackRepository_->get();
      repo.setPlayState(STOP);
      filePlaybackRepository_->update(repo);
    };

    // // --- Playback slider ---
    playbackSlider.setRange(0.0, 1.0);
    playbackSlider.setValue(0.0);  // Default to 0 seconds
    playbackSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    playbackSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    playbackSlider.addListener(this);

    // // --- Volume slider ---
    volumeSlider.setRange(0, 1);
    // volumeSlider.setValue(filePlaybackRepository_->get().getVolume());
    volumeSlider.setValue(0.5);  // Default volume
    volumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    volumeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    volumeSlider.addListener(this);

    // // --- Time label ---
    juce::Colour textColour = juce::Colour(221, 228, 227);
    timeLabel.setColour(juce::Label::ColourIds::backgroundColourId,
                        juce::Colours::transparentBlack);
    timeLabel.setColour(juce::Label::textColourId, textColour);
    timeLabel.setFont(juce::Font("Roboto", 12.0f, juce::Font::plain));

    configurePlayerComponents();

    filePlaybackRepository_->registerListener(this);
    startTimer(100);
  }

  ~AudioPlayerComponent() {
    stopTimer();
    filePlaybackRepository_->deregisterListener(this);
  }

  void timerCallback() override {
    // Update the component's state or appearance
    repaint();
  }

  void valueTreePropertyChanged(juce::ValueTree& tree,
                                const juce::Identifier& property) override {
    if (tree == filePlaybackRepository_->getTree()) {
      configurePlayerComponents();
    }
  }

  void valueTreeChildAdded(juce::ValueTree& parentTree,
                           juce::ValueTree& childTree) override {
    if (parentTree == filePlaybackRepository_->getTree()) {
      configurePlayerComponents();
    }
  }

  void valueTreeChildRemoved(juce::ValueTree& parentTree,
                             juce::ValueTree& childWhichHasBeenRemoved,
                             int indexFromWhichChildWasRemoved) override {
    if (parentTree == filePlaybackRepository_->getTree()) {
      configurePlayerComponents();
    }
  }

  void valueTreeChildOrderChanged(
      juce::ValueTree& parentTreeWhoseChildrenHaveMoved, int oldIndex,
      int newIndex) override {
    if (parentTreeWhoseChildrenHaveMoved ==
        filePlaybackRepository_->getTree()) {
      configurePlayerComponents();
    }
  }

  void configurePlayerComponents() {
    auto repo = filePlaybackRepository_->get();

    // Enable / Disable buttons as needed
    if (repo.getPlayState() == DISABLED) {
      playButton.setEnabled(false);
      pauseButton.setEnabled(false);
      stopButton.setEnabled(false);
      playbackSlider.setEnabled(false);
    } else if (repo.getPlayState() == PLAY) {
      playButton.setEnabled(false);
      pauseButton.setEnabled(true);
      stopButton.setEnabled(true);
      playbackSlider.setEnabled(true);
    } else if (repo.getPlayState() == PAUSE) {
      playButton.setEnabled(true);
      pauseButton.setEnabled(false);
      stopButton.setEnabled(true);
      playbackSlider.setEnabled(true);
    } else if (repo.getPlayState() == STOP) {
      playButton.setEnabled(true);
      pauseButton.setEnabled(false);
      stopButton.setEnabled(false);
      playbackSlider.setEnabled(true);
    }
    repaint();
  }

  juce::Rectangle<int> centerArea(juce::Rectangle<int> area) {
    int width = area.getWidth();
    int additionalHeight = area.getHeight() - width;
    additionalHeight = additionalHeight / 2;
    area.reduce(0, additionalHeight);
    return area;
  }

  void paint(juce::Graphics& g) override {
    // Set the second information
    int currentPlaybackSecond = playbackMonitorData_->currentPositionInSeconds;
    int totalFileLength = playbackMonitorData_->totalFileLength;
    timeLabel.setText(convertToMinutes(currentPlaybackSecond) + " / " +
                          convertToMinutes(totalFileLength),
                      juce::dontSendNotification);

    // Set the playback slider location
    if (totalFileLength != 0) {
      playbackSlider.setValue((double)currentPlaybackSecond /
                              (double)totalFileLength);
    } else {
      playbackSlider.setValue(0.0);
    }

    // First, fill in the graphics with a rounded square
    g.setColour(EclipsaColours::tableAlternateGrey);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 20.0f);

    // Next, add the buttons
    auto graphicsArea = getLocalBounds();
    graphicsArea.removeFromLeft(5.f);

    playButton.setBounds(centerArea(graphicsArea.removeFromLeft(25)));
    graphicsArea.removeFromLeft(10.f);  // Add some padding
    pauseButton.setBounds(centerArea(graphicsArea.removeFromLeft(25)));
    graphicsArea.removeFromLeft(10.f);  // Add some padding
    stopButton.setBounds(centerArea(graphicsArea.removeFromLeft(25)));
    graphicsArea.removeFromLeft(15.f);  // Add some padding

    timeLabel.setBounds(graphicsArea.removeFromLeft(50));
    playbackSlider.setBounds(graphicsArea.removeFromLeft(200));
    graphicsArea.removeFromLeft(15);
    speakerIcon.setBounds(centerArea(graphicsArea.removeFromLeft(20)));
    volumeSlider.setBounds(graphicsArea.removeFromLeft(100));

    addAndMakeVisible(playButton);
    addAndMakeVisible(pauseButton);
    addAndMakeVisible(stopButton);
    addAndMakeVisible(playbackSlider);
    addAndMakeVisible(volumeSlider);
    addAndMakeVisible(timeLabel);
    addAndMakeVisible(speakerIcon);
  }

  void sliderValueChanged(juce::Slider* slider) override {
    if (slider == &playbackSlider && !isDragging) {
      double newPosition = playbackSlider.getValue() * totalLengthSeconds;
      auto repo = filePlaybackRepository_->get();
      repo.setSetCurrentSecond(newPosition);
      filePlaybackRepository_->update(repo);
    } else if (slider == &volumeSlider) {
      double newVolume = volumeSlider.getValue();
      auto repo = filePlaybackRepository_->get();
      repo.setVolume(newVolume);
      filePlaybackRepository_->update(repo);
    }
  }

 private:
  void updateTimeLabel(double current, double total) {
    auto formatTime = [](double t) {
      int minutes = int(t) / 60;
      int seconds = int(t) % 60;
      return juce::String::formatted("%02d:%02d", minutes, seconds);
    };
    timeLabel.setText(formatTime(current) + " / " + formatTime(total),
                      juce::dontSendNotification);
  }

  RoundImageButton playButton, pauseButton, stopButton;
  ColouredSlider playbackSlider, volumeSlider;
  SpeakerImageComponent speakerIcon;
  juce::Label timeLabel;
  FilePlaybackRepository* filePlaybackRepository_;
  PlaybackMonitorData* playbackMonitorData_;

  double totalLengthSeconds = 0.0;
  bool isDragging = false;
  bool isEnabled = true;

  const juce::Colour kBackgroundColour = juce::Colours::lightgrey;
};
