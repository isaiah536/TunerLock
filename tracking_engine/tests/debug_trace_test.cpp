#include "tracking_engine.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace {

std::vector<float> SineWave(
    double frequency_hz,
    int sample_rate,
    std::size_t sample_count) {
  constexpr double kPi = 3.14159265358979323846;
  std::vector<float> samples(sample_count);
  for (std::size_t i = 0; i < sample_count; ++i) {
    const double phase =
        2.0 * kPi * frequency_hz * static_cast<double>(i) /
        static_cast<double>(sample_rate);
    samples[i] = static_cast<float>(std::sin(phase));
  }
  return samples;
}

bool HasMetric(
    const tunerlock::DebugTrace& trace,
    const std::string& stage,
    const std::string& name) {
  for (const auto& metric : trace.metrics) {
    if (metric.stage == stage && metric.name == name) {
      return true;
    }
  }
  return false;
}

double MetricValue(
    const tunerlock::DebugTrace& trace,
    const std::string& stage,
    const std::string& name) {
  for (const auto& metric : trace.metrics) {
    if (metric.stage == stage && metric.name == name) {
      return metric.value;
    }
  }
  return 0.0;
}

}  // namespace

int main() {
  constexpr int kSampleRate = 48000;
  const std::vector<float> samples = SineWave(440.0, kSampleRate, 4096);

  tunerlock::TrackingEngine engine;
  tunerlock::DebugTrace trace;
  tunerlock::TrackingResult result;
  for (int frame = 0; frame < 5; ++frame) {
    result = engine.ProcessFrameWithDebug(
        samples.data(),
        static_cast<int>(samples.size()),
        kSampleRate,
        &trace);
  }

  assert(result.locked);
  assert(std::fabs(result.frequency_hz - 440.0) < 2.0);
  assert(result.midi_note == 69);
  assert(std::fabs(result.target_frequency_hz - 440.0) < 0.01);
  assert(std::fabs(result.cents) < 5.0);
  assert(result.confidence > 0.0);
  assert(!trace.metrics.empty());
  for (const auto& metric : trace.metrics) {
    std::cout << metric.stage << "." << metric.name << "="
              << metric.value << '\n';
  }

  assert(HasMetric(trace, "input", "sample_count"));
  assert(HasMetric(trace, "audio_buffer", "rms_after"));
  assert(HasMetric(trace, "windowing", "window_size"));
  assert(HasMetric(trace, "fft", "peak_frequency_hz"));
  assert(HasMetric(trace, "yin", "pitch_hz"));
  assert(HasMetric(trace, "tracking", "locked_target_energy_ratio"));
  assert(HasMetric(trace, "tracking", "locked"));
  assert(HasMetric(trace, "output", "filtered_frequency_hz"));
  assert(HasMetric(trace, "output", "cents"));
  assert(MetricValue(trace, "tracking", "locked_target_energy_ratio") > 0.5);

  const double yin_pitch = MetricValue(trace, "yin", "pitch_hz");
  assert(std::fabs(yin_pitch - 440.0) < 2.0);

  engine.SetReferencePitchHz(442.0);
  assert(engine.reference_pitch_hz() == 442.0);
  engine.SetReferencePitchHz(-1.0);
  assert(engine.reference_pitch_hz() == 442.0);

  return 0;
}
