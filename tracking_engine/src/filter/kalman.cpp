namespace tunerlock::filter {

double SmoothPitch(double previous, double current, double blend) {
  return previous + (current - previous) * blend;
}

}  // namespace tunerlock::filter
