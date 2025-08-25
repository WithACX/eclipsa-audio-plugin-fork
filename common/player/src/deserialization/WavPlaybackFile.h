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

#include "PlaybackFile.h"

class WavPlaybackFile : public PlaybackFile {
 public:
  WavPlaybackFile(const juce::File& toLoad) {
    formatManager
        .registerBasicFormats();  // Register basic audio formats (handles WAV)
    reader = formatManager.createReaderFor(toLoad);
  }

  ~WavPlaybackFile() override {
    if (reader != nullptr) {
      delete reader;  // Clean up the reader
    }
  }

  int getSampleRate() override { return reader->sampleRate; }

  // Fetch audio for playback
  juce::AudioFormatReaderSource* getAudioForPlayback() {
    if (reader != nullptr) {
      return new juce::AudioFormatReaderSource(reader, false);
    }
    return nullptr;  // Return null if reader is not available
  }

 private:
  juce::AudioFormatManager formatManager;
  juce::AudioFormatReader* reader;
};