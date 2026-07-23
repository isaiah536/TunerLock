#include "kalman.h"

#include <cassert>
#include <cmath>

int main() {
  tunerlock::filter::PitchKalmanFilter filter;

  assert(filter.Process(440.0, 0.95) == 440.0);
  const double noisy = filter.Process(446.0, 0.20);
  assert(noisy > 440.0);
  assert(noisy < 446.0);

  double stable = noisy;
  for (int frame = 0; frame < 20; ++frame) {
    stable = filter.Process(442.0, 0.95);
  }
  assert(std::fabs(stable - 442.0) < 0.5);

  const double held = filter.Process(0.0, 0.0);
  assert(std::fabs(held - stable) < 0.0001);

  filter.Reset();
  assert(!filter.initialized());
  return 0;
}
