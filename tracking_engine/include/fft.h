#pragma once

#include <complex>
#include <cstddef>
#include <vector>

namespace tunerlock::fft {

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

}  // namespace tunerlock::fft
