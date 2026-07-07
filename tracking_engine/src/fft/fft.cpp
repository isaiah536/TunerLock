#include "fft.h"

#include <algorithm>
#include <cmath>

namespace tunerlock::fft {

std::vector<std::complex<float>> DiscreteFourierTransform(
    const std::vector<float>& samples) {
  constexpr double kPi = 3.14159265358979323846;
  const std::size_t count = samples.size();
  std::vector<std::complex<float>> spectrum(count);

  for (std::size_t k = 0; k < count; ++k) {
    std::complex<double> sum = 0.0;
    for (std::size_t n = 0; n < count; ++n) {
      const double phase =
          -2.0 * kPi * static_cast<double>(k) * static_cast<double>(n) /
          static_cast<double>(count);
      sum += static_cast<double>(samples[n]) *
             std::complex<double>(std::cos(phase), std::sin(phase));
    }
    spectrum[k] = std::complex<float>(
        static_cast<float>(sum.real()),
        static_cast<float>(sum.imag()));
  }

  return spectrum;
}

std::vector<double> Magnitudes(const std::vector<std::complex<float>>& spectrum) {
  std::vector<double> magnitudes;
  magnitudes.reserve(spectrum.size());
  for (const auto& bin : spectrum) {
    magnitudes.push_back(std::abs(bin));
  }
  return magnitudes;
}

std::size_t FindPeakBin(const std::vector<double>& magnitudes) {
  if (magnitudes.empty()) {
    return 0;
  }

  return static_cast<std::size_t>(
      std::max_element(magnitudes.begin(), magnitudes.end()) -
      magnitudes.begin());
}

double BinToFrequency(std::size_t bin, int sample_rate, std::size_t fft_size) {
  if (sample_rate <= 0 || fft_size == 0) {
    return 0.0;
  }

  return static_cast<double>(bin) * static_cast<double>(sample_rate) /
         static_cast<double>(fft_size);
}

std::size_t FrequencyToBin(
    double frequency_hz,
    int sample_rate,
    std::size_t fft_size) {
  if (frequency_hz <= 0.0 || sample_rate <= 0 || fft_size == 0) {
    return 0;
  }

  const double bin = frequency_hz * static_cast<double>(fft_size) /
                     static_cast<double>(sample_rate);
  return static_cast<std::size_t>(std::round(bin));
}

}  // namespace tunerlock::fft
