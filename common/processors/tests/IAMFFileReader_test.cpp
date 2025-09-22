#include "../file_output/iamf_export_utils/IAMFFileReader.h"

#include <gtest/gtest.h>

#include "FileOutputTestFixture.h"
#include "iamf_tools_api_types.h"

class IAMFFileReaderTest : public FileOutputTests {};

// Open and close an existing IAMF file
TEST_F(IAMFFileReaderTest, read_iamf) {
  // Create a reader with sensible decoding settings
  const IAMFFileReader::Settings kSettings = {
      .requested_mix =
          {.output_layout =
               iamf_tools::api::OutputLayout::kItu2051_SoundSystemA_0_2_0},
      .requested_profile_versions =
          {iamf_tools::api::ProfileVersion::kIamfSimpleProfile},
      .requested_output_sample_type =
          iamf_tools::api::OutputSampleType::kInt32LittleEndian,
  };
  IAMFFileReader reader(kSettings);

  // Open the file and examine stream metadata
  const std::filesystem::path kReferenceFilePath =
      std::filesystem::current_path().parent_path() /
      "rendererplugin/test/testresources/HashSourceFileRelease.iamf";
  const IAMFFileReader::StreamData kSData =
      reader.open(std::filesystem::current_path());
  EXPECT_TRUE(kSData.valid);
  EXPECT_EQ(kSData.numChannels, 2);
  EXPECT_EQ(kSData.sampleRate, kSampleRate);
  EXPECT_EQ(kSData.frameSize, kSamplesPerFrame);

  // Close the file
  EXPECT_TRUE(reader.close());
}