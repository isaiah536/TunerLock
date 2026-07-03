#pragma once

#include "types.h"

namespace tunerlock {

class TrackingEngine {
 public:
  TrackingEngine() = default;

  TrackingResult ProcessFrame(const float* samples, int sample_count, int sample_rate);
};

}  // namespace tunerlock
