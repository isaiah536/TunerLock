namespace tunerlock::tracking {

double ComputeConfidence(double signal_quality, double pitch_stability) {
  return signal_quality * pitch_stability;
}

}  // namespace tunerlock::tracking
