#include "fft.h"
#include "harmonic.h"

#include <cassert>
#include <cmath>
#include <vector>

namespace {

std::vector<float> HarmonicTone(
    double fundamental_hz,
    int sample_rate,
    std::size_t sample_count) {
  constexpr double kPi = 3.14159265358979323846;
  std::vector<float> samples(sample_count);

  for (std::size_t i = 0; i < sample_count; ++i) {
    const double time =
        static_cast<double>(i) / static_cast<double>(sample_rate);
    const double sample =
        0.7 * std::sin(2.0 * kPi * fundamental_hz * time) +
        0.4 * std::sin(2.0 * kPi * fundamental_hz * 2.0 * time) +
        0.25 * std::sin(2.0 * kPi * fundamental_hz * 3.0 * time);
    samples[i] = static_cast<float>(sample);
  }

  return samples;
}

}  // namespace

int main() {
  constexpr int kSampleRate = 48000;
  constexpr std::size_t kFftSize = 1024;
  constexpr double kFundamental = 375.0;

  const std::vector<float> samples =
      HarmonicTone(kFundamental, kSampleRate, kFftSize);
  const auto spectrum = tunerlock::fft::DiscreteFourierTransform(samples);
  const std::vector<double> magnitudes = tunerlock::fft::Magnitudes(spectrum);

  const tunerlock::feature::HarmonicScore target_score =
      tunerlock::feature::ComputeHarmonicSumSpectrum(
          magnitudes,
          kSampleRate,
          kFftSize,
          kFundamental);
  const tunerlock::feature::HarmonicScore wrong_score =
      tunerlock::feature::ComputeHarmonicSumSpectrum(
          magnitudes,
          kSampleRate,
          kFftSize,
          520.0);

  assert(target_score.harmonic_count >= 3);
  assert(target_score.weighted_sum > wrong_score.weighted_sum);
  assert(target_score.normalized_score > wrong_score.normalized_score);
  assert(target_score.harmonic_energy_ratio > 0.0);
  assert(target_score.harmonic_energy_ratio <= 1.0);

  const tunerlock::feature::HarmonicScore invalid_score =
      tunerlock::feature::ComputeHarmonicSumSpectrum(
          {},
          kSampleRate,
          kFftSize,
          kFundamental);
  assert(invalid_score.harmonic_count == 0);
  assert(invalid_score.weighted_sum == 0.0);

  return 0;
}
