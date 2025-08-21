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

#include "FileOutputProcessor.h"

#include <logger/logger.h>

#include <iostream>
#include <string>
#include <unordered_map>  // Include the necessary header for HashTable
#include <vector>

#include "absl/status/status.h"
#include "data_structures/src/AudioElement.h"
#include "data_structures/src/FileExport.h"
#include "data_structures/src/MixPresentationLoudness.h"
#include "iamf/include/iamf_tools/iamf_encoder_factory.h"
#include "iamf_encoder_interface.h"
#include "iamf_export_utils/IAMFExportUtil.h"
#include "iamf_tools_encoder_api_types.h"
#include "processors/render/RenderProcessor.h"
#include "user_metadata.pb.h"

//==============================================================================
FileOutputProcessor::FileOutputProcessor(
    FileExportRepository& fileExportRepository,
    AudioElementRepository& audioElementRepository,
    MixPresentationRepository& mixPresentationRepository,
    MixPresentationLoudnessRepository& mixPresentationLoudnessRepository)
    : ProcessorBase(),
      performingRender_(false),
      fileExportRepository_(fileExportRepository),
      audioElementRepository_(audioElementRepository),
      mixPresentationRepository_(mixPresentationRepository),
      mixPresentationLoudnessRepository_(mixPresentationLoudnessRepository),
      numSamples_(512) {}

FileOutputProcessor::~FileOutputProcessor() {}

//==============================================================================
const juce::String FileOutputProcessor::getName() const {
  return {"FileOutput"};
}

void FileOutputProcessor::initIamfMetadata(
    iamf_tools_cli_proto::UserMetadata& iamfMD,
    juce::String outputFileName) const {
  // NOTE: Leaving these test vector MD fields here for now as removal causes
  // silent export failure.
  iamfMD.mutable_test_vector_metadata()
      ->set_partition_mix_gain_parameter_blocks(false);
  iamfMD.mutable_test_vector_metadata()->set_file_name_prefix(
      outputFileName.toStdString());
}

void FileOutputProcessor::updateIamfMDFromRepositories(
    iamf_tools_cli_proto::UserMetadata& iamfMD) {
  // From member repositories, update matching fields in the IAMF metadata.
  // Create a map of audio element UUID's to ints
  audioElementIDMap_.clear();
  updateIamfMDFromRepository(fileExportRepository_, iamfMD);
  updateIamfMDFromRepository(audioElementRepository_, iamfMD,
                             audioElementIDMap_);
  updateIamfMDFromRepository(mixPresentationRepository_, iamfMD,
                             audioElementIDMap_);
}

void FileOutputProcessor::updateIamfMDFromRepository(
    FileExportRepository& fileExportRepository,
    iamf_tools_cli_proto::UserMetadata& iamfMD) {
  // Pull down file export data from repository
  FileExport fileExportData = fileExportRepository.get();

  // iamfMD.clear_codec_config_metadata();
  // iamfMD.clear_ia_sequence_header_metadata();

  IAMFExportHelper::writeIASeqHdr(fileExportData.getProfile(), iamfMD);

  switch (fileExportData.getAudioCodec()) {
    case AudioCodec::FLAC:
      IAMFExportHelper::writeFLACConfigMD(
          numSamples_, sampleTally_, fileExportData.getBitDepth(),
          fileExportData.getFlacCompressionLevel(), iamfMD);
      break;
    case AudioCodec::OPUS:
      IAMFExportHelper::writeOPUSConfigMD(
          sampleRate_, fileExportData.getOpusTotalBitrate(), iamfMD);
      break;
    case AudioCodec::LPCM:
    default:
      IAMFExportHelper::writeLPCMConfigMD(
          numSamples_, sampleTally_, sampleRate_,
          fileExportData.getLPCMSampleSize(), iamfMD);
      break;
  }
}

void FileOutputProcessor::updateIamfMDFromRepository(
    AudioElementRepository& audioElementRepository,
    iamf_tools_cli_proto::UserMetadata& iamfMD,
    std::unordered_map<juce::Uuid, int>& audioElementIDMap) {
  // Pull down audio elements from repository.
  juce::OwnedArray<AudioElement> audioElements;
  audioElementRepository.getAll(audioElements);

  // Clear any existing metadata.
  // iamfMD.clear_audio_element_metadata();
  // iamfMD.clear_audio_frame_metadata();

  // For each audio element, add and populate: audio_element_metadata and
  // audio_frame_metadata.
  int minAudioSubstreamForElement = 0;
  int firstAudioElementId = 500;
  for (int i = 0; i < iamfWavFileWriters_.size(); ++i) {
    auto aeMDToPopulate = iamfMD.add_audio_element_metadata();
    juce::Uuid elementID = iamfWavFileWriters_[i].get()->getElement().getId();
    audioElementIDMap[elementID] = ++firstAudioElementId;
    int aeID = audioElementIDMap[elementID];
    iamfWavFileWriters_[i].get()->getElement().populateIamfAudioElementMetadata(
        aeMDToPopulate, aeID, minAudioSubstreamForElement);
    auto afMDToPopulate = iamfMD.add_audio_frame_metadata();
    iamfWavFileWriters_[i].get()->getElement().populateIamfAudioFrameMetadata(
        afMDToPopulate, aeID, iamfWavFileWriters_[i].get()->getFilePath(),
        iamfWavFileWriters_[i].get()->getFramesWritten());
  }
}

void FileOutputProcessor::updateIamfMDFromRepository(
    MixPresentationRepository& mixPresentationRepository,
    iamf_tools_cli_proto::UserMetadata& iamfMD,
    std::unordered_map<juce::Uuid, int>& audioElementIDMap) {
  // Pull down mix presentations from repository.
  juce::OwnedArray<MixPresentation> mixPresentations;
  // jassert(mixPresentationRepository.getItemCount() != 0);
  mixPresentationRepository.getAll(mixPresentations);

  // Clear any existing mix_presentation_metadata.
  // iamfMD.clear_mix_presentation_metadata();

  // For each mix presentation, add and populate: mix_presentation_metadata.
  for (int i = 0; i < mixPresentations.size(); ++i) {
    const juce::Uuid mixPresentationId = mixPresentations[i]->getId();
    MixPresentationLoudness mixPresentationLoudness =
        mixPresentationLoudnessRepository_.get(mixPresentationId).value();
    auto mpMDToPopulate = iamfMD.add_mix_presentation_metadata();
    mixPresentations[i]->populateIamfMixPresentationMetadata(
        i, sampleRate_, sampleTally_, mpMDToPopulate, iamfMD,
        mixPresentationLoudness, audioElementIDMap);
  }
}
//==============================================================================
void FileOutputProcessor::prepareToPlay(double sampleRate,
                                        int samplesPerBlock) {
  FileExport configParams = fileExportRepository_.get();
  if (configParams.getSampleRate() != sampleRate) {
    LOG_ANALYTICS(0, "FileOutputProcessor sample rate changed to " +
                         std::to_string(sampleRate));
    configParams.setSampleRate(sampleRate);
    fileExportRepository_.update(configParams);
  }
  numSamples_ = samplesPerBlock;
  sampleTally_ = 0;
  sampleRate_ = sampleRate;
}

void FileOutputProcessor::setNonRealtime(bool isNonRealtime) noexcept {
  if (isNonRealtime == performingRender_) {
    return;
  }

  FileExport config = fileExportRepository_.get();
  // Initialize the writer if we are rendering in offline mode
  if (!performingRender_) {
    if ((config.getAudioFileFormat() == AudioFileFormat::IAMF) &&
        (config.getExportAudio())) {
      initializeFileExport(config);
    }
    return;
  }

  // Stop rendering if we are switching back to online mode
  if (performingRender_) {
    closeFileExport(config);
    performingRender_ = false;
  }
}

void FileOutputProcessor::dumpExportLogs(const absl::Status& status) const {
  juce::OwnedArray<AudioElement> audioElements;
  juce::OwnedArray<MixPresentation> mixPresentations;
  audioElementRepository_.getAll(audioElements);
  mixPresentationRepository_.getAll(mixPresentations);

  std::vector<std::string> audioElementInfo;
  std::vector<std::string> mixPresInfo;

  // Format Audio Element and Mix Presentation data for logging.
  for (const AudioElement* ae : audioElements) {
    juce::String aeStr("Audio Element - " + ae->getId().toString() + " " +
                       ae->getDescription());
    audioElementInfo.push_back(aeStr.toStdString());
  }
  for (const MixPresentation* mp : mixPresentations) {
    juce::String aeStr("Mix Presentation - " + mp->getId().toString() + " " +
                       mp->getName() + " " +
                       juce::String(mp->getDefaultMixGain()));
    mixPresInfo.push_back(aeStr.toStdString());
  }

  // Dump formatted Audio Element and Mix Presentation data.
  LOG_INFO(0, "IAMF Export: Export attempted with:\n");
  for (const std::string& ae : audioElementInfo) {
    LOG_INFO(0, ae + "\n");
  }
  for (const std::string& mp : mixPresInfo) {
    LOG_INFO(0, mp + "\n");
  }
  LOG_INFO(0, "IAMF Export: IAMF export attempt completed with status: " +
                  status.ToString());
}

bool FileOutputProcessor::exportIamfFile(const juce::String input_wav_path,
                                         const juce::String output_iamf_path) {
  iamf_tools_cli_proto::UserMetadata iamfMetadata{};

  // juce::String exportFilename = fileExportRepository_.get().getExportFile();
  // if (exportFilename.endsWith(".iamf")) {
  //   // The IAMF export tool adds .iamf to the filename
  //   exportFilename = exportFilename.upToLastOccurrenceOf(".iamf", false,
  //   true);
  // }
  // initIamfMetadata(iamfMetadata, exportFilename);

  // updateIamfMDFromRepositories(iamfMetadata);

  LOG_INFO(0, "Exporting IAMF File with Metadata");
  LOG_INFO(0, iamfMetadata.DebugString());

  // auto res = iamf_tools::TestMain(iamfMetadata, input_wav_path.toStdString(),
  //                                 output_iamf_path.toStdString());

  // dumpExportLogs(res);

  // return res.ok();
  return true;
}

void FileOutputProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                       juce::MidiBuffer& midiMessages) {
  juce::ignoreUnused(midiMessages);

  if (!(shouldBufferBeWritten(buffer))) {
    // If we are not performing a render or the buffer is empty, do not write
    return;
  }

  juce::AudioBuffer<double> doubleBuffer;
  doubleBuffer.makeCopyOf(buffer);
  iamf_tools::api::IamfTemporalUnitData temporalUnitData;

  for (int i = 0; i < iamfWavFileWriters_.size(); ++i) {
    iamfWavFileWriters_[i]->write(buffer);

    convertBufferToIamfTemporalUnitData(temporalUnitData, doubleBuffer, i);
  }

  iamfEncoder_.value()->Encode(temporalUnitData);
}

//==============================================================================
bool FileOutputProcessor::hasEditor() const { return false; }

juce::AudioProcessorEditor* FileOutputProcessor::createEditor() {
  return nullptr;
}

void FileOutputProcessor::initializeFileExport(FileExport& config) {
  LOG_ANALYTICS(0, "Beginning .iamf file export");
  performingRender_ = true;
  std::string exportFile = config.getExportFile().toStdString();
  startTime_ = config.getStartTime();
  endTime_ = config.getEndTime();

  // To create the IAMF file, create a list of all the audio element wav
  // files to be created
  juce::OwnedArray<AudioElement> audioElements;
  audioElementRepository_.getAll(audioElements);
  iamfWavFileWriters_.clear();
  iamfWavFileWriters_.reserve(audioElements.size());
  for (int i = 0; i < audioElements.size(); i++) {
    juce::String wavFilePath =
        config.getExportFile() + "_audio_element_ " + juce::String(i) + ".wav";
    sampleRate_ = config.getSampleRate();

    iamfWavFileWriters_.emplace_back(new AudioElementFileWriter(
        wavFilePath, config.getSampleRate(), config.getBitDepth(),
        config.getAudioCodec(), *audioElements[i]));
  }
  sampleTally_ = 0;
  juce::String exportFilename = config.getExportFile();
  if (exportFilename.endsWith(".iamf")) {
    // The IAMF export tool adds .iamf to the filename
    exportFilename = exportFilename.upToLastOccurrenceOf(".iamf", false, true);
  }
  iamf_tools_cli_proto::UserMetadata iamfMetadata{};
  initIamfMetadata(iamfMetadata, exportFilename);
  updateIamfMDFromRepositories(iamfMetadata);
  std::cout << "DEBUG IAMF Metadata: " + iamfMetadata.DebugString()
            << std::endl;
  iamfEncoder_ =
      iamf_tools::api::IamfEncoderFactory::CreateFileGeneratingIamfEncoder(
          iamfMetadata, exportFile);
  std::cout << "DEBUG IAMF Encoder Status: " + iamfEncoder_.status().ToString()
            << std::endl;
}

void FileOutputProcessor::closeFileExport(FileExport& config) {
  LOG_ANALYTICS(0, "closing writers and exporting IAMF file");
  // close the output file, since rendering is completed
  for (auto& writer : iamfWavFileWriters_) {
    writer->close();
  }

  bool exportIAMFSuccess =
      exportIamfFile(config.getExportFolder(), config.getExportFolder());

  // juce::File outputFile = juce::File(config.getExportFile());
  // outputFile.deleteFile();

  bool finalizeEncode = iamfEncoder_.value()->FinalizeEncode().ok();

  while (iamfEncoder_.value()->GeneratingTemporalUnits()) {
    std::vector<uint8_t> temporal_unit_obus;
    iamfEncoder_.value()->OutputTemporalUnit(temporal_unit_obus);
  }

  // If muxing is enabled and audio export was successful, mux the audio and
  // video files.
  if (exportIAMFSuccess && fileExportRepository_.get().getExportVideo()) {
    bool muxIAMFSuccess = IAMFExportHelper::muxIAMF(
        audioElementRepository_, mixPresentationRepository_,
        fileExportRepository_.get());

    if (!muxIAMFSuccess) {
      LOG_INFO(0, "IAMF Muxing: Failed to mux IAMF file with provided video.");
    }
  }

  if (!config.getExportAudioElements()) {
    // Delete the extraneuos audio element files
    for (auto& writer : iamfWavFileWriters_) {
      juce::File audioElementFile(writer->getFilePath());
      audioElementFile.deleteFile();
    }
  }
  iamfWavFileWriters_.clear();
}

bool FileOutputProcessor::shouldBufferBeWritten(
    const juce::AudioBuffer<float>& buffer) {
  if (!performingRender_ || buffer.getNumSamples() < 1) {
    return false;
  }

  // Calculate the current time with the existing number of samples that have
  // been processed
  long currentTime = sampleTally_ / sampleRate_;
  // update the sample tally
  sampleTally_ += buffer.getNumSamples();
  // with the updated sample tally, calculate the next time
  long nextTime = sampleTally_ / sampleRate_;

  if (startTime_ != 0 || endTime_ != 0) {
    // Handle the case where startTime and endTime are set, implying we
    // are only bouncing a subset of the mix

    // do not render
    if (currentTime < startTime_ || nextTime > endTime_) {
      return false;
    }
  }
  return true;
}

void FileOutputProcessor::convertBufferToIamfTemporalUnitData(
    iamf_tools::api::IamfTemporalUnitData& temporalUnitData,
    const juce::AudioBuffer<double>& buffer, const int index) {
  temporalUnitData.parameter_block_id_to_metadata = {};
  // temporalUnitData.parameter_block_id_to_metadata.insert(
  //     {index, parameterBlockMetadata_});

  AudioElement audioElement = iamfWavFileWriters_[index]->getElement();
  std::vector<iamf_tools_cli_proto::ChannelLabel> channelLabels =
      audioElement.getChannelConfig().getIamfChannelLabels();

  const juce::Uuid audioElementJUCEUuid =
      iamfWavFileWriters_[index].get()->getElement().getId();

  const int audioElementID = audioElementIDMap_[audioElementJUCEUuid];
  // iamf_tools::api::IamfAudioElementData audioElementData;
  audioElementData_.clear();

  for (int j = audioElement.getFirstChannel();
       j < channelLabels.size() + audioElement.getFirstChannel(); ++j) {
    // absl::Span<const double> channelData(testSamples);
    absl::Span<const double> channelData(buffer.getReadPointer(j),
                                         buffer.getNumSamples());
    audioElementData_.insert(
        {channelLabels[j - audioElement.getFirstChannel()], {channelData}});
  }
  temporalUnitData.audio_element_id_to_data.insert(
      {audioElementID, audioElementData_});
  // juce::Logger::outputDebugString(
  //     "Audio Element w/ index: " + juce::String(index) +
  //     " added to TemporalUnitData w/ " + juce::String(channelLabels.size()) +
  //     " channels.");
  // juce::Logger::outputDebugString(
  //     "DEBUG Audio Element w/ JUCE Uuid: " + audioElementJUCEUuid.toString()
  //     + " and AudioElementID: " +
  //     juce::String(std::to_string(audioElementID)) + " added to
  //     TemporalUnitData w/ " + juce::String(audioElementData_.size()) + "
  //     labelled channels.");
}