namespace tunerlock::feature {

double ExtractSignalEnergy(const float* samples, int sample_count) {
  double energy = 0.0;
  for (int i = 0; i < sample_count; ++i) {
    energy += samples[i] * samples[i];
  }
  return sample_count > 0 ? energy / sample_count : 0.0;
}

}  // namespace tunerlock::feature
