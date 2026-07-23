#include "fft.h"

#include <algorithm>
#include <cmath>

namespace tunerlock::fft {
namespace {

bool IsPowerOfTwo(std::size_t value) {
  return value > 0 && (value & (value - 1)) == 0;
}

std::vector<std::complex<float>> Radix2Fft(
    const std::vector<float>& samples) {
  constexpr double kPi = 3.14159265358979323846;
  const std::size_t count = samples.size();
  std::vector<std::complex<double>> values(count);

  std::size_t reversed = 0;
  for (std::size_t index = 0; index < count; ++index) {
    values[reversed] = static_cast<double>(samples[index]);
    std::size_t bit = count >> 1;
    while (reversed & bit) {
      reversed ^= bit;
      bit >>= 1;
    }
    reversed ^= bit;
  }

  for (std::size_t length = 2; length <= count; length <<= 1) {
    const double angle = -2.0 * kPi / static_cast<double>(length);
    const std::complex<double> step(std::cos(angle), std::sin(angle));
    for (std::size_t start = 0; start < count; start += length) {
      std::complex<double> twiddle(1.0, 0.0);
      const std::size_t half = length / 2;
      for (std::size_t offset = 0; offset < half; ++offset) {
        const std::complex<double> even = values[start + offset];
        const std::complex<double> odd =
            values[start + offset + half] * twiddle;
        values[start + offset] = even + odd;
        values[start + offset + half] = even - odd;
        twiddle *= step;
      }
    }
  }

  std::vector<std::complex<float>> spectrum(count);
  for (std::size_t index = 0; index < count; ++index) {
    spectrum[index] = {
        static_cast<float>(values[index].real()),
        static_cast<float>(values[index].imag()),
    };
  }
  return spectrum;
}

}  // namespace

std::vector<std::complex<float>> DiscreteFourierTransform(
    const std::vector<float>& samples) {
  constexpr double kPi = 3.14159265358979323846;
  const std::size_t count = samples.size();
  if (IsPowerOfTwo(count)) {
    return Radix2Fft(samples);
  }
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

TargetBandEnergy ComputeTargetBandEnergy(
    const std::vector<double>& magnitudes,
    int sample_rate,
    std::size_t fft_size,
    double target_frequency_hz,
    const TargetBandOptions& options) {
  TargetBandEnergy result;
  if (magnitudes.empty() || sample_rate <= 0 || fft_size == 0 ||
      target_frequency_hz <= 0.0 || options.max_harmonics <= 0 ||
      options.radius_bins < 0) {
    return result;
  }

  const std::size_t nyquist_bin =
      std::min(magnitudes.size() - 1, fft_size / 2);
  if (nyquist_bin < 1) {
    return result;
  }

  for (std::size_t bin = 1; bin <= nyquist_bin; ++bin) {
    result.total_energy += magnitudes[bin] * magnitudes[bin];
  }
  if (result.total_energy <= 0.0) {
    return result;
  }

  std::vector<bool> included(nyquist_bin + 1, false);
  const double nyquist_hz = static_cast<double>(sample_rate) * 0.5;
  const double maximum_hz = std::min(options.max_frequency_hz, nyquist_hz);

  for (int harmonic = 1; harmonic <= options.max_harmonics; ++harmonic) {
    const double expected_hz =
        target_frequency_hz * static_cast<double>(harmonic);
    if (expected_hz > maximum_hz) {
      break;
    }

    const std::size_t center =
        FrequencyToBin(expected_hz, sample_rate, fft_size);
    if (center == 0 || center > nyquist_bin) {
      continue;
    }

    const std::size_t radius =
        static_cast<std::size_t>(options.radius_bins);
    const std::size_t first = center > radius ? center - radius : 1;
    const std::size_t last = std::min(nyquist_bin, center + radius);
    double band_energy = 0.0;
    for (std::size_t bin = first; bin <= last; ++bin) {
      band_energy += magnitudes[bin] * magnitudes[bin];
      if (!included[bin]) {
        result.target_energy += magnitudes[bin] * magnitudes[bin];
        included[bin] = true;
      }
    }
    if (harmonic == 1) {
      result.fundamental_energy = band_energy;
    }
    result.strongest_harmonic_energy =
        std::max(result.strongest_harmonic_energy, band_energy);
    ++result.harmonic_bands_used;
  }

  result.ratio =
      std::clamp(result.target_energy / result.total_energy, 0.0, 1.0);
  result.fundamental_ratio =
      std::clamp(result.fundamental_energy / result.total_energy, 0.0, 1.0);
  result.strongest_harmonic_ratio = std::clamp(
      result.strongest_harmonic_energy / result.total_energy, 0.0, 1.0);
  return result;
}

}  // namespace tunerlock::fft
