#include "fft.h"

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
  constexpr std::size_t kFftSize = 1024;
  constexpr double kTargetFrequency = 937.5;

  const std::size_t target_bin =
      tunerlock::fft::FrequencyToBin(kTargetFrequency, kSampleRate, kFftSize);
  assert(target_bin == 20);

  const double bin_frequency =
      tunerlock::fft::BinToFrequency(target_bin, kSampleRate, kFftSize);
  assert(Near(bin_frequency, kTargetFrequency, 0.0001));

  const std::vector<float> samples =
      SineWave(kTargetFrequency, kSampleRate, kFftSize);
  const auto spectrum = tunerlock::fft::DiscreteFourierTransform(samples);
  const std::vector<double> magnitudes = tunerlock::fft::Magnitudes(spectrum);

  assert(spectrum.size() == kFftSize);
  assert(magnitudes.size() == kFftSize);

  const std::size_t peak_bin = tunerlock::fft::FindPeakBin(magnitudes);
  assert(peak_bin == target_bin || peak_bin == kFftSize - target_bin);
  assert(magnitudes[peak_bin] > magnitudes[0]);

  assert(tunerlock::fft::FindPeakBin({}) == 0);
  assert(tunerlock::fft::BinToFrequency(10, 0, kFftSize) == 0.0);
  assert(tunerlock::fft::FrequencyToBin(0.0, kSampleRate, kFftSize) == 0);

  return 0;
}
