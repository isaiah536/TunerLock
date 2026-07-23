#include "attack_detector.h"

#include <cassert>

namespace {

tunerlock::feature::FeatureVector Frame(
    double rms,
    double peak,
    double centroid_hz,
    double pitch_hz,
    double harmonic_ratio) {
  tunerlock::feature::FeatureVector features;
  features.rms = rms;
  features.peak_amplitude = peak;
  features.spectral_centroid_hz = centroid_hz;
  features.yin_pitch_hz = pitch_hz;
  features.yin_detected = pitch_hz > 0.0;
  features.harmonic_energy_ratio = harmonic_ratio;
  return features;
}

void ConfirmsPitchedAttack() {
  tunerlock::tracking::AttackDetector detector;

  detector.Process(Frame(0.005, 0.02, 300.0, 0.0, 0.0));
  const auto candidate =
      detector.Process(Frame(0.08, 0.40, 900.0, 0.0, 0.01));
  assert(candidate.candidate_started);

  const auto checking =
      detector.Process(Frame(0.07, 0.35, 700.0, 439.0, 0.05));
  assert(checking.state == tunerlock::tracking::AttackState::Candidate);

  const auto confirmed =
      detector.Process(Frame(0.065, 0.32, 620.0, 439.3, 0.12));
  assert(confirmed.confirmed);
  assert(confirmed.state == tunerlock::tracking::AttackState::Confirmed);
}

void RejectsNoiseTransient() {
  tunerlock::tracking::AttackDetector detector;

  detector.Process(Frame(0.004, 0.02, 250.0, 0.0, 0.0));
  assert(detector
      .Process(Frame(0.12, 0.90, 1800.0, 0.0, 0.0))
      .candidate_started);
  detector.Process(Frame(0.07, 0.40, 1200.0, 0.0, 0.01));
  detector.Process(Frame(0.04, 0.20, 800.0, 0.0, 0.01));
  detector.Process(Frame(0.025, 0.10, 500.0, 0.0, 0.01));
  const auto rejected =
      detector.Process(Frame(0.015, 0.05, 350.0, 0.0, 0.0));

  assert(rejected.rejected);
  assert(!rejected.confirmed);
}

void DoesNotRetriggerSustainedTone() {
  tunerlock::tracking::AttackDetector detector;

  detector.Process(Frame(0.005, 0.02, 300.0, 0.0, 0.0));
  detector.Process(Frame(0.08, 0.40, 900.0, 440.0, 0.02));
  detector.Process(Frame(0.07, 0.35, 700.0, 440.1, 0.09));
  assert(detector
      .Process(Frame(0.065, 0.32, 620.0, 440.0, 0.12))
      .confirmed);

  for (int i = 0; i < 6; ++i) {
    const auto result =
        detector.Process(Frame(0.065, 0.32, 620.0, 440.0, 0.12));
    assert(!result.candidate_started);
    assert(!result.confirmed);
  }
}

void ProfilesUseDifferentOnsetLogic() {
  tunerlock::tracking::AttackDetector strings(
      tunerlock::tracking::InstrumentProfile::Strings);
  tunerlock::tracking::AttackDetector wind(
      tunerlock::tracking::InstrumentProfile::Wind);

  const auto quiet = Frame(0.005, 0.02, 400.0, 0.0, 0.0);
  const auto soft_bow = Frame(0.025, 0.05, 420.0, 0.0, 0.01);
  strings.Process(quiet);
  wind.Process(quiet);

  assert(strings.Process(soft_bow).candidate_started);
  assert(!wind.Process(soft_bow).candidate_started);

  tunerlock::tracking::AttackDetector wind_key_click(
      tunerlock::tracking::InstrumentProfile::Wind);
  wind_key_click.Process(quiet);
  const auto key_click = Frame(0.006, 0.30, 1800.0, 0.0, 0.0);
  assert(!wind_key_click.Process(key_click).candidate_started);
}

}  // namespace

int main() {
  ConfirmsPitchedAttack();
  RejectsNoiseTransient();
  DoesNotRetriggerSustainedTone();
  ProfilesUseDifferentOnsetLogic();
  return 0;
}
