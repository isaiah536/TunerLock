#include "attack_detector.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace tunerlock::tracking {
namespace {

double SafeRatio(double current, double previous) {
  constexpr double kEpsilon = 1.0e-9;
  return current / std::max(previous, kEpsilon);
}

double PitchDistanceCents(double first_hz, double second_hz) {
  if (first_hz <= 0.0 || second_hz <= 0.0) {
    return INFINITY;
  }
  return std::fabs(1200.0 * std::log2(first_hz / second_hz));
}

}  // namespace

AttackDetector::AttackDetector(AttackDetectorConfig config)
    : config_(std::move(config)) {}

AttackDetector::AttackDetector(InstrumentProfile profile)
    : AttackDetector(AttackConfigForProfile(profile)) {}

AttackDetectorConfig AttackConfigForProfile(InstrumentProfile profile) {
  AttackDetectorConfig config;
  config.profile = profile;

  switch (profile) {
    case InstrumentProfile::Strings:
      config.rms_rise_ratio = 1.35;
      config.minimum_rms_rise = 0.005;
      config.peak_rise_ratio = 1.3;
      config.centroid_rise_hz = 80.0;
      config.centroid_rise_ratio = 1.05;
      config.confirmation_window_frames = 5;
      config.minimum_confirmation_frames = 3;
      config.required_stable_pitch_frames = 3;
      config.maximum_pitch_deviation_cents = 55.0;
      config.minimum_harmonic_energy_ratio = 0.05;
      break;

    case InstrumentProfile::Wind:
      config.rms_rise_ratio = 1.7;
      config.minimum_rms_rise = 0.008;
      config.peak_rise_ratio = 1.5;
      config.centroid_rise_hz = 180.0;
      config.centroid_rise_ratio = 1.15;
      config.confirmation_window_frames = 4;
      config.minimum_confirmation_frames = 2;
      config.required_stable_pitch_frames = 2;
      config.maximum_pitch_deviation_cents = 30.0;
      config.minimum_harmonic_energy_ratio = 0.05;
      break;

    case InstrumentProfile::Brass:
      config.rms_rise_ratio = 1.5;
      config.minimum_rms_rise = 0.007;
      config.peak_rise_ratio = 1.4;
      config.centroid_rise_hz = 100.0;
      config.centroid_rise_ratio = 1.08;
      config.confirmation_window_frames = 5;
      config.minimum_confirmation_frames = 3;
      config.required_stable_pitch_frames = 3;
      config.maximum_pitch_deviation_cents = 50.0;
      config.minimum_harmonic_energy_ratio = 0.10;
      config.minimum_harmonic_recovery = 0.04;
      break;
  }
  return config;
}

AttackResult AttackDetector::Process(
    const feature::FeatureVector& features) {
  AttackResult result;

  if (!has_previous_) {
    previous_ = features;
    has_previous_ = true;
    return result;
  }

  if (state_ == AttackState::Cooldown) {
    --cooldown_frames_remaining_;
    if (cooldown_frames_remaining_ <= 0) {
      state_ = AttackState::Idle;
    }
    result.state = state_;
    previous_ = features;
    return result;
  }

  if (state_ == AttackState::Idle && IsAttackCandidate(features)) {
    BeginCandidate(features);
    result.state = AttackState::Candidate;
    result.candidate_started = true;
    previous_ = features;
    return result;
  }

  if (state_ != AttackState::Candidate) {
    result.state = state_;
    previous_ = features;
    return result;
  }

  ++candidate_age_frames_;
  const bool rms_sustained =
      features.rms >= config_.minimum_rms &&
      features.rms >= candidate_rms_ * config_.minimum_sustained_rms_ratio;

  if (features.yin_detected && IsPitchStable(features.yin_pitch_hz)) {
    ++stable_pitch_frames_;
  } else if (features.yin_detected) {
    stable_pitch_frames_ = 1;
  } else {
    stable_pitch_frames_ = 0;
  }
  last_pitch_hz_ =
      features.yin_detected ? features.yin_pitch_hz : 0.0;

  const bool harmonic_confirmed =
      features.harmonic_energy_ratio >=
          config_.minimum_harmonic_energy_ratio ||
      features.harmonic_energy_ratio - candidate_harmonic_ratio_ >=
          config_.minimum_harmonic_recovery;
  const bool enough_time =
      candidate_age_frames_ >= config_.minimum_confirmation_frames;
  const bool pitch_confirmed =
      stable_pitch_frames_ >= config_.required_stable_pitch_frames;

  if (enough_time && rms_sustained && pitch_confirmed &&
      harmonic_confirmed) {
    result.state = AttackState::Confirmed;
    result.confirmed = true;
    result.candidate_age_frames = candidate_age_frames_;
    ClearCandidate();
    state_ = AttackState::Cooldown;
    cooldown_frames_remaining_ = config_.cooldown_frames;
    previous_ = features;
    return result;
  }

  if (features.rms < config_.minimum_rms ||
      candidate_age_frames_ >= config_.confirmation_window_frames) {
    result.state = AttackState::Idle;
    result.rejected = true;
    result.candidate_age_frames = candidate_age_frames_;
    ClearCandidate();
    state_ = AttackState::Idle;
    previous_ = features;
    return result;
  }

  result.state = AttackState::Candidate;
  result.candidate_age_frames = candidate_age_frames_;
  previous_ = features;
  return result;
}

void AttackDetector::Reset() {
  state_ = AttackState::Idle;
  previous_ = {};
  has_previous_ = false;
  ClearCandidate();
  cooldown_frames_remaining_ = 0;
}

bool AttackDetector::IsAttackCandidate(
    const feature::FeatureVector& current) const {
  const bool rms_rise =
      current.rms >= config_.minimum_rms &&
      current.rms - previous_.rms >= config_.minimum_rms_rise &&
      SafeRatio(current.rms, previous_.rms) >= config_.rms_rise_ratio;
  const bool peak_rise =
      current.peak_amplitude > previous_.peak_amplitude &&
      SafeRatio(current.peak_amplitude, previous_.peak_amplitude) >=
          config_.peak_rise_ratio;
  const bool centroid_rise =
      current.spectral_centroid_hz - previous_.spectral_centroid_hz >=
          config_.centroid_rise_hz &&
      SafeRatio(
          current.spectral_centroid_hz,
          previous_.spectral_centroid_hz) >= config_.centroid_rise_ratio;

  switch (config_.profile) {
    case InstrumentProfile::Strings:
      // Bowed attacks can have little high-frequency onset energy.
      return rms_rise || (peak_rise && centroid_rise);
    case InstrumentProfile::Wind:
      // Breath and key noise must not pass without a tonal level rise.
      return rms_rise && centroid_rise;
    case InstrumentProfile::Brass:
      // Tongued attacks are strong, while slurs are handled by TargetManager.
      return (rms_rise || peak_rise) && centroid_rise;
  }
  return false;
}

bool AttackDetector::IsPitchStable(double pitch_hz) const {
  return last_pitch_hz_ <= 0.0 ||
      PitchDistanceCents(pitch_hz, last_pitch_hz_) <=
          config_.maximum_pitch_deviation_cents;
}

void AttackDetector::BeginCandidate(
    const feature::FeatureVector& features) {
  state_ = AttackState::Candidate;
  candidate_age_frames_ = 0;
  stable_pitch_frames_ = features.yin_detected ? 1 : 0;
  candidate_rms_ = features.rms;
  candidate_harmonic_ratio_ = features.harmonic_energy_ratio;
  last_pitch_hz_ =
      features.yin_detected ? features.yin_pitch_hz : 0.0;
}

void AttackDetector::ClearCandidate() {
  candidate_age_frames_ = 0;
  stable_pitch_frames_ = 0;
  candidate_rms_ = 0.0;
  candidate_harmonic_ratio_ = 0.0;
  last_pitch_hz_ = 0.0;
}

}  // namespace tunerlock::tracking
