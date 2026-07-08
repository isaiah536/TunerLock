#include "harmonic.h"

#include "fft.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace tunerlock::feature {

double EstimateHarmonicity(double fundamental_energy, double total_energy) {
  return total_energy > 0.0 ? fundamental_energy / total_energy : 0.0;
}

namespace {

double HarmonicWeight(int harmonic_number) {
  if (harmonic_number <= 0) {
    return 0.0;
  }

  return 1.0 / static_cast<double>(harmonic_number);
}

std::size_t FindLocalPeakBin(
    const std::vector<double>& magnitudes,
    std::size_t center_bin,
    int radius_bins,
    std::size_t max_bin) {
  if (magnitudes.empty()) {
    return 0;
  }

  const std::size_t radius = static_cast<std::size_t>(
      std::max(0, radius_bins));
  const std::size_t begin =
      center_bin > radius ? center_bin - radius : 0;
  const std::size_t end = std::min(center_bin + radius, max_bin);

  std::size_t best_bin = begin;
  double best_magnitude = magnitudes[begin];
  for (std::size_t bin = begin + 1; bin <= end; ++bin) {
    if (magnitudes[bin] > best_magnitude) {
      best_bin = bin;
      best_magnitude = magnitudes[bin];
    }
  }

  return best_bin;
}

}  // namespace

HarmonicScore ComputeHarmonicSumSpectrum(
    const std::vector<double>& magnitudes,
    int sample_rate,
    std::size_t fft_size,
    double fundamental_frequency_hz,
    const HarmonicOptions& options) {
  HarmonicScore result;
  result.fundamental_frequency_hz = fundamental_frequency_hz;

  if (magnitudes.empty() || sample_rate <= 0 || fft_size == 0 ||
      fundamental_frequency_hz < options.min_frequency_hz ||
      fundamental_frequency_hz > options.max_frequency_hz ||
      options.max_harmonics <= 0) {
    return result;
  }

  const std::size_t usable_bin_count = std::min(magnitudes.size(), fft_size);
  if (usable_bin_count < 2) {
    return result;
  }

  const std::size_t nyquist_bin =
      std::min(usable_bin_count - 1, fft_size / 2);
  const double nyquist_hz = static_cast<double>(sample_rate) * 0.5;
  const double total_energy = std::accumulate(
      magnitudes.begin(),
      magnitudes.begin() + static_cast<std::ptrdiff_t>(nyquist_bin + 1),
      0.0);

  double weight_total = 0.0;
  double harmonic_energy = 0.0;

  for (int harmonic = 1; harmonic <= options.max_harmonics; ++harmonic) {
    const double expected_frequency =
        fundamental_frequency_hz * static_cast<double>(harmonic);
    if (expected_frequency > nyquist_hz ||
        expected_frequency > options.max_frequency_hz) {
      break;
    }

    const std::size_t center_bin =
        fft::FrequencyToBin(expected_frequency, sample_rate, fft_size);
    if (center_bin == 0 || center_bin > nyquist_bin) {
      continue;
    }

    const std::size_t peak_bin = FindLocalPeakBin(
        magnitudes,
        center_bin,
        options.search_radius_bins,
        nyquist_bin);
    const double magnitude = magnitudes[peak_bin];
    const double weight = HarmonicWeight(harmonic);
    const double weighted_magnitude = magnitude * weight;

    result.contributions.push_back(HarmonicContribution{
        harmonic,
        expected_frequency,
        peak_bin,
        magnitude,
        weight,
        weighted_magnitude,
    });
    result.weighted_sum += weighted_magnitude;
    harmonic_energy += magnitude;
    weight_total += weight;
  }

  result.harmonic_count = static_cast<int>(result.contributions.size());
  if (weight_total > 0.0) {
    result.normalized_score = result.weighted_sum / weight_total;
  }
  result.harmonic_energy_ratio = EstimateHarmonicity(
      harmonic_energy,
      total_energy);

  return result;
}

}  // namespace tunerlock::feature
