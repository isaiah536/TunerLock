#pragma once

#include <complex>
#include <cstddef>
#include <vector>

namespace tunerlock::fft {

struct TargetBandOptions {
  int max_harmonics = 6;
  int radius_bins = 1;
  double max_frequency_hz = 5000.0;
};

struct TargetBandEnergy {
  double target_energy = 0.0;
  double fundamental_energy = 0.0;
  double strongest_harmonic_energy = 0.0;
  double total_energy = 0.0;
  double ratio = 0.0;
  double fundamental_ratio = 0.0;
  double strongest_harmonic_ratio = 0.0;
  int harmonic_bands_used = 0;
};

std::vector<std::complex<float>> DiscreteFourierTransform(
    const std::vector<float>& samples);

std::vector<double> Magnitudes(
    const std::vector<std::complex<float>>& spectrum);

std::size_t FindPeakBin(const std::vector<double>& magnitudes);

double BinToFrequency(std::size_t bin, int sample_rate, std::size_t fft_size);

std::size_t FrequencyToBin(
    double frequency_hz,
    int sample_rate,
    std::size_t fft_size);

TargetBandEnergy ComputeTargetBandEnergy(
    const std::vector<double>& magnitudes,
    int sample_rate,
    std::size_t fft_size,
    double target_frequency_hz,
    const TargetBandOptions& options = TargetBandOptions{});

}  // namespace tunerlock::fft
