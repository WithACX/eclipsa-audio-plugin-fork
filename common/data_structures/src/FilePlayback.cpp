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

#include "FilePlayback.h"

#include "substream_rdr/substream_rdr_utils/Speakers.h"

FilePlayback::FilePlayback()
    : RepositoryItemBase({}),
      volume_(0),
      totalFileLength_(0),
      currentSecond_(0),
      playState_(CurrentPlayerState::kDisabled),
      audioElements_(""),
      loudnessInfo_(""),
      currentMixPresentation_(""),
      setCurrentSecond_(0),
      mixPresentations_("") {}

FilePlayback::FilePlayback(int volume, int totalFileLength, int currentSecond,
                           CurrentPlayerState playState,
                           juce::String audioElements,
                           juce::String loudnessInfo,
                           juce::String currentMixPresentation,
                           int setCurrentSecond, juce::String mixPresentations,
                           juce::String playbackFile, float seekPosition,
                           Speakers::AudioElementSpeakerLayout reqdDecodeLayout)
    : RepositoryItemBase({}),
      volume_(volume),
      totalFileLength_(totalFileLength),
      currentSecond_(currentSecond),
      playState_(playState),
      audioElements_(audioElements),
      loudnessInfo_(loudnessInfo),
      currentMixPresentation_(currentMixPresentation),
      setCurrentSecond_(setCurrentSecond),
      mixPresentations_(mixPresentations),
      playbackFile_(playbackFile),
      seekPosition_(seekPosition),
      reqdDecodeLayout_(reqdDecodeLayout) {}

FilePlayback FilePlayback::fromTree(const juce::ValueTree tree) {
  FilePlayback fpb(
      tree[kVolume], tree[kTotalFileLength], tree[kCurrentSecond],
      (CurrentPlayerState)(int)tree[kPlayState], tree[kAudioElements],
      tree[kLoudnessInfo], tree[kCurrentMixPresentation],
      tree[kSetCurrentSecond], tree[kMixPresentations], tree[kPlaybackFile],
      tree[kSeekPosition],
      (Speakers::AudioElementSpeakerLayout)tree[kReqdDecodeLayout]);
  return fpb;
}

juce::ValueTree FilePlayback::toValueTree() const {
  return {kTreeType,
          {{kVolume, volume_},
           {kTotalFileLength, totalFileLength_},
           {kCurrentSecond, currentSecond_},
           {kPlayState, (int)playState_},
           {kAudioElements, audioElements_},
           {kLoudnessInfo, loudnessInfo_},
           {kCurrentMixPresentation, currentMixPresentation_},
           {kSetCurrentSecond, setCurrentSecond_},
           {kMixPresentations, mixPresentations_},
           {kPlaybackFile, playbackFile_},
           {kSeekPosition, seekPosition_},
           {kReqdDecodeLayout, (int)reqdDecodeLayout_}}};
}