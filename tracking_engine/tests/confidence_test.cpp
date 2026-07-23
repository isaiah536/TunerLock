#include "confidence.h"

#include <cassert>

namespace {

tunerlock::tracking::ConfidenceInput StableInput(double pitch_hz) {
  tunerlock::tracking::ConfidenceInput input;
  input.yin.frequency_hz = pitch_hz;
  input.yin.periodicity = 0.95;
  input.yin.cmndf_value = 0.05;
  input.yin.detected = true;
  input.features.yin_pitch_hz = pitch_hz;
  input.features.yin_detected = true;
  input.features.rms = 0.12;
  input.features.harmonic_energy_ratio = 0.30;
  input.features.harmonic_normalized_score = 0.40;
  input.locked_target_energy_ratio = 0.40;
  input.tracking_state = tunerlock::TrackingState::Locked;
  return input;
}

}  // namespace

int main() {
  tunerlock::tracking::ConfidenceCalculator calculator(
      tunerlock::tracking::InstrumentProfile::Strings);

  const auto first = calculator.Process(StableInput(440.0));
  auto stable = first;
  for (int frame = 0; frame < 10; ++frame) {
    stable = calculator.Process(StableInput(440.1));
  }
  assert(stable.value > first.value);
  assert(stable.value > 0.80);
  assert(stable.pitch_stability > 0.9);
  assert(stable.persistence_score == 1.0);

  auto unstable_input = StableInput(493.88);
  unstable_input.yin.periodicity = 0.40;
  unstable_input.features.harmonic_energy_ratio = 0.02;
  unstable_input.features.harmonic_normalized_score = 0.02;
  unstable_input.locked_target_energy_ratio = 0.01;
  const auto unstable = calculator.Process(unstable_input);
  assert(unstable.value < stable.value);
  assert(unstable.pitch_stability == 0.0);

  auto missed_input = StableInput(440.0);
  missed_input.yin = {};
  const auto missed = calculator.Process(missed_input);
  assert(missed.value > 0.0);
  assert(missed.value < unstable.value);

  auto lost_input = unstable_input;
  lost_input.yin = {};
  lost_input.tracking_state = tunerlock::TrackingState::Lost;
  const auto lost = calculator.Process(lost_input);
  assert(lost.value == 0.0);

  return 0;
}
