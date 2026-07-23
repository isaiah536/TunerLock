#pragma once

#include "target_manager.h"
#include "types.h"

namespace tunerlock::tracking {

struct TrackingLockConfig {
  int recovering_frames = 3;
};

struct TrackingLockResult {
  TrackingState state = TrackingState::Idle;
  double frequency_hz = 0.0;
  double confidence = 0.0;
  bool locked = false;
  bool target_changed = false;
};

class TrackingLock {
 public:
  explicit TrackingLock(TrackingLockConfig config = {});

  TrackingLockResult Process(const TargetDecision& decision);
  void Reset();

 private:
  TrackingLockConfig config_;
  TrackingState state_ = TrackingState::Idle;
  double frequency_hz_ = 0.0;
  double confidence_ = 0.0;
  int recovering_frame_count_ = 0;
};

}  // namespace tunerlock::tracking
