#pragma once

#include "feature_extractor.h"
#include "instrument_profile.h"

namespace tunerlock::tracking {

enum class TargetDecisionType {
  None,
  Acquired,
  Maintained,
  Transition,
  Switched,
  ChallengerRejected,
  Recovering,
  Lost,
};

struct TargetObservation {
  feature::FeatureVector features;
  bool attack_confirmed = false;

  // FFT energy near the currently locked frequency divided by total energy.
  double locked_target_energy_ratio = 0.0;
  double locked_target_fundamental_ratio = 0.0;
  double locked_target_strongest_harmonic_ratio = 0.0;
  bool protect_locked_target = false;
};

struct TargetDecision {
  TargetDecisionType type = TargetDecisionType::None;
  double target_frequency_hz = 0.0;
  double confidence = 0.0;
  bool has_target = false;
};

struct TargetManagerConfig {
  InstrumentProfile profile = InstrumentProfile::Strings;
  int acquisition_frames = 3;
  int challenger_frames = 3;
  int lost_frames = 5;
  double same_target_cents = 50.0;
  double stable_challenger_cents = 35.0;
  double minimum_harmonic_ratio = 0.06;
  double minimum_harmonic_score = 0.05;
  double locked_target_present_ratio = 0.08;
  double locked_target_fundamental_present_ratio = 0.025;
  double locked_target_harmonic_present_ratio = 0.04;
  int locked_target_persistence_frames = 2;
  double switch_score_margin = 1.05;
  double frequency_smoothing = 0.25;
  bool allow_legato_transition = true;
  int legato_entry_frames = 2;
  bool allow_stable_pitch_transition = false;
  int stable_pitch_transition_frames = 2;
  double transition_departure_cents = 50.0;
  double transition_stable_cents = 25.0;
  double transition_rms_drop_ratio = 0.55;
  double transition_switch_score_margin = 0.85;
  bool transition_can_override_locked_target = false;
};

class TargetManager {
 public:
  explicit TargetManager(TargetManagerConfig config = {});
  explicit TargetManager(InstrumentProfile profile);
  TargetManager(InstrumentProfile profile, TrackingMode mode);

  TargetDecision Process(const TargetObservation& observation);
  void Reset();
  void Configure(TargetManagerConfig config);

  bool has_target() const;
  double target_frequency_hz() const;

 private:
  bool HasUsablePitch(const feature::FeatureVector& features) const;
  double CandidateScore(const feature::FeatureVector& features) const;
  bool IsNear(double first_hz, double second_hz, double cents) const;
  void StartAcquisition(double pitch_hz, double score);
  void StartChallenger(double pitch_hz, double score);
  void ClearAcquisition();
  void ClearChallenger();

  TargetManagerConfig config_;
  bool has_target_ = false;
  double target_frequency_hz_ = 0.0;
  double target_score_ = 0.0;
  int missing_target_frames_ = 0;
  int locked_target_present_frames_ = 0;

  double acquisition_frequency_hz_ = 0.0;
  double acquisition_score_ = 0.0;
  int acquisition_frame_count_ = 0;

  double challenger_frequency_hz_ = 0.0;
  double challenger_score_ = 0.0;
  int challenger_frame_count_ = 0;
  double previous_rms_ = 0.0;
};

TargetManagerConfig TargetConfigForProfile(InstrumentProfile profile);
TargetManagerConfig TargetConfigForProfile(
    InstrumentProfile profile,
    TrackingMode mode);

}  // namespace tunerlock::tracking
