#include "audio_buffer.h"
#include "tracking_engine.h"
#include "wav_reader.h"

#include <algorithm>
#include <cstddef>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <cctype>
#include <cstdlib>
#include <string>
#include <vector>

namespace {

std::string Lowercase(std::string value) {
  for (char& ch : value) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  return value;
}

tunerlock::tracking::InstrumentProfile ParseProfile(
    const std::string& profile_arg) {
  const std::string profile = Lowercase(profile_arg);
  if (profile == "wind") {
    return tunerlock::tracking::InstrumentProfile::Wind;
  }
  if (profile == "brass" || profile == "trumpet") {
    return tunerlock::tracking::InstrumentProfile::Brass;
  }
  return tunerlock::tracking::InstrumentProfile::Strings;
}

double TraceMetric(
    const tunerlock::DebugTrace& trace,
    const std::string& stage,
    const std::string& name) {
  for (const auto& metric : trace.metrics) {
    if (metric.stage == stage && metric.name == name) {
      return metric.value;
    }
  }
  return 0.0;
}

}  // namespace

int main(int argc, char** argv) {
  const std::string wav_path =
      argc > 1 ? argv[1] : "datasets/violin/violin_e.wav";
  const std::string csv_path =
      argc > 2 ? argv[2]
               : "tracking_engine/build/violin_e_engine_frames.csv";
  const std::string mode_arg = argc > 3 ? Lowercase(argv[3]) : "tuning";
  const std::string profile_arg = argc > 4 ? Lowercase(argv[4]) : "strings";
  const double reference_pitch_hz =
      argc > 5 ? std::atof(argv[5]) : 440.0;
  const tunerlock::tracking::TrackingMode mode =
      mode_arg == "performance" || mode_arg == "perform" ||
              mode_arg == "play"
          ? tunerlock::tracking::TrackingMode::Performance
          : tunerlock::tracking::TrackingMode::Tuning;
  const tunerlock::tracking::InstrumentProfile profile =
      ParseProfile(profile_arg);

  tunerlock::audio::WavReadResult wav =
      tunerlock::audio::ReadWavFile(wav_path);
  if (!wav.ok) {
    std::cerr << wav.error << '\n';
    return 1;
  }

  constexpr std::size_t kFrameSize = tunerlock::audio::kDefaultFrameSize;
  const std::size_t hop_size =
      (profile == tunerlock::tracking::InstrumentProfile::Strings ||
       profile == tunerlock::tracking::InstrumentProfile::Wind) &&
              mode == tunerlock::tracking::TrackingMode::Performance
          ? kFrameSize / 2
          : kFrameSize;
  tunerlock::TrackingEngine engine(
      profile,
      mode);
  engine.SetReferencePitchHz(reference_pitch_hz);
  std::ofstream csv(csv_path);
  csv << "frame,time_seconds,frequency_hz,cents,confidence,locked,"
      << "rms,attack_confirmed,downward_pitch_drop_cents,downward_tail,"
      << "downward_hold,pending_downward_frame_count\n";

  int frame_count = 0;
  int locked_frame_count = 0;
  double confidence_sum = 0.0;
  double confidence_min = std::numeric_limits<double>::max();
  double confidence_max = 0.0;
  double frequency_sum = 0.0;
  int first_locked_frame = -1;

  std::vector<float> frame(kFrameSize, 0.0F);
  for (std::size_t start = 0;
       start < wav.buffer.samples.size();
       start += hop_size) {
    std::fill(frame.begin(), frame.end(), 0.0F);
    const std::size_t available =
        std::min(kFrameSize, wav.buffer.samples.size() - start);
    std::copy_n(
        wav.buffer.samples.begin() + static_cast<std::ptrdiff_t>(start),
        available,
        frame.begin());

    tunerlock::DebugTrace trace;
    const tunerlock::TrackingResult result = engine.ProcessFrameWithDebug(
        frame.data(),
        static_cast<int>(frame.size()),
        wav.buffer.sample_rate,
        &trace);
    const double time_seconds =
        static_cast<double>(start) /
        static_cast<double>(wav.buffer.sample_rate);

    csv << frame_count << ',' << std::fixed << std::setprecision(6)
        << time_seconds << ',' << result.frequency_hz << ','
        << result.cents << ',' << result.confidence << ','
        << (result.locked ? 1 : 0) << ','
        << TraceMetric(trace, "feature", "rms") << ','
        << TraceMetric(trace, "tracking", "attack_confirmed") << ','
        << TraceMetric(trace, "output", "downward_pitch_drop_cents") << ','
        << TraceMetric(trace, "output", "downward_tail") << ','
        << TraceMetric(trace, "output", "downward_hold") << ','
        << TraceMetric(trace, "output", "pending_downward_frame_count")
        << '\n';

    if (result.locked) {
      if (first_locked_frame < 0) {
        first_locked_frame = frame_count;
      }
      ++locked_frame_count;
      confidence_sum += result.confidence;
      confidence_min = std::min(confidence_min, result.confidence);
      confidence_max = std::max(confidence_max, result.confidence);
      frequency_sum += result.frequency_hz;
    }
    ++frame_count;
  }

  const double confidence_mean =
      locked_frame_count > 0
          ? confidence_sum / static_cast<double>(locked_frame_count)
          : 0.0;
  const double frequency_mean =
      locked_frame_count > 0
          ? frequency_sum / static_cast<double>(locked_frame_count)
          : 0.0;
  if (locked_frame_count == 0) {
    confidence_min = 0.0;
  }

  std::cout << std::fixed << std::setprecision(6);
  std::cout << "wav=" << wav_path << '\n';
  std::cout << "csv=" << csv_path << '\n';
  std::cout << "mode=" << mode_arg << '\n';
  std::cout << "profile=" << profile_arg << '\n';
  std::cout << "reference_pitch_hz=" << engine.reference_pitch_hz() << '\n';
  std::cout << "hop_size=" << hop_size << '\n';
  std::cout << "frame_count=" << frame_count << '\n';
  std::cout << "locked_frame_count=" << locked_frame_count << '\n';
  std::cout << "first_locked_frame=" << first_locked_frame << '\n';
  std::cout << "confidence.mean=" << confidence_mean << '\n';
  std::cout << "confidence.min=" << confidence_min << '\n';
  std::cout << "confidence.max=" << confidence_max << '\n';
  std::cout << "frequency.mean_hz=" << frequency_mean << '\n';
  return locked_frame_count > 0 ? 0 : 2;
}
