#include "confidence.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace tunerlock::tracking {
namespace {

double PitchDistanceCents(double first_hz, double second_hz) {
  if (first_hz <= 0.0 || second_hz <= 0.0) {
    return 0.0;
  }
  return std::fabs(1200.0 * std::log2(first_hz / second_hz));
}

double StabilityRatio(double current, double previous) {
  if (current <= 0.0 || previous <= 0.0) {
    return 0.0;
  }
  return std::min(current, previous) / std::max(current, previous);
}

}  // namespace

ConfidenceCalculator::ConfidenceCalculator(ConfidenceConfig config)
    : config_(std::move(config)) {}

ConfidenceCalculator::ConfidenceCalculator(InstrumentProfile profile)
    : ConfidenceCalculator(ConfidenceConfigForProfile(profile)) {}

ConfidenceConfig ConfidenceConfigForProfile(InstrumentProfile profile) {
  ConfidenceConfig config;
  config.profile = profile;
  switch (profile) {
    case InstrumentProfile::Strings:
      config.yin_weight = 0.20;
      config.pitch_stability_weight = 0.15;
      config.harmonic_weight = 0.30;
      config.target_energy_weight = 0.15;
      config.rms_stability_weight = 0.05;
      config.persistence_weight = 0.15;
      config.maximum_stable_pitch_cents = 65.0;
      config.persistence_full_frames = 10;
      break;
    case InstrumentProfile::Wind:
      config.yin_weight = 0.30;
      config.pitch_stability_weight = 0.25;
      config.harmonic_weight = 0.20;
      config.target_energy_weight = 0.10;
      config.rms_stability_weight = 0.10;
      config.persistence_weight = 0.05;
      config.maximum_stable_pitch_cents = 35.0;
      break;
    case InstrumentProfile::Brass:
      config.yin_weight = 0.20;
      config.pitch_stability_weight = 0.15;
      config.harmonic_weight = 0.30;
      config.target_energy_weight = 0.25;
      config.rms_stability_weight = 0.05;
      config.persistence_weight = 0.05;
      config.maximum_stable_pitch_cents = 55.0;
      config.harmonic_full_scale = 0.40;
      break;
  }
  return config;
}

ConfidenceResult ConfidenceCalculator::Process(
    const ConfidenceInput& input) {
  ConfidenceResult result;

  if (input.tracking_state == TrackingState::Lost ||
      input.tracking_state == TrackingState::Idle ||
      !input.yin.detected) {
    if (input.tracking_state == TrackingState::Recovering ||
        (input.tracking_state == TrackingState::Locked &&
         !input.yin.detected)) {
      previous_confidence_ *= config_.recovering_decay;
      result.value = previous_confidence_;
      return result;
    }
    Reset();
    return result;
  }

  result.yin_score = std::clamp(input.yin.periodicity, 0.0, 1.0);

  if (previous_pitch_hz_ <= 0.0) {
    result.pitch_stability = 0.5;
    stable_frame_count_ = 1;
  } else {
    const double pitch_delta =
        PitchDistanceCents(input.yin.frequency_hz, previous_pitch_hz_);
    result.pitch_stability = std::clamp(
        1.0 - pitch_delta / config_.maximum_stable_pitch_cents,
        0.0,
        1.0);
    stable_frame_count_ =
        result.pitch_stability >= 0.5 ? stable_frame_count_ + 1 : 1;
  }

  const double harmonic_raw = std::max(
      input.features.harmonic_energy_ratio,
      input.features.harmonic_normalized_score);
  result.harmonic_score = std::clamp(
      harmonic_raw / config_.harmonic_full_scale, 0.0, 1.0);
  result.target_energy_score = std::clamp(
      input.locked_target_energy_ratio / config_.target_energy_full_scale,
      0.0,
      1.0);
  result.rms_stability =
      previous_rms_ > 0.0
          ? StabilityRatio(input.features.rms, previous_rms_)
          : 0.5;
  result.persistence_score = std::clamp(
      static_cast<double>(stable_frame_count_) /
          static_cast<double>(config_.persistence_full_frames),
      0.0,
      1.0);

  result.value =
      config_.yin_weight * result.yin_score +
      config_.pitch_stability_weight * result.pitch_stability +
      config_.harmonic_weight * result.harmonic_score +
      config_.target_energy_weight * result.target_energy_score +
      config_.rms_stability_weight * result.rms_stability +
      config_.persistence_weight * result.persistence_score;

  if (input.tracking_state == TrackingState::Locking) {
    result.value = std::min(result.value, config_.locking_cap);
  } else if (input.tracking_state == TrackingState::Recovering) {
    result.value =
        std::min(result.value, previous_confidence_ * config_.recovering_decay);
  }

  result.value = std::clamp(result.value, 0.0, 1.0);
  previous_pitch_hz_ = input.yin.frequency_hz;
  previous_rms_ = input.features.rms;
  previous_confidence_ = result.value;
  return result;
}

void ConfidenceCalculator::Reset() {
  previous_pitch_hz_ = 0.0;
  previous_rms_ = 0.0;
  previous_confidence_ = 0.0;
  stable_frame_count_ = 0;
}

}  // namespace tunerlock::tracking
