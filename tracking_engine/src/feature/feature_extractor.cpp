#include "feature_extractor.h"

#include "fft.h"

#include <algorithm>
#include <cmath>

namespace tunerlock::feature {

double ExtractSignalEnergy(const float* samples, int sample_count) {
  double energy = 0.0;
  for (int i = 0; i < sample_count; ++i) {
    energy += samples[i] * samples[i];
  }
  return sample_count > 0 ? energy / sample_count : 0.0;
}

namespace {

double ComputePeakAmplitude(const float* samples, std::size_t sample_count) {
  if (samples == nullptr || sample_count == 0) {
    return 0.0;
  }

  double peak = 0.0;
  for (std::size_t i = 0; i < sample_count; ++i) {
    peak = std::max(peak, std::fabs(static_cast<double>(samples[i])));
  }
  return peak;
}

double ComputeSpectralCentroidHz(
    const std::vector<double>& magnitudes,
    int sample_rate,
    std::size_t fft_size) {
  if (magnitudes.empty() || sample_rate <= 0 || fft_size == 0) {
    return 0.0;
  }

  const std::size_t nyquist_bin =
      std::min(magnitudes.size() - 1, fft_size / 2);
  double weighted_sum = 0.0;
  double magnitude_sum = 0.0;
  for (std::size_t bin = 0; bin <= nyquist_bin; ++bin) {
    const double magnitude = magnitudes[bin];
    const double frequency = fft::BinToFrequency(bin, sample_rate, fft_size);
    weighted_sum += magnitude * frequency;
    magnitude_sum += magnitude;
  }

  return magnitude_sum > 0.0 ? weighted_sum / magnitude_sum : 0.0;
}

}  // namespace

FeatureVector ExtractFeatures(const FeatureInput& input) {
  FeatureVector features;

  if (input.samples != nullptr && input.sample_count > 0) {
    features.rms = std::sqrt(ExtractSignalEnergy(
        input.samples,
        static_cast<int>(input.sample_count)));
    features.peak_amplitude =
        ComputePeakAmplitude(input.samples, input.sample_count);
  }

  features.yin_pitch_hz = input.yin_pitch_hz;
  features.yin_detected = input.yin_pitch_hz > 0.0;

  if (input.magnitudes != nullptr &&
      !input.magnitudes->empty() &&
      input.sample_rate > 0 &&
      input.fft_size > 0) {
    const std::vector<double>& magnitudes = *input.magnitudes;
    const std::size_t nyquist_bin =
        std::min(magnitudes.size() - 1, input.fft_size / 2);
    const auto begin = magnitudes.begin();
    const auto end = begin + static_cast<std::ptrdiff_t>(nyquist_bin + 1);
    const auto peak = std::max_element(begin, end);
    const std::size_t peak_bin =
        static_cast<std::size_t>(peak - magnitudes.begin());

    features.fft_peak_frequency_hz =
        fft::BinToFrequency(peak_bin, input.sample_rate, input.fft_size);
    features.fft_peak_magnitude = *peak;
    features.spectral_centroid_hz =
        ComputeSpectralCentroidHz(magnitudes, input.sample_rate, input.fft_size);
  }

  features.harmonic_weighted_sum = input.harmonic_score.weighted_sum;
  features.harmonic_normalized_score =
      input.harmonic_score.normalized_score;
  features.harmonic_energy_ratio =
      input.harmonic_score.harmonic_energy_ratio;
  features.harmonic_count = input.harmonic_score.harmonic_count;

  return features;
}

}  // namespace tunerlock::feature
