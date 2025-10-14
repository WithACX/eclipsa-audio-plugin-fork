#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "AudioFilePlayer.h"
#include "components/src/SelectionBox.h"
#include "data_repository/implementation/FilePlaybackRepository.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"

class ExportValidationComponent : public juce::Component {
 public:
  ExportValidationComponent(FilePlaybackRepository& filePlaybackRepo)
      : title_("Export validation", "Export validation"),
        layoutToDecode_("Mix Presentation Layout"),
        audioPlayer_(filePlaybackRepo) {
    title_.setColour(juce::Label::textColourId, juce::Colour(221, 228, 227));
    title_.setJustificationType(juce::Justification::left);
    title_.setFont(juce::Font("Roboto", 22.0f, juce::Font::plain));
    addAndMakeVisible(title_);

    for (const auto& layout : kLayouts) {
      layoutToDecode_.addOption(layout.toString());
    }
    addAndMakeVisible(layoutToDecode_);
    addAndMakeVisible(audioPlayer_);
  }

  void paint(juce::Graphics& g) override {}

  void resized() override {
    auto bounds = getLocalBounds();

    const int kRowHeight = 65, kPadding = 25;
    title_.setBounds(bounds.removeFromTop(kRowHeight));
    layoutToDecode_.setBounds(bounds.removeFromTop(kRowHeight));
    bounds.removeFromTop(kPadding);
    audioPlayer_.setBounds(bounds.removeFromTop(kRowHeight * 2));
  }

 private:
  const std::array<Speakers::AudioElementSpeakerLayout, 10> kLayouts{
      Speakers::kStereo,
      Speakers::k3Point1Point2,
      Speakers::k5Point1,
      Speakers::k5Point1Point2,
      Speakers::k5Point1Point4,
      Speakers::k7Point1,
      Speakers::k7Point1Point2,
      Speakers::k7Point1Point4,
      Speakers::kExpl9Point1Point6,
      Speakers::kBinaural};

  juce::Label title_;
  SelectionBox layoutToDecode_;
  AudioFilePlayer audioPlayer_;
};