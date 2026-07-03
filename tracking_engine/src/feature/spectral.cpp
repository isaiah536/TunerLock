namespace tunerlock::feature {

double EstimateSpectralCentroid(const double* magnitudes, int bin_count) {
  double weighted_sum = 0.0;
  double magnitude_sum = 0.0;
  for (int i = 0; i < bin_count; ++i) {
    weighted_sum += magnitudes[i] * i;
    magnitude_sum += magnitudes[i];
  }
  return magnitude_sum > 0.0 ? weighted_sum / magnitude_sum : 0.0;
}

}  // namespace tunerlock::feature
