#include "window.h"

#include <cassert>
#include <cmath>
#include <vector>

namespace {

bool Near(float lhs, float rhs, float tolerance = 0.0001F) {
  return std::fabs(lhs - rhs) <= tolerance;
}

}  // namespace

int main() {
  const std::vector<float> hann = tunerlock::audio::HannWindow(5);
  assert(hann.size() == 5);
  assert(Near(hann[0], 0.0F));
  assert(Near(hann[1], 0.5F));
  assert(Near(hann[2], 1.0F));
  assert(Near(hann[3], 0.5F));
  assert(Near(hann[4], 0.0F));

  const std::vector<float> single = tunerlock::audio::HannWindow(1);
  assert(single.size() == 1);
  assert(Near(single[0], 1.0F));

  std::vector<float> samples = {1.0F, 2.0F, 3.0F, 4.0F, 5.0F};
  tunerlock::audio::ApplyWindowInPlace(samples, hann);
  assert(Near(samples[0], 0.0F));
  assert(Near(samples[1], 1.0F));
  assert(Near(samples[2], 3.0F));
  assert(Near(samples[3], 2.0F));
  assert(Near(samples[4], 0.0F));

  std::vector<float> shorter_samples = {2.0F, 4.0F};
  tunerlock::audio::ApplyWindowInPlace(shorter_samples, hann);
  assert(Near(shorter_samples[0], 0.0F));
  assert(Near(shorter_samples[1], 2.0F));

  return 0;
}
