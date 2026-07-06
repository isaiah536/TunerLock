#include "window.h"

#include <cmath>

namespace tunerlock::audio {

std::vector<float> HannWindow(std::size_t size) {
  std::vector<float> window(size, 1.0F);
  if (size <= 1) {
    return window;
  }

  constexpr double kPi = 3.14159265358979323846;
  const double denominator = static_cast<double>(size - 1);
  for (std::size_t i = 0; i < size; ++i) {
    const double phase = 2.0 * kPi * static_cast<double>(i) / denominator;
    window[i] = static_cast<float>(0.5 * (1.0 - std::cos(phase)));
  }

  return window;
}

void ApplyWindowInPlace(
    std::vector<float>& samples,
    const std::vector<float>& window) {
  const std::size_t count =
      samples.size() < window.size() ? samples.size() : window.size();
  for (std::size_t i = 0; i < count; ++i) {
    samples[i] *= window[i];
  }
}

}  // namespace tunerlock::audio
