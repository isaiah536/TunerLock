#include "audio_buffer.h"
#include "wav_reader.h"
#include "yin.h"

#include <cassert>
#include <iostream>
#include <string>

int main() {
  const tunerlock::audio::WavReadResult wav =
      tunerlock::audio::ReadWavFile("datasets/violin/violin_a.wav");

  assert(wav.ok);
  assert(wav.error.empty());
  assert(wav.buffer.sample_rate > 0);
  assert(!wav.buffer.empty());
  assert(wav.channel_count > 0);
  assert(wav.bits_per_sample > 0);
  assert(tunerlock::audio::PeakAmplitude(wav.buffer) <= 1.0F);
  assert(tunerlock::audio::ComputeRms(wav.buffer) > 0.0F);

  const double pitch = tunerlock::pitch::EstimatePitchYin(
      wav.buffer.samples.data(),
      static_cast<int>(wav.buffer.samples.size()),
      wav.buffer.sample_rate);
  std::cout << "wav_reader_test file=datasets/violin/violin_a.wav"
            << " sample_rate=" << wav.buffer.sample_rate
            << " channels=" << wav.channel_count
            << " bits_per_sample=" << wav.bits_per_sample
            << " sample_count=" << wav.buffer.sample_count()
            << " yin_pitch_hz=" << pitch
            << '\n';
  assert(pitch >= 0.0);

  const tunerlock::audio::WavReadResult missing =
      tunerlock::audio::ReadWavFile("datasets/violin/missing.wav");
  assert(!missing.ok);
  assert(!missing.error.empty());

  return 0;
}
