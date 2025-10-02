#include "../file_output/iamf_export_utils/IAMFFileReader.h"

#include <gtest/gtest.h>
#include <juce_audio_formats/juce_audio_formats.h>

#include <filesystem>
#include <memory>

#include "FileOutputTestFixture.h"
#include "iamf_tools_api_types.h"
#include "processors/tests/FileOutputTestUtils.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"

class IAMFFileReaderTest : public FileOutputTests {};

const std::filesystem::path kReferenceFilePath =
    std::filesystem::current_path() / "test_reader.iamf";

// DEBUG:
const std::filesystem::path kDebugOutPath =
    std::filesystem::current_path().parent_path() / "test_reader.wav";

TEST_F(IAMFFileReaderTest, open_iamf) {
  createBasicIAMFFile(kReferenceFilePath);
  IAMFFileReader reader(kReferenceFilePath);

  const IAMFFileReader::StreamData kSData = reader.getStreamData();
  EXPECT_TRUE(kSData.valid);
  EXPECT_EQ(kSData.numChannels, 2);
  EXPECT_EQ(kSData.frameSize, kSamplesPerFrame);
  // Note: All input audio is resampled to 48kHz during IAMF encoding!
  EXPECT_EQ(kSData.sampleRate, 48e3);
}

// Open the file with a decoder output layout that differs from the AE layout in
// the file
TEST_F(IAMFFileReaderTest, open_iamf_different_playback) {
  createBasicIAMFFile(kReferenceFilePath);
  const IAMFFileReader::Settings kSettings = {
      .requested_mix =
          {.output_layout =
               iamf_tools::api::OutputLayout::kItu2051_SoundSystemB_0_5_0},
  };
  IAMFFileReader reader(kReferenceFilePath, kSettings);

  const IAMFFileReader::StreamData kSData = reader.getStreamData();
  EXPECT_TRUE(kSData.valid);
  EXPECT_EQ(kSData.numChannels, Speakers::k5Point1.getNumChannels());
  EXPECT_EQ(kSData.sampleRate, 48e3);
  EXPECT_EQ(kSData.frameSize, kSamplesPerFrame);
}

// In a file with multiple mix presentations, decodes the mix presentation with
// a `loudness_layout` matching the requested layout
TEST_F(IAMFFileReaderTest, multi_mix) {
  createIAMFFile2AE2MP(kReferenceFilePath);
  IAMFFileReader reader(
      kReferenceFilePath,
      {
          .requested_mix =
              {.output_layout =
                   iamf_tools::api::OutputLayout::kItu2051_SoundSystemB_0_5_0},
      });

  const IAMFFileReader::StreamData kSData = reader.getStreamData();
  EXPECT_TRUE(kSData.valid);
  EXPECT_EQ(kSData.numChannels, Speakers::k5Point1.getNumChannels());
  EXPECT_EQ(kSData.sampleRate, 16e3);
  EXPECT_EQ(kSData.frameSize, kSamplesPerFrame);

  size_t totalFramesRead = 0, samplesRead = 0;
  juce::AudioBuffer<float> buffer(kSData.numChannels, kSData.frameSize);
  while ((samplesRead = reader.readFrame(buffer)) > 0) {
    ASSERT_EQ(samplesRead, (size_t)kSData.frameSize);

    // Decoded samples should match the written 660Hz sine wave
    for (int i = 0; i < kSData.numChannels; ++i) {
      for (int j = 0; j < samplesRead; ++j) {
        ASSERT_NEAR(buffer.getSample(i, j),
                    sampleSine(660.f, totalFramesRead * kSData.frameSize + j,
                               kSData.sampleRate),
                    .0001f);
      }
    }
    ++totalFramesRead;
  }
}

// In a file with multiple mix presentations, decodes the mix presentation given
// the requested ID
TEST_F(IAMFFileReaderTest, multi_mix_2) {
  createIAMFFile2AE2MP(kReferenceFilePath);
  IAMFFileReader reader(kReferenceFilePath,
                        {
                            .requested_mix = {.mix_presentation_id = 0},
                        });

  const IAMFFileReader::StreamData kSData = reader.getStreamData();
  EXPECT_TRUE(kSData.valid);
  EXPECT_EQ(kSData.numChannels, Speakers::kStereo.getNumChannels());
  EXPECT_EQ(kSData.sampleRate, 16e3);
  EXPECT_EQ(kSData.frameSize, kSamplesPerFrame);

  size_t totalFramesRead = 0, samplesRead = 0;
  juce::AudioBuffer<float> buffer(kSData.numChannels, kSData.frameSize);
  while ((samplesRead = reader.readFrame(buffer)) > 0) {
    ASSERT_EQ(samplesRead, (size_t)kSData.frameSize);

    // Decoded samples should match the written 440Hz sine wave
    for (int i = 0; i < kSData.numChannels; ++i) {
      for (int j = 0; j < samplesRead; ++j) {
        ASSERT_NEAR(buffer.getSample(i, j),
                    sampleSine(440.f, totalFramesRead * kSData.frameSize + j,
                               kSData.sampleRate),
                    .0001f);
      }
    }
    ++totalFramesRead;
  }
}

// Construct a reader for a file with multiple mix presentations. Destroy the
// reader and construct one for the same file with a different layout
TEST_F(IAMFFileReaderTest, swap_mix) {
  createIAMFFile2AE2MP(kReferenceFilePath);
  std::unique_ptr<IAMFFileReader> reader =
      std::make_unique<IAMFFileReader>(kReferenceFilePath);

  const IAMFFileReader::StreamData kSData = reader->getStreamData();
  EXPECT_TRUE(kSData.valid);
  EXPECT_EQ(kSData.numChannels, Speakers::kStereo.getNumChannels());
  EXPECT_EQ(kSData.sampleRate, 16e3);
  EXPECT_EQ(kSData.frameSize, kSamplesPerFrame);

  const IAMFFileReader::Settings kSettings = {
      .requested_mix = {
          .output_layout =
              iamf_tools::api::OutputLayout::kItu2051_SoundSystemB_0_5_0}};
  reader = std::make_unique<IAMFFileReader>(kReferenceFilePath, kSettings);

  const IAMFFileReader::StreamData kSData2 = reader->getStreamData();
  EXPECT_TRUE(kSData2.valid);
  EXPECT_EQ(kSData2.numChannels, Speakers::k5Point1.getNumChannels());
  EXPECT_EQ(kSData2.sampleRate, 16e3);
  EXPECT_EQ(kSData2.frameSize, kSamplesPerFrame);
}

// Construct the reader for one given mix of a given file. Destroy the
// reader partway through the file. Recreate while requesting a different
// mix.
TEST_F(IAMFFileReaderTest, swap_reset_mix) {
  createIAMFFile2AE2MP(kReferenceFilePath);
  std::unique_ptr<IAMFFileReader> reader =
      std::make_unique<IAMFFileReader>(kReferenceFilePath);

  const IAMFFileReader::StreamData kSData = reader->getStreamData();
  EXPECT_TRUE(kSData.valid);
  EXPECT_EQ(kSData.numChannels, Speakers::kStereo.getNumChannels());
  EXPECT_EQ(kSData.sampleRate, 16e3);
  EXPECT_EQ(kSData.frameSize, kSamplesPerFrame);

  juce::AudioBuffer<float> buffer(kSData.numChannels, kSData.frameSize);
  reader->readFrame(buffer);

  const IAMFFileReader::Settings kSettings = {
      .requested_mix = {
          .output_layout =
              iamf_tools::api::OutputLayout::kItu2051_SoundSystemB_0_5_0}};
  reader = std::make_unique<IAMFFileReader>(kReferenceFilePath, kSettings);

  const IAMFFileReader::StreamData kSData2 = reader->getStreamData();
  EXPECT_TRUE(kSData2.valid);
  EXPECT_EQ(kSData2.numChannels, Speakers::k5Point1.getNumChannels());
  EXPECT_EQ(kSData2.sampleRate, 16e3);
  EXPECT_EQ(kSData2.frameSize, kSamplesPerFrame);
}

// Seek to a valid frame in the file forwards from the current frame
TEST_F(IAMFFileReaderTest, seek_valid) {
  createBasicIAMFFile(kReferenceFilePath);
  IAMFFileReader reader(kReferenceFilePath);

  const IAMFFileReader::StreamData kSData = reader.getStreamData();
  EXPECT_TRUE(kSData.valid);
  EXPECT_EQ(kSData.numChannels, 2);
  EXPECT_EQ(kSData.sampleRate, 48e3);
  EXPECT_EQ(kSData.frameSize, kSamplesPerFrame);

  size_t samplesRead = 0;
  juce::AudioBuffer<float> buffer(kSData.numChannels, kSData.frameSize);
  WavFileWriter debugWriter(kDebugOutPath, kSData.sampleRate,
                            kSData.numChannels);

  const unsigned kFramesToSeek = 4;
  EXPECT_TRUE(reader.seekFrame(kFramesToSeek));
  samplesRead = reader.readFrame(buffer);
  EXPECT_EQ(samplesRead, (size_t)kSData.frameSize);
  debugWriter.write(buffer, samplesRead);

  // Decoded samples should match the written 440Hz sine wave
  for (int i = 0; i < kSData.numChannels; ++i) {
    for (int j = 0; j < samplesRead; ++j) {
      ASSERT_NEAR(buffer.getSample(i, j),
                  sampleSine(440.f, kFramesToSeek * kSData.frameSize + j,
                             kSData.sampleRate),
                  .0001f);
    }
  }
}

// // Seek to a valid frame in the file backwards from the current frame
// TEST_F(IAMFFileReaderTest, seek_valid_backwards) {
//   createBasicIAMFFile(kReferenceFilePath);
//   IAMFFileReader reader(kReferenceFilePath);
//   const IAMFFileReader::StreamData kSData = reader.getStreamData();
//   EXPECT_TRUE(kSData.valid);
//   EXPECT_EQ(kSData.numChannels, 2);
//   EXPECT_EQ(kSData.sampleRate, 48e3);
//   EXPECT_EQ(kSData.frameSize, kSamplesPerFrame);
//   size_t totalFramesRead = 0, samplesRead = 0;
//   juce::AudioBuffer<float> buffer(kSData.numChannels, kSData.frameSize);
//   // Read first 10 frames
//   for (int i = 0; i < 10; ++i) {
//     samplesRead = reader.readFrame(buffer);
//     ASSERT_EQ(samplesRead, (size_t)kSData.frameSize);
//     ++totalFramesRead;
//   }
//   // Seek back to frame 5
//   ASSERT_TRUE(reader.seekFrame(5));
//   // Read frame 5
//   samplesRead = reader.readFrame(buffer);
//   ASSERT_EQ(samplesRead, (size_t)kSData.frameSize);
//   ++totalFramesRead;
//   // Decoded samples should match the written 440Hz sine wave
//   for (int i = 0; i < kSData.numChannels; ++i) {
//     for (int j = 0; j < samplesRead; ++j) {
//       ASSERT_NEAR(
//           buffer.getSample(i, j),
//           sampleSine(440.f, 5 * kSData.frameSize + j, kSData.sampleRate),
//           .0001f);
//     }
//   }
// }

// // Seek to an invalid frame past the end of the file
// TEST_F(IAMFFileReaderTest, seek_invalid) {
//   createBasicIAMFFile(kReferenceFilePath);
//   IAMFFileReader reader(kReferenceFilePath);
//   const IAMFFileReader::StreamData kSData = reader.getStreamData();
//   EXPECT_TRUE(kSData.valid);
//   EXPECT_EQ(kSData.numChannels, 2);
//   EXPECT_EQ(kSData.sampleRate, 48e3);
//   EXPECT_EQ(kSData.frameSize, kSamplesPerFrame);
//   size_t totalFramesRead = 0, samplesRead = 0;
//   juce::AudioBuffer<float> buffer(kSData.numChannels, kSData.frameSize);
//   // Read first 10 frames
//   for (int i = 0; i < 10; ++i) {
//     samplesRead = reader.readFrame(buffer);
//     ASSERT_EQ(samplesRead, (size_t)kSData.frameSize);
//     ++totalFramesRead;
//   }
//   // Seek to invalid frame index
//   ASSERT_FALSE(reader.seekFrame(1000));
// }
