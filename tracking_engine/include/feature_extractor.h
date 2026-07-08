#pragma once

#include "harmonic.h"

#include <cstddef>
#include <vector>

namespace tunerlock::feature {

struct FeatureInput {
  const float* samples = nullptr;
  std::size_t sample_count = 0;
  int sample_rate = 0;
  const std::vector<double>* magnitudes = nullptr;
  std::size_t fft_size = 0;
  double yin_pitch_hz = 0.0;
  HarmonicScore harmonic_score;
};

struct FeatureVector {
  double rms = 0.0;
  double peak_amplitude = 0.0;
  double yin_pitch_hz = 0.0;
  bool yin_detected = false;
  double fft_peak_frequency_hz = 0.0;
  double fft_peak_magnitude = 0.0;
  double spectral_centroid_hz = 0.0;
  double harmonic_weighted_sum = 0.0;
  double harmonic_normalized_score = 0.0;
  double harmonic_energy_ratio = 0.0;
  int harmonic_count = 0;
};

double ExtractSignalEnergy(const float* samples, int sample_count);

FeatureVector ExtractFeatures(const FeatureInput& input);

}  // namespace tunerlock::feature
