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

#include "player/src/transport/IAMFPlaybackEngine.h"

TEST(test_iamf_transport, engine) {
  const std::filesystem::path kIamfReferencePath =
      std::filesystem::current_path() / "test_reader.iamf";

  // Initialize JUCE's message manager and make this the message thread.
  // This is required for testing with the device manager.
  if (!juce::MessageManager::getInstance()->isThisTheMessageThread()) {
    juce::MessageManager::getInstance()->setCurrentThreadAsMessageThread();
  }

  // Now we can safely create the engine
  auto player = std::make_unique<IAMFPlaybackEngine>(kIamfReferencePath);

  player->play();
  std::this_thread::sleep_for(std::chrono::seconds(2));
  player->stop();
}