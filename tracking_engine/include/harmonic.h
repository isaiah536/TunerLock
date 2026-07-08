#pragma once

#include <cstddef>
#include <vector>

namespace tunerlock::feature {

struct HarmonicContribution {
  int harmonic_number = 0;
  double expected_frequency_hz = 0.0;
  std::size_t bin = 0;
  double magnitude = 0.0;
  double weight = 0.0;
  double weighted_magnitude = 0.0;
};

struct HarmonicScore {
  double fundamental_frequency_hz = 0.0;
  double weighted_sum = 0.0;
  double normalized_score = 0.0;
  double harmonic_energy_ratio = 0.0;
  int harmonic_count = 0;
  std::vector<HarmonicContribution> contributions;
};

struct HarmonicOptions {
  int max_harmonics = 8;
  int search_radius_bins = 1;
  double min_frequency_hz = 20.0;
  double max_frequency_hz = 5000.0;
};

double EstimateHarmonicity(double fundamental_energy, double total_energy);

HarmonicScore ComputeHarmonicSumSpectrum(
    const std::vector<double>& magnitudes,
    int sample_rate,
    std::size_t fft_size,
    double fundamental_frequency_hz,
    const HarmonicOptions& options = HarmonicOptions{});

}  // namespace tunerlock::feature
