#include "tracking_engine.h"

namespace tunerlock {

TrackingResult TrackingEngine::ProcessFrame(
    const float* samples,
    int sample_count,
    int sample_rate) {
  (void)samples;
  (void)sample_count;
  (void)sample_rate;
  return TrackingResult{};
}

}  // namespace tunerlock
