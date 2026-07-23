#include "target_manager.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace tunerlock::tracking {
namespace {

double PitchDistanceCents(double first_hz, double second_hz) {
  if (first_hz <= 0.0 || second_hz <= 0.0) {
    return INFINITY;
  }
  return std::fabs(1200.0 * std::log2(first_hz / second_hz));
}

double Blend(double previous, double current, double amount) {
  return previous + (current - previous) * amount;
}

}  // namespace

TargetManager::TargetManager(TargetManagerConfig config)
    : config_(std::move(config)) {}

TargetManager::TargetManager(InstrumentProfile profile)
    : TargetManager(TargetConfigForProfile(profile)) {}

TargetManager::TargetManager(InstrumentProfile profile, TrackingMode mode)
    : TargetManager(TargetConfigForProfile(profile, mode)) {}

TargetManagerConfig TargetConfigForProfile(InstrumentProfile profile) {
  TargetManagerConfig config;
  config.profile = profile;

  switch (profile) {
    case InstrumentProfile::Strings:
      config.acquisition_frames = 4;
      config.challenger_frames = 4;
      config.same_target_cents = 65.0;
      config.stable_challenger_cents = 55.0;
      config.minimum_harmonic_ratio = 0.05;
      config.minimum_harmonic_score = 0.04;
      config.locked_target_present_ratio = 0.07;
      config.locked_target_fundamental_present_ratio = 0.018;
      config.locked_target_harmonic_present_ratio = 0.035;
      config.locked_target_persistence_frames = 2;
      config.switch_score_margin = 1.0;
      config.legato_entry_frames = 2;
      break;

    case InstrumentProfile::Wind:
      config.acquisition_frames = 3;
      config.challenger_frames = 3;
      config.same_target_cents = 45.0;
      config.stable_challenger_cents = 30.0;
      config.minimum_harmonic_ratio = 0.04;
      config.minimum_harmonic_score = 0.04;
      config.locked_target_present_ratio = 0.08;
      config.locked_target_fundamental_present_ratio = 0.025;
      config.locked_target_harmonic_present_ratio = 0.04;
      config.locked_target_persistence_frames = 1;
      config.switch_score_margin = 1.05;
      config.legato_entry_frames = 3;
      break;

    case InstrumentProfile::Brass:
      config.acquisition_frames = 4;
      config.challenger_frames = 3;
      config.same_target_cents = 60.0;
      config.stable_challenger_cents = 50.0;
      config.minimum_harmonic_ratio = 0.10;
      config.minimum_harmonic_score = 0.08;
      config.locked_target_present_ratio = 0.10;
      config.locked_target_fundamental_present_ratio = 0.035;
      config.locked_target_harmonic_present_ratio = 0.06;
      config.locked_target_persistence_frames = 2;
      config.switch_score_margin = 1.05;
      config.legato_entry_frames = 2;
      break;
  }
  return config;
}

TargetManagerConfig TargetConfigForProfile(
    InstrumentProfile profile,
    TrackingMode mode) {
  TargetManagerConfig config = TargetConfigForProfile(profile);

  if (mode == TrackingMode::Tuning &&
      profile == InstrumentProfile::Brass) {
    config.challenger_frames = 2;
    config.stable_challenger_cents = 45.0;
    config.locked_target_persistence_frames = 1;
    config.switch_score_margin = 1.0;
    return config;
  }

  if (mode != TrackingMode::Performance) {
    return config;
  }

  switch (profile) {
    case InstrumentProfile::Strings:
      config.same_target_cents = 45.0;
      config.stable_challenger_cents = 35.0;
      config.challenger_frames = 3;
      config.switch_score_margin = 0.95;
      config.locked_target_persistence_frames = 1;
      config.allow_stable_pitch_transition = true;
      config.stable_pitch_transition_frames = 2;
      config.transition_departure_cents = 70.0;
      config.transition_stable_cents = 38.0;
      config.transition_rms_drop_ratio = 0.50;
      config.transition_switch_score_margin = 0.90;
      config.transition_can_override_locked_target = true;
      break;

    case InstrumentProfile::Wind:
      config.same_target_cents = 30.0;
      config.stable_challenger_cents = 25.0;
      config.challenger_frames = 2;
      config.switch_score_margin = 0.95;
      config.locked_target_persistence_frames = 1;
      config.legato_entry_frames = 1;
      config.allow_stable_pitch_transition = true;
      config.stable_pitch_transition_frames = 2;
      config.transition_departure_cents = 45.0;
      config.transition_stable_cents = 22.0;
      config.transition_rms_drop_ratio = 0.55;
      config.transition_switch_score_margin = 0.80;
      config.transition_can_override_locked_target = true;
      break;

    case InstrumentProfile::Brass:
      config.same_target_cents = 35.0;
      config.stable_challenger_cents = 30.0;
      config.challenger_frames = 2;
      config.switch_score_margin = 0.95;
      config.locked_target_persistence_frames = 1;
      config.legato_entry_frames = 1;
      break;
  }
  return config;
}

TargetDecision TargetManager::Process(
    const TargetObservation& observation) {
  const feature::FeatureVector& features = observation.features;
  const bool usable_pitch = HasUsablePitch(features);
  const double pitch_hz = features.yin_pitch_hz;
  const double score = usable_pitch ? CandidateScore(features) : 0.0;
  const double previous_rms = previous_rms_;
  previous_rms_ = features.rms;

  if (!has_target_) {
    if (!usable_pitch) {
      ClearAcquisition();
      return {};
    }

    if (acquisition_frame_count_ == 0 ||
        !IsNear(
            pitch_hz,
            acquisition_frequency_hz_,
            config_.stable_challenger_cents)) {
      StartAcquisition(pitch_hz, score);
    } else {
      ++acquisition_frame_count_;
      acquisition_frequency_hz_ = Blend(
          acquisition_frequency_hz_,
          pitch_hz,
          config_.frequency_smoothing);
      acquisition_score_ = Blend(acquisition_score_, score, 0.5);
    }

    if (acquisition_frame_count_ < config_.acquisition_frames) {
      return {};
    }

    has_target_ = true;
    target_frequency_hz_ = acquisition_frequency_hz_;
    target_score_ = acquisition_score_;
    missing_target_frames_ = 0;
    ClearAcquisition();
    return {
        TargetDecisionType::Acquired,
        target_frequency_hz_,
        target_score_,
        true,
    };
  }

  if (usable_pitch &&
      IsNear(pitch_hz, target_frequency_hz_, config_.same_target_cents)) {
    target_frequency_hz_ = Blend(
        target_frequency_hz_, pitch_hz, config_.frequency_smoothing);
    target_score_ = Blend(target_score_, score, 0.25);
    missing_target_frames_ = 0;
    ClearChallenger();
    return {
        TargetDecisionType::Maintained,
        target_frequency_hz_,
        target_score_,
        true,
    };
  }

  ++missing_target_frames_;
  const bool fundamental_supported =
      observation.locked_target_fundamental_ratio >=
      config_.locked_target_fundamental_present_ratio;
  const bool weak_fundamental_supported =
      observation.locked_target_fundamental_ratio >=
      config_.locked_target_fundamental_present_ratio * 0.35;
  const bool harmonic_structure_supported =
      weak_fundamental_supported &&
      (observation.locked_target_energy_ratio >=
           config_.locked_target_present_ratio ||
       observation.locked_target_strongest_harmonic_ratio >=
           config_.locked_target_harmonic_present_ratio);
  const bool old_target_present_now =
      fundamental_supported || harmonic_structure_supported;
  if (old_target_present_now) {
    locked_target_present_frames_ = std::min(
        locked_target_present_frames_ + 1,
        config_.locked_target_persistence_frames + 1);
  } else if (locked_target_present_frames_ > 0) {
    --locked_target_present_frames_;
  }
  const bool old_target_present =
      observation.protect_locked_target
          ? old_target_present_now ||
                locked_target_present_frames_ >=
                    config_.locked_target_persistence_frames
          : observation.locked_target_energy_ratio >=
                config_.locked_target_present_ratio;

  const bool legato_candidate =
      config_.allow_legato_transition &&
      !old_target_present &&
      missing_target_frames_ >= config_.legato_entry_frames;
  const bool pitch_departed_from_target =
      usable_pitch &&
      PitchDistanceCents(pitch_hz, target_frequency_hz_) >=
          config_.transition_departure_cents;
  const bool harmonic_maintained =
      features.harmonic_energy_ratio >= config_.minimum_harmonic_ratio ||
      features.harmonic_normalized_score >= config_.minimum_harmonic_score;
  const bool rms_not_dropping =
      previous_rms <= 0.0 ||
      features.rms >= previous_rms * config_.transition_rms_drop_ratio;
  const bool stable_pitch_transition_candidate =
      config_.allow_stable_pitch_transition &&
      pitch_departed_from_target &&
      harmonic_maintained &&
      rms_not_dropping;

  if (usable_pitch &&
      (observation.attack_confirmed ||
       (legato_candidate && challenger_frame_count_ == 0) ||
       (stable_pitch_transition_candidate && challenger_frame_count_ == 0))) {
    StartChallenger(pitch_hz, score);
  } else if (usable_pitch && challenger_frame_count_ > 0) {
    const double stable_cents =
        stable_pitch_transition_candidate
            ? config_.transition_stable_cents
            : config_.stable_challenger_cents;
    if (IsNear(
            pitch_hz,
            challenger_frequency_hz_,
            stable_cents)) {
      ++challenger_frame_count_;
      challenger_frequency_hz_ = Blend(
          challenger_frequency_hz_,
          pitch_hz,
          config_.frequency_smoothing);
      challenger_score_ = Blend(challenger_score_, score, 0.5);
    } else {
      ClearChallenger();
    }
  }

  if (stable_pitch_transition_candidate &&
      challenger_frame_count_ > 0 &&
      challenger_frame_count_ < config_.stable_pitch_transition_frames) {
    return {
        TargetDecisionType::Transition,
        target_frequency_hz_,
        target_score_,
        true,
    };
  }

  if (challenger_frame_count_ >= config_.challenger_frames) {
    const bool transition_ready =
        stable_pitch_transition_candidate &&
        challenger_frame_count_ >= config_.stable_pitch_transition_frames;
    const bool challenger_is_stronger =
        challenger_score_ >=
        target_score_ *
            (transition_ready
                 ? config_.transition_switch_score_margin
                 : config_.switch_score_margin);

    if ((!old_target_present ||
         (transition_ready &&
          config_.transition_can_override_locked_target)) &&
        (challenger_is_stronger ||
         missing_target_frames_ >= config_.challenger_frames ||
         transition_ready)) {
      target_frequency_hz_ = challenger_frequency_hz_;
      target_score_ = challenger_score_;
      missing_target_frames_ = 0;
      ClearChallenger();
      return {
          TargetDecisionType::Switched,
          target_frequency_hz_,
          target_score_,
          true,
      };
    }

    if (transition_ready) {
      return {
          TargetDecisionType::Transition,
          target_frequency_hz_,
          target_score_,
          true,
      };
    }

    if (old_target_present) {
      ClearChallenger();
      return {
          TargetDecisionType::ChallengerRejected,
          target_frequency_hz_,
          target_score_,
          true,
      };
    }
  }

  if (missing_target_frames_ >= config_.lost_frames &&
      challenger_frame_count_ == 0 && !old_target_present) {
    const double lost_frequency = target_frequency_hz_;
    Reset();
    return {
        TargetDecisionType::Lost,
        lost_frequency,
        0.0,
        false,
    };
  }

  if (!usable_pitch && !old_target_present &&
      challenger_frame_count_ == 0) {
    return {
        TargetDecisionType::Recovering,
        target_frequency_hz_,
        target_score_,
        true,
    };
  }

  return {
      TargetDecisionType::None,
      target_frequency_hz_,
      target_score_,
      true,
  };
}

void TargetManager::Reset() {
  has_target_ = false;
  target_frequency_hz_ = 0.0;
  target_score_ = 0.0;
  missing_target_frames_ = 0;
  locked_target_present_frames_ = 0;
  previous_rms_ = 0.0;
  ClearAcquisition();
  ClearChallenger();
}

void TargetManager::Configure(TargetManagerConfig config) {
  config_ = std::move(config);
  Reset();
}

bool TargetManager::has_target() const {
  return has_target_;
}

double TargetManager::target_frequency_hz() const {
  return target_frequency_hz_;
}

bool TargetManager::HasUsablePitch(
    const feature::FeatureVector& features) const {
  return features.yin_detected && features.yin_pitch_hz > 0.0 &&
      (features.harmonic_energy_ratio >= config_.minimum_harmonic_ratio ||
       features.harmonic_normalized_score >=
           config_.minimum_harmonic_score);
}

double TargetManager::CandidateScore(
    const feature::FeatureVector& features) const {
  const double harmonic = std::clamp(
      std::max(
          features.harmonic_energy_ratio,
          features.harmonic_normalized_score),
      0.0,
      1.0);
  const double level = std::clamp(features.rms * 4.0, 0.0, 1.0);
  return harmonic * 0.75 + level * 0.25;
}

bool TargetManager::IsNear(
    double first_hz,
    double second_hz,
    double cents) const {
  return PitchDistanceCents(first_hz, second_hz) <= cents;
}

void TargetManager::StartAcquisition(double pitch_hz, double score) {
  acquisition_frequency_hz_ = pitch_hz;
  acquisition_score_ = score;
  acquisition_frame_count_ = 1;
}

void TargetManager::StartChallenger(double pitch_hz, double score) {
  challenger_frequency_hz_ = pitch_hz;
  challenger_score_ = score;
  challenger_frame_count_ = 1;
}

void TargetManager::ClearAcquisition() {
  acquisition_frequency_hz_ = 0.0;
  acquisition_score_ = 0.0;
  acquisition_frame_count_ = 0;
}

void TargetManager::ClearChallenger() {
  challenger_frequency_hz_ = 0.0;
  challenger_score_ = 0.0;
  challenger_frame_count_ = 0;
}

}  // namespace tunerlock::tracking
