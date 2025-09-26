#include "../file_output/iamf_export_utils/IAMFFileReader.h"

#include <gtest/gtest.h>

#include <filesystem>

#include "FileOutputTestFixture.h"
#include "iamf_tools_api_types.h"
#include "processors/tests/FileOutputTestUtils.h"

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
  // Note: All input audio is resampled to 48kHz during IAMF encoding!
  EXPECT_EQ(kSData.sampleRate, 48e3) << kSData.sampleRate;
  EXPECT_EQ(kSData.frameSize, kSamplesPerFrame) << kSData.frameSize;
}

TEST_F(IAMFFileReaderTest, select_mix_presentation) {}

// The best way to test this reader is to write an IAMF file and keep the
// samples in memory. Then read the IAMF file back and compare the samples read
// against the original samples.
TEST_F(IAMFFileReaderTest, validate_readback) {
  createBasicIAMFFile(kReferenceFilePath);

  IAMFFileReader reader(kReferenceFilePath);
  const IAMFFileReader::StreamData kSData = reader.getStreamData();
  ASSERT_TRUE(kSData.valid);

  const juce::AudioBuffer<float> kReferenceBuffer =
      generateSineWave(440.0f, 48000, kSamplesPerFrame);

  juce::AudioBuffer<float> buffer(kSData.numChannels, kSData.frameSize);
  int totalFramesRead = 0;
  size_t samplesRead = 0;
  while ((samplesRead = reader.readFrame(buffer)) > 0) {
    ASSERT_EQ(samplesRead, (size_t)kSData.frameSize);
    // We expect each frame to closely match the reference buffer
    for (int ch = 0; ch < kSData.numChannels; ++ch) {
      for (int smp = 0; smp < kSData.frameSize; ++smp) {
        EXPECT_NEAR(buffer.getSample(ch, smp),
                    kReferenceBuffer.getSample(0, smp), 0.01f)
            << "Channel " << ch << " Sample " << smp;
      }
    }
    totalFramesRead++;
  }

  EXPECT_EQ(totalFramesRead, 8);  // We bounced 8 blocks of audio
}