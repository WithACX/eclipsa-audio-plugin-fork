
/*
 * Copyright 2025 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include "../deserialization/WavPlaybackFile.h"
#include "data_repository/implementation/FilePlaybackRepository.h"
#include "data_structures/src/PlaybackMonitorData.h"

class IAMFTransport : public juce::AudioAppComponent,
                      public juce::ChangeListener,
                      public juce::ValueTree::Listener {
 public:
  /*=============================================================================*
                          TRANSPORT COMPONENT FUNCTIONS
   *=============================================================================*/
  IAMFTransport(int outputChannelCount, FilePlaybackRepository* fileRepo,
                PlaybackMonitorData* playbackData);
  ~IAMFTransport() override {
    shutdownAudio();
    delete playbackFile_;
    filePlaybackRepo_->deregisterListener(this);
  }

  void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;

  void getNextAudioBlock(
      const juce::AudioSourceChannelInfo& bufferToFill) override;

  void releaseResources() override;

  void changeListenerCallback(juce::ChangeBroadcaster* source) override {
    if (source == &transportSource) {
      // Handle transport source changes if needed.
    }
  }

  /*=============================================================================*
                          TRANSPORT COMPONENT FUNCTIONS
   *=============================================================================*/
  void valueTreePropertyChanged(juce::ValueTree& tree,
                                const juce::Identifier& property) override {
    updateTransport();
  }
  void valueTreeChildAdded(juce::ValueTree& parentTree,
                           juce::ValueTree& childTree) override {
    updateTransport();
  }

  void valueTreeChildRemoved(juce::ValueTree& parentTree,
                             juce::ValueTree& childWhichHasBeenRemoved,
                             int indexFromWhichChildWasRemoved) override {
    updateTransport();
  }
  void valueTreeChildOrderChanged(juce::ValueTree& parentTree,
                                  const int property,
                                  const int newOrder) override {
    updateTransport();
  }
  void updateTransport();

  /*=============================================================================*
                          PLAYBACK FUNCTIONS
   *=============================================================================*/

  void seekToLocation(double seconds);
  void loadForPlayback(PlaybackFile* audioToPlay, int sampleRate);

 private:
  juce::AudioTransportSource transportSource;
  PlaybackFile* playbackFile_;
  juce::String currentFilePath_;
  FilePlaybackRepository* filePlaybackRepo_;
  PlaybackMonitorData* playbackMonitorData_;
  int currentPosition_;
};