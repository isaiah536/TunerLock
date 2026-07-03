namespace tunerlock::tracking {

bool IsTrackingLocked(double confidence) {
  return confidence >= 0.9;
}

}  // namespace tunerlock::tracking
