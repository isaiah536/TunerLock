#include "yin.h"

#include <cassert>
#include <cmath>
#include <vector>

namespace {

bool Near(double lhs, double rhs, double tolerance) {
  return std::fabs(lhs - rhs) <= tolerance;
}

std::vector<float> SineWave(
    double frequency_hz,
    int sample_rate,
    std::size_t sample_count) {
  constexpr double kPi = 3.14159265358979323846;
  std::vector<float> samples(sample_count);
  for (std::size_t i = 0; i < sample_count; ++i) {
    const double phase =
        2.0 * kPi * frequency_hz * static_cast<double>(i) /
        static_cast<double>(sample_rate);
    samples[i] = static_cast<float>(std::sin(phase));
  }
  return samples;
}

}  // namespace

int main() {
  constexpr int kSampleRate = 48000;
  constexpr std::size_t kSampleCount = 4096;

  const std::vector<float> a4 = SineWave(440.0, kSampleRate, kSampleCount);
  const double a4_pitch = tunerlock::pitch::EstimatePitchYin(
      a4.data(),
      static_cast<int>(a4.size()),
      kSampleRate);
  assert(Near(a4_pitch, 440.0, 2.0));

  const std::vector<float> a3 = SineWave(220.0, kSampleRate, kSampleCount);
  const double a3_pitch = tunerlock::pitch::EstimatePitchYin(
      a3.data(),
      static_cast<int>(a3.size()),
      kSampleRate);
  assert(Near(a3_pitch, 220.0, 2.0));

  const auto difference = tunerlock::pitch::DifferenceFunction(
      a4.data(),
      a4.size(),
      2,
      256);
  const auto cmndf =
      tunerlock::pitch::CumulativeMeanNormalizedDifference(difference);
  assert(cmndf.size() == difference.size());
  assert(Near(cmndf[0], 1.0, 0.0001));

  assert(tunerlock::pitch::EstimatePitchYin(nullptr, 0, kSampleRate) == 0.0);
  assert(tunerlock::pitch::EstimatePitchYin(a4.data(), 0, kSampleRate) == 0.0);
  assert(tunerlock::pitch::EstimatePitchYin(a4.data(), 16, 0) == 0.0);

  return 0;
}
