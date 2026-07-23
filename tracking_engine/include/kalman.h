#pragma once

namespace tunerlock::filter {

struct KalmanConfig {
  double process_noise = 0.000002;
  double minimum_measurement_noise = 0.000004;
  double maximum_measurement_noise = 0.0004;
  double initial_error = 0.001;
};

class PitchKalmanFilter {
 public:
  explicit PitchKalmanFilter(KalmanConfig config = {});

  double Process(double frequency_hz, double confidence);
  void Reset();
  bool initialized() const;

 private:
  KalmanConfig config_;
  bool initialized_ = false;
  double log_frequency_ = 0.0;
  double error_covariance_ = 0.0;
};

}  // namespace tunerlock::filter
