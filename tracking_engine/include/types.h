#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace tunerlock {

enum class TrackingState {
  Idle,
  Locking,
  Transition,
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

  bool empty() const {
    return samples.empty();
  }

  std::size_t sample_count() const {
    return samples.size();
  }
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
  double target_frequency_hz = 0.0;
  double cents = 0.0;
  double confidence = 0.0;
  int midi_note = -1;
  bool locked = false;
};

struct DebugMetric {
  std::string stage;
  std::string name;
  double value = 0.0;
};

struct DebugTrace {
  std::vector<DebugMetric> metrics;

  void Add(std::string stage, std::string name, double value) {
    metrics.push_back(DebugMetric{std::move(stage), std::move(name), value});
  }

  void Clear() {
    metrics.clear();
  }
};

}  // namespace tunerlock
