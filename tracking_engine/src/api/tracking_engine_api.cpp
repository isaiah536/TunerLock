#include "audio_buffer.h"
#include "fft.h"
#include "tracking_engine.h"
#include "window.h"
#include "yin.h"

#include <vector>

namespace tunerlock {

TrackingResult TrackingEngine::ProcessFrame(
    const float* samples,
    int sample_count,
    int sample_rate) {
  return ProcessFrameWithDebug(samples, sample_count, sample_rate, nullptr);
}

TrackingResult TrackingEngine::ProcessFrameWithDebug(
    const float* samples,
    int sample_count,
    int sample_rate,
    DebugTrace* trace) {
  if (trace != nullptr) {
    trace->Clear();
    trace->Add("input", "sample_count", sample_count);
    trace->Add("input", "sample_rate", sample_rate);
  }

  if (sample_count <= 0) {
    return TrackingResult{};
  }

  AudioFrame frame{
      samples,
      static_cast<std::size_t>(sample_count),
      sample_rate,
  };

  if (!audio::IsValidAudioFrame(frame)) {
    if (trace != nullptr) {
      trace->Add("input", "valid", 0.0);
    }
    return TrackingResult{};
  }
  if (trace != nullptr) {
    trace->Add("input", "valid", 1.0);
  }

  AudioBuffer buffer = audio::MakeAudioBuffer(frame);
  if (trace != nullptr) {
    trace->Add("audio_buffer", "sample_count", buffer.sample_count());
    trace->Add("audio_buffer", "peak_before", audio::PeakAmplitude(buffer));
    trace->Add("audio_buffer", "rms_before", audio::ComputeRms(buffer));
  }

  audio::RemoveDcOffsetInPlace(buffer);
  audio::NormalizeInPlace(buffer);
  if (trace != nullptr) {
    trace->Add("audio_buffer", "peak_after", audio::PeakAmplitude(buffer));
    trace->Add("audio_buffer", "rms_after", audio::ComputeRms(buffer));
  }

  std::vector<float> window_input = audio::CopySamplesForWindowing(buffer);
  std::vector<float> window = audio::HannWindow(window_input.size());
  audio::ApplyWindowInPlace(window_input, window);

  const float rms = audio::ComputeRms(
      AudioBuffer{window_input, buffer.sample_rate});
  if (trace != nullptr) {
    trace->Add("windowing", "window_size", window_input.size());
    trace->Add("windowing", "rms_after", rms);

    const auto spectrum = fft::DiscreteFourierTransform(window_input);
    const std::vector<double> magnitudes = fft::Magnitudes(spectrum);
    const std::size_t peak_bin = fft::FindPeakBin(magnitudes);
    trace->Add("fft", "bin_count", magnitudes.size());
    trace->Add("fft", "peak_bin", peak_bin);
    trace->Add(
        "fft",
        "peak_frequency_hz",
        fft::BinToFrequency(peak_bin, buffer.sample_rate, window_input.size()));

    const double yin_pitch = pitch::EstimatePitchYin(
        buffer.samples.data(),
        static_cast<int>(buffer.samples.size()),
        buffer.sample_rate);
    trace->Add("yin", "pitch_hz", yin_pitch);
  }

  return TrackingResult{
      0.0,
      0.0,
      rms,
      rms > 0.01F,
  };
}

}  // namespace tunerlock
