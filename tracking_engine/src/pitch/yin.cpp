#include "yin.h"

#include <algorithm>
#include <cmath>

namespace tunerlock::pitch {

std::vector<float> DifferenceFunction(
    const float* samples,
    std::size_t sample_count,
    std::size_t min_tau,
    std::size_t max_tau) {
  std::vector<float> difference(max_tau + 1, 0.0F);
  if (samples == nullptr || sample_count == 0 || min_tau > max_tau) {
    return difference;
  }

  const std::size_t safe_max_tau =
      std::min(max_tau, sample_count > 1 ? sample_count - 1 : 0);
  for (std::size_t tau = min_tau; tau <= safe_max_tau; ++tau) {
    const std::size_t limit = sample_count - tau;
    double sum = 0.0;
    for (std::size_t j = 0; j < limit; ++j) {
      const double delta =
          static_cast<double>(samples[j]) - static_cast<double>(samples[j + tau]);
      sum += delta * delta;
    }
    difference[tau] = static_cast<float>(sum);
  }

  return difference;
}

std::vector<float> CumulativeMeanNormalizedDifference(
    const std::vector<float>& difference) {
  std::vector<float> cmndf(difference.size(), 1.0F);
  if (difference.size() <= 1) {
    return cmndf;
  }

  double running_sum = 0.0;
  for (std::size_t tau = 1; tau < difference.size(); ++tau) {
    running_sum += difference[tau];
    if (running_sum <= 0.0) {
      cmndf[tau] = 1.0F;
    } else {
      cmndf[tau] =
          static_cast<float>(difference[tau] * static_cast<double>(tau) /
                             running_sum);
    }
  }

  return cmndf;
}

int AbsoluteThreshold(
    const std::vector<float>& cmndf,
    float threshold,
    std::size_t min_tau,
    std::size_t max_tau) {
  if (cmndf.empty() || min_tau >= cmndf.size()) {
    return -1;
  }

  const std::size_t end_tau = std::min(max_tau, cmndf.size() - 1);
  for (std::size_t tau = std::max<std::size_t>(min_tau, 2); tau <= end_tau; ++tau) {
    if (cmndf[tau] < threshold) {
      while (tau + 1 <= end_tau && cmndf[tau + 1] < cmndf[tau]) {
        ++tau;
      }
      return static_cast<int>(tau);
    }
  }

  return -1;
}

float ParabolicInterpolation(
    const std::vector<float>& values,
    std::size_t tau) {
  if (values.empty() || tau == 0 || tau + 1 >= values.size()) {
    return static_cast<float>(tau);
  }

  const float left = values[tau - 1];
  const float center = values[tau];
  const float right = values[tau + 1];
  const float denominator = 2.0F * (2.0F * center - left - right);

  if (std::fabs(denominator) < 0.000001F) {
    return static_cast<float>(tau);
  }

  const float offset = (right - left) / denominator;
  return static_cast<float>(tau) + offset;
}

double EstimatePitchYin(
    const float* samples,
    int sample_count,
    int sample_rate,
    float threshold,
    double min_frequency_hz,
    double max_frequency_hz) {
  return AnalyzePitchYin(
      samples,
      sample_count,
      sample_rate,
      threshold,
      min_frequency_hz,
      max_frequency_hz).frequency_hz;
}

YinResult AnalyzePitchYin(
    const float* samples,
    int sample_count,
    int sample_rate,
    float threshold,
    double min_frequency_hz,
    double max_frequency_hz) {
  YinResult result;
  if (samples == nullptr ||
      sample_count <= 0 ||
      sample_rate <= 0 ||
      min_frequency_hz <= 0.0 ||
      max_frequency_hz <= min_frequency_hz) {
    return result;
  }

  const std::size_t sample_count_size = static_cast<std::size_t>(sample_count);
  std::size_t min_tau = static_cast<std::size_t>(
      std::floor(static_cast<double>(sample_rate) / max_frequency_hz));
  std::size_t max_tau = static_cast<std::size_t>(
      std::ceil(static_cast<double>(sample_rate) / min_frequency_hz));

  min_tau = std::max<std::size_t>(min_tau, 2);
  max_tau = std::min(max_tau, sample_count_size > 1 ? sample_count_size - 1 : 0);
  if (min_tau >= max_tau) {
    return result;
  }

  const std::vector<float> difference =
      DifferenceFunction(samples, sample_count_size, min_tau, max_tau);
  const std::vector<float> cmndf =
      CumulativeMeanNormalizedDifference(difference);

  const int tau = AbsoluteThreshold(cmndf, threshold, min_tau, max_tau);
  if (tau < 0) {
    return result;
  }

  const float tau_fine = ParabolicInterpolation(cmndf, static_cast<std::size_t>(tau));
  if (tau_fine <= 0.0F) {
    return result;
  }

  result.frequency_hz =
      static_cast<double>(sample_rate) / static_cast<double>(tau_fine);
  result.cmndf_value = std::clamp(
      static_cast<double>(cmndf[static_cast<std::size_t>(tau)]),
      0.0,
      1.0);
  result.periodicity = 1.0 - result.cmndf_value;
  result.detected = true;
  return result;
}

}  // namespace tunerlock::pitch
