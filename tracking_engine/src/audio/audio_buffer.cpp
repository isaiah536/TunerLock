#include "audio_buffer.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace tunerlock::audio {

bool IsValidAudioFrame(const AudioFrame& frame) {
  return frame.samples != nullptr &&
         frame.sample_count > 0 &&
         frame.sample_rate > 0;
}

AudioBuffer MakeAudioBuffer(
    const float* samples,
    std::size_t sample_count,
    int sample_rate) {
  AudioBuffer buffer;
  buffer.sample_rate = sample_rate;
  if (samples == nullptr || sample_count == 0) {
    return buffer;
  }

  buffer.samples.assign(samples, samples + sample_count);
  return buffer;
}

AudioBuffer MakeAudioBuffer(const AudioFrame& frame) {
  return MakeAudioBuffer(frame.samples, frame.sample_count, frame.sample_rate);
}

float PeakAmplitude(const AudioBuffer& buffer) {
  float peak = 0.0F;
  for (float sample : buffer.samples) {
    peak = std::max(peak, std::fabs(sample));
  }
  return peak;
}

float ComputeRms(const AudioBuffer& buffer) {
  if (buffer.samples.empty()) {
    return 0.0F;
  }

  double sum = 0.0;
  for (float sample : buffer.samples) {
    sum += static_cast<double>(sample) * static_cast<double>(sample);
  }
  return static_cast<float>(std::sqrt(sum / buffer.samples.size()));
}

void RemoveDcOffsetInPlace(AudioBuffer& buffer) {
  if (buffer.samples.empty()) {
    return;
  }

  const double sum = std::accumulate(
      buffer.samples.begin(),
      buffer.samples.end(),
      0.0);
  const float mean = static_cast<float>(sum / buffer.samples.size());
  for (float& sample : buffer.samples) {
    sample -= mean;
  }
}

void NormalizeInPlace(AudioBuffer& buffer) {
  const float peak = PeakAmplitude(buffer);
  if (peak <= 0.0F) {
    return;
  }

  for (float& sample : buffer.samples) {
    sample /= peak;
  }
}

std::vector<float> CopySamplesForWindowing(
    const AudioBuffer& buffer,
    std::size_t window_size) {
  if (window_size == 0) {
    return {};
  }

  std::vector<float> window_input(window_size, 0.0F);
  const std::size_t copy_count = std::min(buffer.samples.size(), window_size);
  std::copy_n(buffer.samples.begin(), copy_count, window_input.begin());
  return window_input;
}

}  // namespace tunerlock::audio
