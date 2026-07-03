namespace tunerlock::feature {

double EstimateHarmonicity(double fundamental_energy, double total_energy) {
  return total_energy > 0.0 ? fundamental_energy / total_energy : 0.0;
}

}  // namespace tunerlock::feature
