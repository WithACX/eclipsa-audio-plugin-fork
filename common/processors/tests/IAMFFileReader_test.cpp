#include "../file_output/iamf_export_utils/IAMFFileReader.h"

#include <gtest/gtest.h>
#include <juce_audio_formats/juce_audio_formats.h>

#include <filesystem>

#include "FileOutputTestFixture.h"
#include "iamf_tools_api_types.h"
#include "processors/tests/FileOutputTestUtils.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"

class IAMFFileReaderTest : public FileOutputTests {};

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

const std::filesystem::path kReferenceFilePath =
    std::filesystem::current_path() / "test_reader.iamf";

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
  EXPECT_EQ(kSData.numChannels, 6);
  EXPECT_EQ(kSData.sampleRate, 48e3);
  EXPECT_EQ(kSData.frameSize, kSamplesPerFrame);
}

// Construct the decoder with an output layout that matches the AE in the file
TEST_F(IAMFFileReaderTest, read_same) {
  createBasicIAMFFile(kReferenceFilePath);
  IAMFFileReader reader(kReferenceFilePath);
  const IAMFFileReader::StreamData kSData = reader.getStreamData();

  EXPECT_TRUE(kSData.valid);

  juce::AudioBuffer<float> buffer(kSData.numChannels, kSData.frameSize);
  size_t totalFramesRead = 0, samplesRead = 0;
  while ((samplesRead = reader.readFrame(buffer)) > 0) {
    ASSERT_EQ(samplesRead, (size_t)kSData.frameSize);
    compareAudioBuffers(buffer,
                        generateSineWave(440.0f, 48000, kSamplesPerFrame));
    ++totalFramesRead;
  }

  EXPECT_EQ(totalFramesRead, 8);  // We bounced 8 blocks of audio
}

// Decoder selects the correct mix given chosen output layout.
// Construct a file with 2 mix presentations each with a unique audio element.
// 1 mix has a stereo element, the other a surround element.
// Request a surround layout and expect to receive the mix with the surround
// element.
TEST_F(IAMFFileReaderTest, multi_mix) {
  createIAMFFile2AE2MP(kReferenceFilePath);
  IAMFFileReader reader(
      kReferenceFilePath,
      {// .requested_mix =
       //     {.output_layout =
       //          iamf_tools::api::OutputLayout::kItu2051_SoundSystemB_0_5_0},
       .requested_mix = {.mix_presentation_id = 1}});

  const IAMFFileReader::StreamData kSData = reader.getStreamData();
  EXPECT_TRUE(kSData.valid);
  EXPECT_EQ(kSData.numChannels, Speakers::k5Point1.getNumChannels());
  EXPECT_EQ(kSData.sampleRate, 16e3);
  EXPECT_EQ(kSData.frameSize, kSamplesPerFrame);
  WavFileWriter writer(
      (std::filesystem::current_path().parent_path() / "mm.wav").string(),
      kSData.numChannels, kSData.sampleRate);

  EXPECT_TRUE(kSData.valid);

  size_t totalFramesRead = 0, samplesRead = 0;
  juce::AudioBuffer<float> buffer(kSData.numChannels, kSData.frameSize);
  while ((samplesRead = reader.readFrame(buffer)) > 0) {
    ASSERT_EQ(samplesRead, (size_t)kSData.frameSize);

    // Compare the audio buffers
    // compareAudioBuffers(buffer, generateSineWave(660.0f, 48e3, samplesRead));
    writer.write(buffer, samplesRead);

    ++totalFramesRead;
  }

  // EXPECT_EQ(totalFramesRead, 8);  // We bounced 8 blocks of audio
}

/**
 * @brief Notes on test results with the API:
 * 1. When requested output layout x is wider than the AE in the file, the
 * decoder output width is x.numChannels but data is only on the channels of the
 * source AE.
 * 2.
 */
