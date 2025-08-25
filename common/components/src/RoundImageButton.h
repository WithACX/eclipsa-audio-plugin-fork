#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

class RoundImageButton : public juce::Button {
 public:
  RoundImageButton(const juce::String& buttonName, juce::Image iconImage)
      : juce::Button(buttonName), icon(iconImage) {
    setClickingTogglesState(false);  // only acts like a push button
  }

  void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted,
                   bool shouldDrawButtonAsDown) override {
    auto bounds = getLocalBounds().toFloat();
    auto diameter = juce::jmin(bounds.getWidth(), bounds.getHeight());
    auto circleBounds = bounds.withSizeKeepingCentre(diameter, diameter);

    // Background color
    juce::Colour baseColour = juce::Colours::dodgerblue;
    if (isEnabled()) {
      if (shouldDrawButtonAsDown)
        baseColour = baseColour.darker(0.2f);
      else if (shouldDrawButtonAsHighlighted)
        baseColour = baseColour.brighter(0.2f);
    } else {
      baseColour = juce::Colours::darkgrey;
    }

    g.setColour(baseColour);
    g.fillEllipse(circleBounds);

    // Draw the icon centered
    if (icon.isValid()) {
      auto iconSize = diameter * 0.5f;  // scale relative to button size
      auto iconBounds = juce::Rectangle<float>(iconSize, iconSize)
                            .withCentre(circleBounds.getCentre());

      g.drawImage(icon, iconBounds);
    }
  }

 private:
  juce::Image icon;
};
