#pragma once

#include <cstddef>
#include <vector>

namespace tunerlock {

enum class TrackingState {
  Idle,
  Locking,
  Locked,
  Recovering,
  Lost,
};

struct AudioFrame {
  const float* samples = nullptr;
  std::size_t sample_count = 0;
  int sample_rate = 0;
};

struct AudioBuffer {
  std::vector<float> samples;
  int sample_rate = 0;
};

struct PitchResult {
  float frequency_hz = 0.0F;
  float cents = 0.0F;
  float confidence = 0.0F;
  TrackingState state = TrackingState::Idle;
  float wave_offset = 0.0F;
};

struct TrackingResult {
  double frequency_hz = 0.0;
  double cents = 0.0;
  double confidence = 0.0;
  bool locked = false;
};

}  // namespace tunerlock
