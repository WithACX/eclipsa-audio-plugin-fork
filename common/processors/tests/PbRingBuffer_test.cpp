#include "../file_output/iamf_export_utils/PbRingBuffer.h"

#include <gtest/gtest.h>

// Tests will be with windows of 4 samples for illustration purposes.
const unsigned kNumCh = 1;
const unsigned kSize = 4;
const unsigned kPad = kSize / 2;

TEST(PbRingBuffer, InitialState) {
  PbRingBuffer rb(kNumCh, kPad);
  EXPECT_EQ(rb.availReadSamples(), 0);
  EXPECT_EQ(rb.availWriteSamples(), kPad);
}

TEST(PbRingBuffer, WriteAndRead) {
  PbRingBuffer rb(kNumCh, kPad);
  juce::AudioBuffer<float> in(kNumCh, kSize);
  juce::AudioBuffer<float> out(kNumCh, kSize);

  // Fill input with test data
  for (int i = 0; i < kSize; ++i) {
    in.setSample(0, i, static_cast<float>(i + 1));
  }

  EXPECT_TRUE(rb.writeSamples(kPad, in));
  EXPECT_EQ(rb.availReadSamples(), kPad);

  size_t read = rb.readSamples(kPad, out);
  EXPECT_EQ(read, kPad);
  for (int i = 0; i < kPad; ++i) {
    ASSERT_FLOAT_EQ(out.getSample(0, i), static_cast<float>(i + 1));
  }
}

TEST(PbRingBuffer, WriteExceedsCapacity) {
  PbRingBuffer rb(kNumCh, kPad);
  juce::AudioBuffer<float> in(kNumCh, kSize);

  // Try to write more than availWriteSamples
  EXPECT_FALSE(rb.writeSamples(kPad + 1, in));
}

TEST(PbRingBuffer, ReadMoreThanAvailable) {
  PbRingBuffer rb(kNumCh, kPad);
  juce::AudioBuffer<float> in(kNumCh, kSize);
  juce::AudioBuffer<float> out(kNumCh, kSize);

  rb.writeSamples(kPad, in);
  size_t read = rb.readSamples(kPad + 10, out);
  EXPECT_EQ(read, kPad);
}

TEST(PbRingBuffer, WrapAroundWrite) {
  PbRingBuffer rb(kNumCh, kPad);
  juce::AudioBuffer<float> in(kNumCh, kSize);
  juce::AudioBuffer<float> out(kNumCh, kSize);

  // Write, read, write to test wrap-around
  for (int i = 0; i < kPad; ++i) in.setSample(0, i, 1.0f);
  rb.writeSamples(kPad, in);
  rb.readSamples(kPad, out);

  for (int i = 0; i < kPad; ++i) in.setSample(0, i, 2.0f);
  EXPECT_TRUE(rb.writeSamples(kPad, in));
  rb.readSamples(kPad, out);
  for (int i = 0; i < kPad; ++i) {
    EXPECT_FLOAT_EQ(out.getSample(0, i), 2.0f);
  }
}

TEST(PbRingBuffer, SeekForward) {
  PbRingBuffer rb(kNumCh, kPad);
  juce::AudioBuffer<float> in(kNumCh, kSize);
  juce::AudioBuffer<float> out(kNumCh, kSize);

  for (int i = 0; i < kPad; ++i) in.setSample(0, i, static_cast<float>(i));
  rb.writeSamples(kPad, in);

  rb.seek(1, true);
  EXPECT_EQ(rb.availReadSamples(), kPad - 1);
  rb.readSamples(1, out);
  EXPECT_FLOAT_EQ(out.getSample(0, 0), 1.0f);
}

TEST(PbRingBuffer, SeekBackward) {
  PbRingBuffer rb(kNumCh, kPad);
  juce::AudioBuffer<float> in(kNumCh, kSize);
  juce::AudioBuffer<float> out(kNumCh, kSize);

  for (int i = 0; i < kPad; ++i) in.setSample(0, i, static_cast<float>(i));
  rb.writeSamples(kPad, in);
  rb.readSamples(1, out);

  rb.seek(1, false);
  EXPECT_EQ(rb.availReadSamples(), kPad);
  rb.readSamples(1, out);
  EXPECT_FLOAT_EQ(out.getSample(0, 0), 0.0f);
}

// TEST(PbRingBuffer, MultiChannelWrite) {
//   const int kChannels = 2;
//   PbRingBuffer rb(kChannels, kPad);
//   juce::AudioBuffer<float> in(kChannels, kSize);
//   juce::AudioBuffer<float> out(kChannels, kSize);

//   for (int ch = 0; ch < kChannels; ++ch) {
//     for (int i = 0; i < kPad; ++i) {
//       in.setSample(ch, i, static_cast<float>(ch * 10 + i));
//     }
//   }

//   rb.writeSamples(kPad, in);
//   rb.readSamples(kPad, out);

//   for (int ch = 0; ch < kChannels; ++ch) {
//     for (int i = 0; i < kPad; ++i) {
//       EXPECT_FLOAT_EQ(out.getSample(ch, i), static_cast<float>(ch * 10 + i));
//     }
//   }
// }
