#include "audio_buffer.h"
#include "tracking_engine.h"
#include "window.h"

#include <vector>

namespace tunerlock {

TrackingResult TrackingEngine::ProcessFrame(
    const float* samples,
    int sample_count,
    int sample_rate) {
  if (sample_count <= 0) {
    return TrackingResult{};
  }

  AudioFrame frame{
      samples,
      static_cast<std::size_t>(sample_count),
      sample_rate,
  };

  if (!audio::IsValidAudioFrame(frame)) {
    return TrackingResult{};
  }

  AudioBuffer buffer = audio::MakeAudioBuffer(frame);
  audio::RemoveDcOffsetInPlace(buffer);
  audio::NormalizeInPlace(buffer);

  std::vector<float> window_input = audio::CopySamplesForWindowing(buffer);
  std::vector<float> window = audio::HannWindow(window_input.size());
  audio::ApplyWindowInPlace(window_input, window);

  const float rms = audio::ComputeRms(
      AudioBuffer{window_input, buffer.sample_rate});
  return TrackingResult{
      0.0,
      0.0,
      rms,
      rms > 0.01F,
  };
}

}  // namespace tunerlock
