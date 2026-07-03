#include "types.h"

#include <algorithm>

namespace tunerlock::audio {

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

void NormalizeInPlace(AudioBuffer& buffer) {
  float peak = 0.0F;
  for (float sample : buffer.samples) {
    peak = std::max(peak, sample < 0.0F ? -sample : sample);
  }

  if (peak <= 0.0F) {
    return;
  }

  for (float& sample : buffer.samples) {
    sample /= peak;
  }
}

}  // namespace tunerlock::audio
