#include <gtest/gtest.h>

#include "../file_output/iamf_export_utils/AudioWindow.h"

// Tests will be with windows of 4 samples for illustration purposes.
const unsigned kNumCh = 1;
const unsigned kSize = 4;
const unsigned kPad = kSize / 2;

juce::AudioBuffer<float> in(kNumCh, kSize);
juce::AudioBuffer<float> out(kNumCh, kSize);

// 1. Partially fill the buffer and read all of it.
TEST(Window, partial) {
  AudioWindow win(kNumCh, kPad);
  // Write
  const unsigned kSamps = 3;
  for (int i = 0; i < kSamps; ++i) {
    in.setSample(0, i, i);
  }
  ASSERT_TRUE(win.writeSamples(kSamps, in));

  EXPECT_EQ(win.size(), kSamps);
  EXPECT_EQ(win.capacity(), kSize);
  EXPECT_EQ(win.getAvailableWriteSamples(), 1);

  // Read
  ASSERT_EQ(win.readSamples(0, kSamps, out), kSamps);
  for (int i = 0; i < kSamps; ++i) {
    ASSERT_EQ(out.getSample(0, i), i);
  }
  // Because pad == 2 and we read 3 samples (read through the padding), the
  // start pointer should have advanced in the buffer and there should now be 2
  // samples available to read in.
  EXPECT_EQ(win.size(), 2);
  EXPECT_EQ(win.getAvailableWriteSamples(), 2);
}

// 2. Fully fill the buffer and read all of it.
TEST(Window, full) {
  AudioWindow win(kNumCh, kPad);
  const unsigned kSamps = 4;
  for (int i = 0; i < kSamps; ++i) {
    in.setSample(0, i, i);
  }
  ASSERT_TRUE(win.writeSamples(kSamps, in));

  EXPECT_EQ(win.size(), kSamps);
  EXPECT_EQ(win.capacity(), 4);
  EXPECT_EQ(win.getAvailableWriteSamples(), 0);

  // Read
  ASSERT_EQ(win.readSamples(0, kSamps, out), kSamps);
  for (int i = 0; i < kSamps; ++i) {
    ASSERT_EQ(out.getSample(0, i), i);
  }

  EXPECT_EQ(win.size(), 2);
  EXPECT_EQ(win.getAvailableWriteSamples(), 2);
}

// 3. Fully fill the buffer, read some, refill some, read all.
TEST(Window, full_rwr) {
  AudioWindow win(kNumCh, kPad);
  const unsigned kSamps = 4;
  for (int i = 0; i < kSamps; ++i) {
    in.setSample(0, i, i);
  }
  ASSERT_TRUE(win.writeSamples(kSamps, in));

  EXPECT_EQ(win.size(), kSamps);
  EXPECT_EQ(win.capacity(), 4);
  EXPECT_EQ(win.getAvailableWriteSamples(), 0);

  // Read all
  ASSERT_EQ(win.readSamples(0, kSamps, out), kSamps);
  for (int i = 0; i < kSamps; ++i) {
    ASSERT_EQ(out.getSample(0, i), i);
  }

  EXPECT_EQ(win.size(), kPad);
  EXPECT_EQ(win.getAvailableWriteSamples(), kPad);

  // Refill.
  in.setSample(0, 0, 4);
  in.setSample(0, 1, 5);
  ASSERT_TRUE(win.writeSamples(kPad, in));

  // Read the new samples.
  ASSERT_EQ(win.readSamples(0, kPad, out), kPad);
  EXPECT_EQ(out.getSample(0, 0), 4);
  EXPECT_EQ(out.getSample(0, 1), 5);

  EXPECT_EQ(win.size(), kSize);
  EXPECT_EQ(win.getAvailableWriteSamples(), 0);
}

// 4. Set absolute positions ahead valid.
TEST(Window, pos_ahead) {}

// 5. Set absolute position behind valid.
TEST(Window, pos_behind) {}

// 6. Set absolute position ahead invalid.
TEST(Window, pos_ahead_ob) {}

// 7. Set absolute position behind invalid.
TEST(Window, pos_behind_ob) {}
