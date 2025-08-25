// Copyright 2025 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <gtest/gtest.h>

#include "../../player/player.h"

TEST(test_iamf_transport, instantiation) {
  // // Ensure that the IAMFTransport class can be instantiated.
  // juce::File inputWavFile(
  //     "../../../../common/player/tests/test_resources/"
  //     "playback_drums.wav");
  // if (inputWavFile.exists() == false) {
  //   // This file path works when running the tests locally
  //   inputWavFile = juce::File(
  //       "./common/player/tests/test_resources/"
  //       "playback_drums.wav");
  // }

  // auto playbackFile = PlaybackFileFactory::createPlaybackFile(inputWavFile);

  // IAMFTransport iamfPlayer_ = IAMFTransport(2);
  // iamfPlayer_.loadForPlayback(playbackFile, 44100);
  // iamfPlayer_.prepareToPlay(512, 44100);
  // iamfPlayer_.play();

  // // Validate that stop and delete do not crash.
  // iamfPlayer_.stop();
}