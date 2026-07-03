#pragma once

namespace tunerlock {

struct TrackingResult {
  double frequency_hz = 0.0;
  double cents = 0.0;
  double confidence = 0.0;
  bool locked = false;
};

class TrackingEngine {
 public:
  TrackingEngine() = default;

  TrackingResult ProcessFrame(const float* samples, int sample_count, int sample_rate);
};

}  // namespace tunerlock
