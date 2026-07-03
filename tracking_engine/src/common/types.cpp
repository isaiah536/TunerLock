#include "types.h"

namespace tunerlock {

bool IsLockedState(TrackingState state) {
  return state == TrackingState::Locked;
}

}  // namespace tunerlock
