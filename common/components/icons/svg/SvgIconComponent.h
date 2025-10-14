#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>

#include "components/icons/svg/SvgIconLookup.h"

class SvgIconComponent : public juce::Component {
 public:
  SvgIconComponent(const SvgMap::Icon icon)
      : icon_(icon),
        svgDrawable_(juce::Drawable::createFromSVG(
            *juce::parseXML(SvgMap::get(icon).data()))) {};

  void paint(juce::Graphics& g) override {
    if (svgDrawable_) {
      svgDrawable_->drawWithin(g, getLocalBounds().toFloat(),
                               juce::RectanglePlacement::centred, 1.0f);
    }
  }

 private:
  SvgMap::Icon icon_;
  std::unique_ptr<juce::Drawable> svgDrawable_;
};