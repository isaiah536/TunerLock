#pragma once

#include "feature_extractor.h"
#include "instrument_profile.h"

namespace tunerlock::tracking {

enum class AttackState {
  Idle,
  Candidate,
  Confirmed,
  Cooldown,
};

struct AttackDetectorConfig {
  InstrumentProfile profile = InstrumentProfile::Strings;
  double minimum_rms = 0.01;
  double rms_rise_ratio = 1.8;
  double minimum_rms_rise = 0.008;
  double peak_rise_ratio = 1.5;
  double centroid_rise_hz = 150.0;
  double centroid_rise_ratio = 1.15;

  int confirmation_window_frames = 4;
  int minimum_confirmation_frames = 2;
  int required_stable_pitch_frames = 2;
  double maximum_pitch_deviation_cents = 35.0;
  double minimum_sustained_rms_ratio = 0.35;
  double minimum_harmonic_energy_ratio = 0.08;
  double minimum_harmonic_recovery = 0.03;
  int cooldown_frames = 3;
};

struct AttackResult {
  AttackState state = AttackState::Idle;
  bool candidate_started = false;
  bool confirmed = false;
  bool rejected = false;
  int candidate_age_frames = 0;
};

class AttackDetector {
 public:
  explicit AttackDetector(AttackDetectorConfig config = {});
  explicit AttackDetector(InstrumentProfile profile);

  AttackResult Process(const feature::FeatureVector& features);
  void Reset();

 private:
  bool IsAttackCandidate(const feature::FeatureVector& current) const;
  bool IsPitchStable(double pitch_hz) const;
  void BeginCandidate(const feature::FeatureVector& features);
  void ClearCandidate();

  AttackDetectorConfig config_;
  AttackState state_ = AttackState::Idle;
  feature::FeatureVector previous_;
  bool has_previous_ = false;
  int candidate_age_frames_ = 0;
  int stable_pitch_frames_ = 0;
  int cooldown_frames_remaining_ = 0;
  double candidate_rms_ = 0.0;
  double candidate_harmonic_ratio_ = 0.0;
  double last_pitch_hz_ = 0.0;
};

AttackDetectorConfig AttackConfigForProfile(InstrumentProfile profile);

}  // namespace tunerlock::tracking
