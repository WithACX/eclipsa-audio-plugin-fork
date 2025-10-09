#include "../file_output/iamf_export_utils/IAMFBuffer.h"

#include <gtest/gtest.h>

#include <filesystem>

#include "processors/file_output/iamf_export_utils/IAMFFileReader.h"

void waitForData() {
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

const std::filesystem::path kReferenceFilePath =
    std::filesystem::current_path() / "test_reader.iamf";

// Test list:

// 1. Test creating and filling the buffer.
TEST(IAMFBuffer, fill) {
  IAMFFileReader decoder(kReferenceFilePath);
  IAMFBuffer buffer(1, decoder);

  waitForData();

  EXPECT_TRUE(buffer.isReady());
  EXPECT_TRUE(buffer.availableSamples() > 0);
}

// 2. Test filling the buffer then reading some samples.
TEST(IAMFBuffer, fill_read) {
  IAMFFileReader decoder(kReferenceFilePath);
  IAMFBuffer buffer(1, decoder);

  waitForData();

  EXPECT_TRUE(buffer.isReady());
  EXPECT_TRUE(buffer.availableSamples() > 0);

  juce::AudioBuffer<float> out(decoder.getStreamData().numChannels,
                               decoder.getStreamData().frameSize);
  EXPECT_EQ(buffer.readSamples(out, 0, out.getNumSamples()),
            out.getNumSamples());

  juce::AudioBuffer<float> out2(decoder.getStreamData().numChannels,
                                decoder.getStreamData().frameSize + 7);
  EXPECT_EQ(buffer.readSamples(out2, 0, out2.getNumSamples()),
            out2.getNumSamples());
}

// 3. Test filling the buffer, then seeking to a position ahead but in the
// buffer.
TEST(IAMFBuffer, fill_seek) {
  IAMFFileReader decoder(kReferenceFilePath);
  IAMFBuffer buffer(1, decoder);

  waitForData();

  EXPECT_TRUE(buffer.isReady());
  EXPECT_TRUE(buffer.availableSamples() > 0);

  juce::AudioBuffer<float> out(decoder.getStreamData().numChannels,
                               decoder.getStreamData().frameSize);
  EXPECT_EQ(buffer.readSamples(out, 0, out.getNumSamples()),
            out.getNumSamples());

  juce::AudioBuffer<float> out2(decoder.getStreamData().numChannels,
                                decoder.getStreamData().frameSize + 7);
  EXPECT_EQ(buffer.readSamples(out2, 0, out2.getNumSamples()),
            out2.getNumSamples());
}

// 4. Test filling the buffer, then seeking to a position behind but in the
// buffer.
// 5. Test filling the buffer, then seeking to a position ahead outside the
// buffer.
// 6. Test filling the buffer, then seeking to a position behind outside the
// buffer.