#include "../file_output/iamf_export_utils/SlidingAudioWindow.h"

#include <gtest/gtest.h>

TEST(SlidingAudioWindow, WriteAndReadNoWrap) {
  // Create a window with 4 samples of padding in both directions - 8 samples
  // total.
  const unsigned kNumChannels = 1;
  const unsigned kNumPadSamples = 4;
  SlidingAudioWindow window(kNumChannels, kNumPadSamples);
  // 1 extra sample to differentiate full vs empty buffer
  EXPECT_EQ(window.capacity(), kNumPadSamples * 2 + 1);
  EXPECT_EQ(window.getAvailableWriteSamples(), kNumPadSamples * 2);

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
TEST(SlidingAudioWindow, WriteReadWrapAround) {
  const unsigned kNumChannels = 1;
  const unsigned kNumPadSamples = 4;
  SlidingAudioWindow window(kNumChannels, kNumPadSamples);

  // Create input buffer with known values
  juce::AudioBuffer<float> inputBuffer(kNumChannels, kNumPadSamples * 2);
  for (unsigned i = 0; i < kNumPadSamples * 2; ++i) {
    inputBuffer.setSample(0, i, i);
  }
  EXPECT_TRUE(window.writeSamples(kNumPadSamples * 2, inputBuffer));

  // Read back just past the halfway point
  juce::AudioBuffer<float> outputBuffer(kNumChannels, kNumPadSamples + 1);
  EXPECT_EQ(window.readSamples(0, kNumPadSamples + 1, outputBuffer),
            kNumPadSamples + 1);
  for (unsigned i = 0; i < kNumPadSamples + 1; ++i) {
    ASSERT_FLOAT_EQ(outputBuffer.getSample(0, i), inputBuffer.getSample(0, i));
  }

  // There should be 1 available to write now
  const auto kAvailable = window.getAvailableWriteSamples();
  EXPECT_EQ(kAvailable, 1);

  // Write available samples (1)
  juce::AudioBuffer<float> inputBuffer2(kNumChannels, kAvailable);
  for (unsigned i = 0; i < kAvailable; ++i) {
    inputBuffer2.setSample(0, i, i + kNumPadSamples * 2);
  }
  EXPECT_TRUE(window.writeSamples(kAvailable, inputBuffer2));

  // Read back the next 4 samples
  juce::AudioBuffer<float> outputBuffer2(kNumChannels, kNumPadSamples);
  EXPECT_EQ(window.readSamples(0, kNumPadSamples, outputBuffer2),
            kNumPadSamples);
  for (unsigned i = 0; i < kNumPadSamples - 1; ++i) {
    ASSERT_FLOAT_EQ(outputBuffer2.getSample(0, i),
                    inputBuffer.getSample(0, i + kNumPadSamples + 1));
    std::cout << "Comparing " << outputBuffer2.getSample(0, i) << " to "
              << inputBuffer.getSample(0, i + kNumPadSamples + 1) << std::endl;
  }
  // Last sample should be the new sample we wrote (8)
  ASSERT_FLOAT_EQ(outputBuffer2.getSample(0, outputBuffer2.getNumSamples() - 1),
                  8);
}
