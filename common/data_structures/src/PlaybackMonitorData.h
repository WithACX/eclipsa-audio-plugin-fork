#pragma once
#include <processors/mix_monitoring/loudness_standards/MeasureEBU128.h>

// #include "RealtimeDataType.h"

struct PlaybackMonitorData {
  PlaybackMonitorData() : currentPositionInSeconds(0), totalFileLength(0) {}

  std::atomic_int currentPositionInSeconds;
  std::atomic_int totalFileLength;
};