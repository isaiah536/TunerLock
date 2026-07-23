#include "kalman.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace tunerlock::filter {

PitchKalmanFilter::PitchKalmanFilter(KalmanConfig config)
    : config_(std::move(config)),
      error_covariance_(config_.initial_error) {}

double PitchKalmanFilter::Process(
    double frequency_hz,
    double confidence) {
  if (frequency_hz <= 0.0) {
    return initialized_ ? std::exp2(log_frequency_) : 0.0;
  }

  const double measurement = std::log2(frequency_hz);
  if (!initialized_) {
    initialized_ = true;
    log_frequency_ = measurement;
    error_covariance_ = config_.initial_error;
    return frequency_hz;
  }

  error_covariance_ += config_.process_noise;
  const double quality = std::clamp(confidence, 0.0, 1.0);
  const double measurement_noise =
      config_.maximum_measurement_noise -
      quality * (config_.maximum_measurement_noise -
                 config_.minimum_measurement_noise);
  const double gain =
      error_covariance_ / (error_covariance_ + measurement_noise);

  log_frequency_ += gain * (measurement - log_frequency_);
  error_covariance_ *= 1.0 - gain;
  return std::exp2(log_frequency_);
}

void PitchKalmanFilter::Reset() {
  initialized_ = false;
  log_frequency_ = 0.0;
  error_covariance_ = config_.initial_error;
}

bool PitchKalmanFilter::initialized() const {
  return initialized_;
}

}  // namespace tunerlock::filter
