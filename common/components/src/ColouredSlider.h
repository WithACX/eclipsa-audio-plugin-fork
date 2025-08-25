#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// Custom LookAndFeel (internal)
class BlueSliderLookAndFeel : public juce::LookAndFeel_V4 {
 public:
  void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                        float sliderPos, float minSliderPos, float maxSliderPos,
                        const juce::Slider::SliderStyle,
                        juce::Slider& slider) override {
    auto trackHeight = 4.0f;
    auto thumbRadius = 8.0f;

    auto trackBounds =
        juce::Rectangle<float>((float)x, y + height * 0.5f - trackHeight * 0.5f,
                               (float)width, trackHeight);

    // Background track (grey)
    g.setColour(juce::Colours::darkgrey);
    g.fillRect(trackBounds);

    // Value track (blue) – from left edge to sliderPos
    auto valueBounds = trackBounds.withWidth(sliderPos - (float)x);
    g.setColour(juce::Colours::dodgerblue);
    g.fillRect(valueBounds);

    // Thumb (blue circle)
    g.setColour(juce::Colours::dodgerblue);
    g.fillEllipse(sliderPos - thumbRadius,
                  trackBounds.getCentreY() - thumbRadius, thumbRadius * 2.0f,
                  thumbRadius * 2.0f);
  }
};

// Self-contained slider
class ColouredSlider : public juce::Slider {
 public:
  ColouredSlider() {
    setSliderStyle(juce::Slider::LinearHorizontal);
    setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    setLookAndFeel(&lookAndFeel);
  }

  ~ColouredSlider() override {
    setLookAndFeel(nullptr);  // avoid dangling pointer
  }

 private:
  BlueSliderLookAndFeel lookAndFeel;
};
