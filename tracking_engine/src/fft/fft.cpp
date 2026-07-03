#include <cmath>
#include <complex>
#include <cstddef>
#include <vector>

namespace tunerlock::fft {

std::vector<std::complex<float>> DiscreteFourierTransform(const std::vector<float>& samples) {
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

}  // namespace tunerlock::fft
