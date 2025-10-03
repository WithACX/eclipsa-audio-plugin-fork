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

#include <filesystem>
#include <thread>

#include "../../player/player.h"
#include "player/src/transport/FilePlaybackEngine.h"
#include "player/src/transport/IAMFTransport2.h"

// TEST(test_iamf_transport, instantiation) {
//   // // Ensure that the IAMFTransport class can be instantiated.
//   // juce::File inputWavFile(
//   //     "../../../../common/player/tests/test_resources/"
//   //     "playback_drums.wav");
//   // if (inputWavFile.exists() == false) {
//   //   // This file path works when running the tests locally
//   //   inputWavFile = juce::File(
//   //       "./common/player/tests/test_resources/"
//   //       "playback_drums.wav");
//   // }

//   // auto playbackFile =
//   PlaybackFileFactory::createPlaybackFile(inputWavFile);

//   // IAMFTransport iamfPlayer_ = IAMFTransport(2);
//   // iamfPlayer_.loadForPlayback(playbackFile, 44100);
//   // iamfPlayer_.prepareToPlay(512, 44100);
//   // iamfPlayer_.play();

//   // // Validate that stop and delete do not crash.
//   // iamfPlayer_.stop();
// }

TEST(test_iamf_transport, basic) {
  IAMFAudioSource iamfSource(std::filesystem::current_path() /
                             "test_reader.iamf");

  // Initialize audio device with stereo output
  juce::AudioDeviceManager deviceManager;
  auto result = deviceManager.initialise(0, 2, nullptr, true);
  // deviceManager.getCurrentAudioDevice(). This method I can see all the
  // information about the current device. deviceManager.getAudioDeviceSetup().
  // Same here
  ASSERT_TRUE(result.isEmpty())
      << "Failed to initialize audio: " << result.toStdString();

  // Configure audio source
  const int samplesPerBlock = 128;
  const double sampleRate = 16e3;
  iamfSource.prepareToPlay(samplesPerBlock, sampleRate);

  juce::AudioSourcePlayer audioSourcePlayer;
  audioSourcePlayer.prepareToPlay(sampleRate, samplesPerBlock);
  audioSourcePlayer.setSource(&iamfSource);

  deviceManager.addAudioCallback(&audioSourcePlayer);

  // Start playback
  iamfSource.play();

  // Play for a few seconds to ensure no crashes
  std::cout << "Playing audio for 5 seconds..." << std::endl;
  for (int i = 0; i < 5; ++i) {
    std::this_thread::sleep_for(std::chrono::seconds(2));
    std::cout << "." << std::flush;
  }
  std::cout << std::endl;

  // Clean up
  iamfSource.stop();
  deviceManager.removeAudioCallback(&audioSourcePlayer);
  audioSourcePlayer.setSource(nullptr);
  iamfSource.releaseResources();
}

TEST(test_iamf_transport, engine) {
  const std::filesystem::path kIamfReferencePath =
      std::filesystem::current_path() / "test_reader.iamf";

  // Initialize JUCE's message manager and make this the message thread.
  // This is required for testing with the device manager.
  if (!juce::MessageManager::getInstance()->isThisTheMessageThread()) {
    juce::MessageManager::getInstance()->setCurrentThreadAsMessageThread();
  }

  // Now we can safely create the engine
  auto player = std::make_unique<FilePlaybackEngine>(kIamfReferencePath);

  std::this_thread::sleep_for(std::chrono::seconds(2));
}