#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>

#include "components/icons/svg/SvgIconComponent.h"

class RoundImageButton : public juce::Button {
 public:
  RoundImageButton(const juce::String& buttonName, SvgMap::Icon svgIcon)
      : juce::Button(buttonName),
        icon(std::make_unique<SvgIconComponent>(svgIcon)) {
    setClickingTogglesState(false);  // only acts like a push button
    addAndMakeVisible(icon.get());
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
  }

  void resized() override {
    if (icon) {
      auto bounds = getLocalBounds().toFloat();
      auto diameter = juce::jmin(bounds.getWidth(), bounds.getHeight());
      auto circleBounds = bounds.withSizeKeepingCentre(diameter, diameter);
      auto iconSize = diameter * 0.5f;  // scale relative to button size
      auto iconBounds = juce::Rectangle<float>(iconSize, iconSize)
                            .withCentre(circleBounds.getCentre());
      icon->setBounds(iconBounds.toNearestInt());
    }
  }

 private:
  std::unique_ptr<SvgIconComponent> icon;
};
