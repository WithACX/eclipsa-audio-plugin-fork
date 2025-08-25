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
#include "IAMFTransport.h"

#include "../deserialization/PlaybackFile.h"
#include "../deserialization/PlaybackFileFactory.h"

IAMFTransport::IAMFTransport(int outputChannelCount,
                             FilePlaybackRepository* fileRepo,
                             PlaybackMonitorData* playbackData)
    : playbackFile_(nullptr),
      currentFilePath_(""),
      filePlaybackRepo_(fileRepo),
      playbackMonitorData_(playbackData) {
  transportSource.addChangeListener(this);
  setAudioChannels(0, outputChannelCount);
  fileRepo->registerListener(this);
}

void IAMFTransport::prepareToPlay(int samplesPerBlockExpected,
                                  double sampleRate) {
  transportSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void IAMFTransport::getNextAudioBlock(
    const juce::AudioSourceChannelInfo& bufferToFill) {
  if (playbackFile_ == nullptr) {
    bufferToFill.clearActiveBufferRegion();
    return;
  }

  // If the current second has changed, notify the UI
  if (playbackFile_ != nullptr) {
    int currentSecond = (int)transportSource.getCurrentPosition();
    if (currentSecond != currentPosition_) {
      currentPosition_ = currentSecond;
      playbackMonitorData_->currentPositionInSeconds = currentSecond;
    }
  }

  transportSource.getNextAudioBlock(bufferToFill);
}

void IAMFTransport::releaseResources() { transportSource.releaseResources(); }

void IAMFTransport::updateTransport() {
  auto fileRepository = filePlaybackRepo_->get();

  // First, check to see if the file to playback has changed
  if (!fileRepository.getPlaybackFile().equalsIgnoreCase(currentFilePath_)) {
    // If the file to playback has changed, reload the file and reset the
    // current state information
    if (playbackFile_ != nullptr) {
      transportSource.stop();
      delete playbackFile_;
      playbackFile_ = nullptr;
    }
    playbackFile_ = PlaybackFileFactory::createPlaybackFile(
        fileRepository.getPlaybackFile());
    if (playbackFile_ != NULL) {
      transportSource.setSource(playbackFile_->getAudioForPlayback(), 0,
                                nullptr, playbackFile_->getSampleRate());
      currentFilePath_ = fileRepository.getPlaybackFile();
      playbackMonitorData_->currentPositionInSeconds = 0;
      playbackMonitorData_->totalFileLength =
          transportSource.getLengthInSeconds();
    }
  }

  // If the file is still the same, check the current playback state matches the
  // current transport source state
  if (fileRepository.getPlayState() == CurrentPlayerState::PLAY) {
    if (transportSource.isPlaying() == false) {
      transportSource.start();
    }
  } else if (fileRepository.getPlayState() == CurrentPlayerState::STOP ||
             fileRepository.getPlayState() == CurrentPlayerState::DISABLED) {
    if (transportSource.isPlaying() == true) {
      transportSource.stop();
      transportSource.setPosition(0);
    }
  } else if (fileRepository.getPlayState() == CurrentPlayerState::PAUSE) {
    transportSource.stop();
  }

  // Finally, check current second to see if we need to skip forward/backward
}

void IAMFTransport::seekToLocation(double seconds) {
  transportSource.setPosition(seconds);
}