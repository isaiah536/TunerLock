#pragma once

#include <cstddef>
#include <vector>

namespace tunerlock::pitch {

struct YinResult {
  double frequency_hz = 0.0;
  double periodicity = 0.0;
  double cmndf_value = 1.0;
  bool detected = false;
};

constexpr float kDefaultYinThreshold = 0.15F;
constexpr double kDefaultMinFrequencyHz = 40.0;
constexpr double kDefaultMaxFrequencyHz = 2000.0;

std::vector<float> DifferenceFunction(
    const float* samples,
    std::size_t sample_count,
    std::size_t min_tau,
    std::size_t max_tau);

std::vector<float> CumulativeMeanNormalizedDifference(
    const std::vector<float>& difference);

int AbsoluteThreshold(
    const std::vector<float>& cmndf,
    float threshold,
    std::size_t min_tau,
    std::size_t max_tau);

float ParabolicInterpolation(
    const std::vector<float>& values,
    std::size_t tau);

double EstimatePitchYin(
    const float* samples,
    int sample_count,
    int sample_rate,
    float threshold = kDefaultYinThreshold,
    double min_frequency_hz = kDefaultMinFrequencyHz,
    double max_frequency_hz = kDefaultMaxFrequencyHz);

YinResult AnalyzePitchYin(
    const float* samples,
    int sample_count,
    int sample_rate,
    float threshold = kDefaultYinThreshold,
    double min_frequency_hz = kDefaultMinFrequencyHz,
    double max_frequency_hz = kDefaultMaxFrequencyHz);

}  // namespace tunerlock::pitch
