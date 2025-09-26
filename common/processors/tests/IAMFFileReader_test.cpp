#include "../file_output/iamf_export_utils/IAMFFileReader.h"

#include <gtest/gtest.h>
#include <juce_audio_formats/juce_audio_formats.h>

#include <filesystem>

#include "FileOutputTestFixture.h"
#include "iamf_tools_api_types.h"
#include "processors/tests/FileOutputTestUtils.h"

// Macro to enable writing rendered output to a file for debugging purposes.
#define RDR_TO_FILE

// Function for debug
static std::unique_ptr<juce::AudioFormatWriter> prepareWriter(
    const int sampleRate, const int numChannels, const juce::String& filename) {
  juce::File outputFile = juce::File::getCurrentWorkingDirectory()
                              .getParentDirectory()
                              .getChildFile(filename);
  juce::WavAudioFormat wavFormat;
  std::unique_ptr<juce::FileOutputStream> outputStream(
      outputFile.createOutputStream());
  std::unique_ptr<juce::AudioFormatWriter> writer(
      wavFormat.createWriterFor(outputStream.get(), sampleRate, numChannels,
                                16,   // Bits per sample
                                {},   // Metadata
                                0));  // Default compression
  if (writer) {
    [[maybe_unused]] auto* released = outputStream.release();
  }
  return writer;
}

// Helper function to compare audio buffers with tolerance
static void compareAudioBuffers(const juce::AudioBuffer<float>& actualBuffer,
                                const juce::AudioBuffer<float>& referenceBuffer,
                                float tolerance = 0.0001f) {
  const int numChannels = actualBuffer.getNumChannels();
  const int frameSize = actualBuffer.getNumSamples();

  for (int ch = 0; ch < numChannels; ++ch) {
    for (int smp = 0; smp < frameSize; ++smp) {
      ASSERT_NEAR(actualBuffer.getSample(ch, smp),
                  referenceBuffer.getSample(0, smp), tolerance);
    }
  }
}

class IAMFFileReaderTest : public FileOutputTests {};

const std::filesystem::path kReferenceFilePath =
    std::filesystem::current_path() / "test_reader.iamf";

TEST_F(IAMFFileReaderTest, open_iamf) {
  createBasicIAMFFile(kReferenceFilePath);
  // Create a reader with sensible decoder settings
  const IAMFFileReader::Settings kSettings = {
      .requested_mix =
          {.output_layout =
               iamf_tools::api::OutputLayout::kItu2051_SoundSystemA_0_2_0},
      .requested_profile_versions =
          {iamf_tools::api::ProfileVersion::kIamfSimpleProfile},
      .requested_output_sample_type =
          iamf_tools::api::OutputSampleType::kInt32LittleEndian,
  };

  IAMFFileReader reader(kReferenceFilePath, kSettings);
  const IAMFFileReader::StreamData kSData = reader.getStreamData();
  EXPECT_TRUE(kSData.valid) << "Decoded IAMF stream data:\n";
  EXPECT_EQ(kSData.numChannels, 2) << kSData.numChannels;
  EXPECT_EQ(kSData.sampleRate, 48e3)
      << kSData.sampleRate;  // Note: All input audio is resampled to 48kHz
                             // during IAMF encoding!
  EXPECT_EQ(kSData.frameSize, kSamplesPerFrame) << kSData.frameSize;
}

TEST_F(IAMFFileReaderTest, open_iamf_different_playback) {
  createBasicIAMFFile(kReferenceFilePath);

  // Create a reader with sensible decoder settings
  const IAMFFileReader::Settings kSettings = {
      .requested_mix =
          {.output_layout =
               iamf_tools::api::OutputLayout::kItu2051_SoundSystemB_0_5_0},
  };
  IAMFFileReader reader(kReferenceFilePath, kSettings);

  const IAMFFileReader::StreamData kSData = reader.getStreamData();
  EXPECT_TRUE(kSData.valid) << "Decoded IAMF stream data:\n";
  EXPECT_EQ(kSData.numChannels, 6) << kSData.numChannels;
  EXPECT_EQ(kSData.sampleRate, 48e3)
      << kSData.sampleRate;  // Note: All input audio is resampled to 48kHz
                             // during IAMF encoding!
  EXPECT_EQ(kSData.frameSize, kSamplesPerFrame) << kSData.frameSize;
}

// Construct the decoder with an output layout that matches the AE in the file
TEST_F(IAMFFileReaderTest, read_same) {
  createBasicIAMFFile(kReferenceFilePath);

  IAMFFileReader reader(kReferenceFilePath);
  const IAMFFileReader::StreamData kSData = reader.getStreamData();
  ASSERT_TRUE(kSData.valid);

  const juce::AudioBuffer<float> kReferenceBuffer =
      generateSineWave(440.0f, 48000, kSamplesPerFrame);

  juce::AudioBuffer<float> buffer(kSData.numChannels, kSData.frameSize);
  size_t totalFramesRead = 0, samplesRead = 0;
  while ((samplesRead = reader.readFrame(buffer)) > 0) {
    ASSERT_EQ(samplesRead, (size_t)kSData.frameSize);
    compareAudioBuffers(buffer, kReferenceBuffer);
    totalFramesRead++;
  }

  EXPECT_EQ(totalFramesRead, 8);  // We bounced 8 blocks of audio
}

// Construct the decoder with an output layout that does not match the AE in the
// file
TEST_F(IAMFFileReaderTest, read_different) {
  createIAMFFile2AE2MP(kReferenceFilePath);
  const IAMFFileReader::Settings kSettings = {
      .requested_mix =
          {.output_layout =
               iamf_tools::api::OutputLayout::kItu2051_SoundSystemB_0_5_0},
  };
  IAMFFileReader reader(kReferenceFilePath, kSettings);
  const IAMFFileReader::StreamData kSData = reader.getStreamData();
  ASSERT_TRUE(kSData.valid);
}
