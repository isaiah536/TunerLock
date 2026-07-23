#include "tracking_lock.h"

#include <utility>

namespace tunerlock::tracking {

TrackingLock::TrackingLock(TrackingLockConfig config)
    : config_(std::move(config)) {}

TrackingLockResult TrackingLock::Process(const TargetDecision& decision) {
  bool target_changed = false;

  switch (decision.type) {
    case TargetDecisionType::Acquired:
      state_ = TrackingState::Locked;
      frequency_hz_ = decision.target_frequency_hz;
      confidence_ = decision.confidence;
      recovering_frame_count_ = 0;
      target_changed = true;
      break;

    case TargetDecisionType::Transition:
      state_ = TrackingState::Transition;
      frequency_hz_ = decision.target_frequency_hz;
      confidence_ = decision.confidence;
      recovering_frame_count_ = 0;
      break;

    case TargetDecisionType::Switched:
      state_ = TrackingState::Locked;
      frequency_hz_ = decision.target_frequency_hz;
      confidence_ = decision.confidence;
      recovering_frame_count_ = 0;
      target_changed = true;
      break;

    case TargetDecisionType::Maintained:
    case TargetDecisionType::ChallengerRejected:
      state_ = TrackingState::Locked;
      frequency_hz_ = decision.target_frequency_hz;
      confidence_ = decision.confidence;
      recovering_frame_count_ = 0;
      break;

    case TargetDecisionType::Recovering:
      state_ = TrackingState::Recovering;
      frequency_hz_ = decision.target_frequency_hz;
      confidence_ = decision.confidence;
      ++recovering_frame_count_;
      break;

    case TargetDecisionType::None:
      if (decision.has_target) {
        state_ = TrackingState::Locked;
        frequency_hz_ = decision.target_frequency_hz;
        confidence_ = decision.confidence;
        recovering_frame_count_ = 0;
      } else if (state_ == TrackingState::Locked) {
        state_ = TrackingState::Recovering;
        recovering_frame_count_ = 1;
      } else if (state_ == TrackingState::Recovering) {
        ++recovering_frame_count_;
        if (recovering_frame_count_ > config_.recovering_frames) {
          state_ = TrackingState::Lost;
        }
      }
      break;

    case TargetDecisionType::Lost:
      state_ = TrackingState::Lost;
      frequency_hz_ = 0.0;
      confidence_ = 0.0;
      recovering_frame_count_ = 0;
      break;
  }

  return {
      state_,
      frequency_hz_,
      confidence_,
      state_ == TrackingState::Locked ||
          state_ == TrackingState::Transition,
      target_changed,
  };
}

void TrackingLock::Reset() {
  state_ = TrackingState::Idle;
  frequency_hz_ = 0.0;
  confidence_ = 0.0;
  recovering_frame_count_ = 0;
}

}  // namespace tunerlock::tracking
