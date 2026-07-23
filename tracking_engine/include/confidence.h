#pragma once

#include "feature_extractor.h"
#include "instrument_profile.h"
#include "types.h"
#include "yin.h"

namespace tunerlock::tracking {

struct ConfidenceConfig {
  InstrumentProfile profile = InstrumentProfile::Strings;
  double yin_weight = 0.25;
  double pitch_stability_weight = 0.20;
  double harmonic_weight = 0.25;
  double target_energy_weight = 0.15;
  double rms_stability_weight = 0.10;
  double persistence_weight = 0.05;
  double maximum_stable_pitch_cents = 50.0;
  double harmonic_full_scale = 0.30;
  double target_energy_full_scale = 0.35;
  int persistence_full_frames = 8;
  double locking_cap = 0.70;
  double recovering_decay = 0.75;
};

struct ConfidenceInput {
  pitch::YinResult yin;
  feature::FeatureVector features;
  double locked_target_energy_ratio = 0.0;
  TrackingState tracking_state = TrackingState::Idle;
};

struct ConfidenceResult {
  double value = 0.0;
  double yin_score = 0.0;
  double pitch_stability = 0.0;
  double harmonic_score = 0.0;
  double target_energy_score = 0.0;
  double rms_stability = 0.0;
  double persistence_score = 0.0;
};

class ConfidenceCalculator {
 public:
  explicit ConfidenceCalculator(ConfidenceConfig config = {});
  explicit ConfidenceCalculator(InstrumentProfile profile);

  ConfidenceResult Process(const ConfidenceInput& input);
  void Reset();

 private:
  ConfidenceConfig config_;
  double previous_pitch_hz_ = 0.0;
  double previous_rms_ = 0.0;
  double previous_confidence_ = 0.0;
  int stable_frame_count_ = 0;
};

ConfidenceConfig ConfidenceConfigForProfile(InstrumentProfile profile);

}  // namespace tunerlock::tracking
