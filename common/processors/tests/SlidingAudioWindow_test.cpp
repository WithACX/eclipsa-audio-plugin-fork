#include "../file_output/iamf_export_utils/SlidingAudioWindow.h"

#include <gtest/gtest.h>

TEST(SlidingAudioWindow, WriteAndReadNoWrap) {
  // Create a window with 4 samples of padding in both directions - 8 samples
  // total.
  const unsigned kNumChannels = 1;
  const unsigned kNumPadSamples = 4;
  SlidingAudioWindow window(kNumChannels, kNumPadSamples);
  EXPECT_EQ(window.capacity(), kNumPadSamples * 2);

  // Create input buffer with known values
  juce::AudioBuffer<float> inputBuffer(kNumChannels, kNumPadSamples);
  for (unsigned i = 0; i < kNumPadSamples; ++i) {
    inputBuffer.setSample(0, i, i);
  }
  EXPECT_TRUE(window.writeSamples(kNumPadSamples, inputBuffer));

  // Read back the samples
  juce::AudioBuffer<float> outputBuffer(kNumChannels, kNumPadSamples);
  EXPECT_EQ(window.readSamples(0, kNumPadSamples, outputBuffer),
            kNumPadSamples);
  for (unsigned i = 0; i < kNumPadSamples; ++i) {
    ASSERT_FLOAT_EQ(outputBuffer.getSample(0, i), inputBuffer.getSample(0, i));
  }
}

// Test writing the full buffer, then reading samples.
TEST(SlidingAudioWindow, WriteAndReadFullBuffer) {
  const unsigned kNumChannels = 1;
  const unsigned kNumPadSamples = 4;
  SlidingAudioWindow window(kNumChannels, kNumPadSamples);

  // Create input buffer with known values
  juce::AudioBuffer<float> inputBuffer(kNumChannels, kNumPadSamples * 2);
  for (unsigned i = 0; i < kNumPadSamples * 2; ++i) {
    inputBuffer.setSample(0, i, i);
  }
  EXPECT_TRUE(window.writeSamples(kNumPadSamples * 2, inputBuffer));

  // Read back the samples
  juce::AudioBuffer<float> outputBuffer(kNumChannels, kNumPadSamples * 2);
  EXPECT_EQ(window.readSamples(0, kNumPadSamples * 2, outputBuffer),
            kNumPadSamples * 2);
  for (unsigned i = 0; i < kNumPadSamples * 2; ++i) {
    ASSERT_FLOAT_EQ(outputBuffer.getSample(0, i), inputBuffer.getSample(0, i));
  }
}

// Test writing the full buffer, reading 1 past the halfway point, then check
// there's available samples to be written. Then write available samples, and
// read the right half of the buffer back.
