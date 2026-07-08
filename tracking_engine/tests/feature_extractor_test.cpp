#include "feature_extractor.h"
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
        0.6 * std::sin(2.0 * kPi * fundamental_hz * time) +
        0.3 * std::sin(2.0 * kPi * fundamental_hz * 2.0 * time) +
        0.15 * std::sin(2.0 * kPi * fundamental_hz * 3.0 * time);
    samples[i] = static_cast<float>(sample);
  }

  return samples;
}

}  // namespace

int main() {
  constexpr int kSampleRate = 48000;
  constexpr std::size_t kFftSize = 1024;
  constexpr double kPitchHz = 375.0;

  const std::vector<float> samples =
      HarmonicTone(kPitchHz, kSampleRate, kFftSize);
  const auto spectrum = tunerlock::fft::DiscreteFourierTransform(samples);
  const std::vector<double> magnitudes = tunerlock::fft::Magnitudes(spectrum);
  const tunerlock::feature::HarmonicScore harmonic_score =
      tunerlock::feature::ComputeHarmonicSumSpectrum(
          magnitudes,
          kSampleRate,
          kFftSize,
          kPitchHz);

  const tunerlock::feature::FeatureVector features =
      tunerlock::feature::ExtractFeatures(
          tunerlock::feature::FeatureInput{
              samples.data(),
              samples.size(),
              kSampleRate,
              &magnitudes,
              kFftSize,
              kPitchHz,
              harmonic_score,
          });

  assert(features.rms > 0.0);
  assert(features.peak_amplitude > 0.0);
  assert(features.yin_detected);
  assert(features.yin_pitch_hz == kPitchHz);
  assert(features.fft_peak_frequency_hz > 0.0);
  assert(features.fft_peak_magnitude > 0.0);
  assert(features.spectral_centroid_hz > 0.0);
  assert(features.harmonic_count >= 3);
  assert(features.harmonic_normalized_score > 0.0);
  assert(features.harmonic_energy_ratio > 0.0);

  const tunerlock::feature::FeatureVector empty_features =
      tunerlock::feature::ExtractFeatures(tunerlock::feature::FeatureInput{});
  assert(empty_features.rms == 0.0);
  assert(!empty_features.yin_detected);

  return 0;
}
