#include "audio_buffer.h"

#include <cassert>
#include <cmath>
#include <vector>

namespace {

bool Near(float lhs, float rhs, float tolerance = 0.0001F) {
  return std::fabs(lhs - rhs) <= tolerance;
}

}  // namespace

int main() {
  const std::vector<float> samples = {0.25F, -0.5F, 0.75F, -1.0F};
  const tunerlock::AudioFrame frame{
      samples.data(),
      samples.size(),
      tunerlock::audio::kDefaultSampleRate,
  };

  assert(tunerlock::audio::IsValidAudioFrame(frame));

  tunerlock::AudioBuffer buffer = tunerlock::audio::MakeAudioBuffer(frame);
  assert(buffer.sample_rate == tunerlock::audio::kDefaultSampleRate);
  assert(buffer.sample_count() == samples.size());
  assert(buffer.samples[0] == 0.25F);
  assert(buffer.samples[3] == -1.0F);

  assert(Near(tunerlock::audio::PeakAmplitude(buffer), 1.0F));
  assert(Near(tunerlock::audio::ComputeRms(buffer), 0.684653F));

  tunerlock::AudioBuffer dc_buffer{{2.0F, 4.0F, 6.0F}, 48000};
  tunerlock::audio::RemoveDcOffsetInPlace(dc_buffer);
  assert(Near(dc_buffer.samples[0], -2.0F));
  assert(Near(dc_buffer.samples[1], 0.0F));
  assert(Near(dc_buffer.samples[2], 2.0F));

  tunerlock::audio::NormalizeInPlace(dc_buffer);
  assert(Near(dc_buffer.samples[0], -1.0F));
  assert(Near(dc_buffer.samples[1], 0.0F));
  assert(Near(dc_buffer.samples[2], 1.0F));

  const std::vector<float> window_input =
      tunerlock::audio::CopySamplesForWindowing(buffer, 6);
  assert(window_input.size() == 6);
  assert(window_input[0] == 0.25F);
  assert(window_input[3] == -1.0F);
  assert(window_input[4] == 0.0F);
  assert(window_input[5] == 0.0F);

  const std::vector<float> truncated =
      tunerlock::audio::CopySamplesForWindowing(buffer, 2);
  assert(truncated.size() == 2);
  assert(truncated[0] == 0.25F);
  assert(truncated[1] == -0.5F);

  const tunerlock::AudioFrame invalid_frame{nullptr, 0, 0};
  assert(!tunerlock::audio::IsValidAudioFrame(invalid_frame));

  return 0;
}
