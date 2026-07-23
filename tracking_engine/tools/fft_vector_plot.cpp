#include "audio_buffer.h"
#include "feature_extractor.h"
#include "fft.h"
#include "harmonic.h"
#include "wav_reader.h"
#include "window.h"
#include "yin.h"

#include <algorithm>
#include <cstddef>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace {

struct FrameAnalysis {
  std::size_t frame_index = 0;
  std::size_t start_sample = 0;
  int sample_rate = 0;
  std::size_t fft_size = 0;
  double start_time_seconds = 0.0;
  std::vector<double> magnitudes;
  double yin_pitch_hz = 0.0;
  tunerlock::feature::HarmonicScore harmonic_score;
  tunerlock::feature::FeatureVector features;
};

std::vector<float> CopyFrameSamples(
    const tunerlock::AudioBuffer& buffer,
    std::size_t start_sample,
    std::size_t frame_size) {
  std::vector<float> frame(frame_size, 0.0F);
  if (start_sample >= buffer.samples.size()) {
    return frame;
  }

  const std::size_t available =
      std::min(frame_size, buffer.samples.size() - start_sample);
  std::copy_n(buffer.samples.begin() + static_cast<std::ptrdiff_t>(start_sample),
              available,
              frame.begin());
  return frame;
}

FrameAnalysis AnalyzeFrame(
    const tunerlock::AudioBuffer& buffer,
    std::size_t frame_index,
    std::size_t start_sample,
    std::size_t frame_size) {
  FrameAnalysis analysis;
  analysis.frame_index = frame_index;
  analysis.start_sample = start_sample;
  analysis.sample_rate = buffer.sample_rate;
  analysis.fft_size = frame_size;
  analysis.start_time_seconds =
      static_cast<double>(start_sample) / static_cast<double>(buffer.sample_rate);

  std::vector<float> frame =
      CopyFrameSamples(buffer, start_sample, frame_size);
  std::vector<float> window_input = frame;
  const std::vector<float> window =
      tunerlock::audio::HannWindow(window_input.size());
  tunerlock::audio::ApplyWindowInPlace(window_input, window);

  const auto spectrum =
      tunerlock::fft::DiscreteFourierTransform(window_input);
  analysis.magnitudes = tunerlock::fft::Magnitudes(spectrum);
  analysis.yin_pitch_hz = tunerlock::pitch::EstimatePitchYin(
      frame.data(),
      static_cast<int>(frame.size()),
      buffer.sample_rate);
  analysis.harmonic_score =
      tunerlock::feature::ComputeHarmonicSumSpectrum(
          analysis.magnitudes,
          buffer.sample_rate,
          frame_size,
          analysis.yin_pitch_hz);
  analysis.features = tunerlock::feature::ExtractFeatures(
      tunerlock::feature::FeatureInput{
          frame.data(),
          frame.size(),
          buffer.sample_rate,
          &analysis.magnitudes,
          frame_size,
          analysis.yin_pitch_hz,
          analysis.harmonic_score,
      });

  return analysis;
}

void WriteCsv(
    const std::string& path,
    const std::vector<double>& magnitudes,
    int sample_rate,
    std::size_t fft_size) {
  std::ofstream csv(path);
  csv << "bin,frequency_hz,magnitude\n";

  const std::size_t nyquist_bin =
      std::min(magnitudes.size() - 1, fft_size / 2);
  for (std::size_t bin = 0; bin <= nyquist_bin; ++bin) {
    csv << bin << ','
        << tunerlock::fft::BinToFrequency(bin, sample_rate, fft_size) << ','
        << magnitudes[bin] << '\n';
  }
}

void WriteHarmonicCsv(
    const std::string& path,
    const tunerlock::feature::HarmonicScore& harmonic_score,
    int sample_rate,
    std::size_t fft_size) {
  std::ofstream csv(path);
  csv << "harmonic,expected_frequency_hz,bin,bin_frequency_hz,magnitude,"
      << "weight,weighted_magnitude\n";

  for (const auto& contribution : harmonic_score.contributions) {
    csv << contribution.harmonic_number << ','
        << contribution.expected_frequency_hz << ','
        << contribution.bin << ','
        << tunerlock::fft::BinToFrequency(
               contribution.bin,
               sample_rate,
               fft_size)
        << ','
        << contribution.magnitude << ','
        << contribution.weight << ','
        << contribution.weighted_magnitude << '\n';
  }
}

void WriteFrameFeaturesCsv(
    const std::string& path,
    const std::vector<FrameAnalysis>& frames) {
  std::ofstream csv(path);
  csv << "frame,start_sample,start_time_seconds,yin_pitch_hz,rms,"
      << "peak_amplitude,fft_peak_frequency_hz,fft_peak_magnitude,"
      << "spectral_centroid_hz,harmonic_normalized_score,"
      << "harmonic_energy_ratio,harmonic_count,"
      << "octave_1_quality,octave_2_quality,octave_3_quality,"
      << "octave_4_quality\n";

  for (const FrameAnalysis& frame : frames) {
    double octave_quality[4] = {0.0, 0.0, 0.0, 0.0};
    for (int divisor = 1; divisor <= 4; ++divisor) {
      const double candidate_hz =
          frame.yin_pitch_hz / static_cast<double>(divisor);
      if (candidate_hz <= 0.0) {
        continue;
      }

      const tunerlock::feature::HarmonicScore score =
          tunerlock::feature::ComputeHarmonicSumSpectrum(
              frame.magnitudes,
              frame.sample_rate,
              frame.fft_size,
              candidate_hz);
      octave_quality[divisor - 1] =
          score.normalized_score * 0.65 +
          score.harmonic_energy_ratio * 0.35;
    }

    csv << frame.frame_index << ','
        << frame.start_sample << ','
        << frame.start_time_seconds << ','
        << frame.features.yin_pitch_hz << ','
        << frame.features.rms << ','
        << frame.features.peak_amplitude << ','
        << frame.features.fft_peak_frequency_hz << ','
        << frame.features.fft_peak_magnitude << ','
        << frame.features.spectral_centroid_hz << ','
        << frame.features.harmonic_normalized_score << ','
        << frame.features.harmonic_energy_ratio << ','
        << frame.features.harmonic_count << ','
        << octave_quality[0] << ','
        << octave_quality[1] << ','
        << octave_quality[2] << ','
        << octave_quality[3] << '\n';
  }
}

void WriteSvg(
    const std::string& path,
    const std::vector<double>& magnitudes,
    int sample_rate,
    std::size_t fft_size,
    double max_frequency_hz) {
  constexpr int kWidth = 1200;
  constexpr int kHeight = 520;
  constexpr int kLeft = 70;
  constexpr int kRight = 30;
  constexpr int kTop = 30;
  constexpr int kBottom = 55;

  const int plot_width = kWidth - kLeft - kRight;
  const int plot_height = kHeight - kTop - kBottom;
  const std::size_t max_bin = std::min(
      tunerlock::fft::FrequencyToBin(max_frequency_hz, sample_rate, fft_size),
      std::min(magnitudes.size() - 1, fft_size / 2));

  double max_magnitude = 0.0;
  for (std::size_t bin = 0; bin <= max_bin; ++bin) {
    max_magnitude = std::max(max_magnitude, magnitudes[bin]);
  }
  if (max_magnitude <= 0.0) {
    max_magnitude = 1.0;
  }

  std::ofstream svg(path);
  svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << kWidth
      << "\" height=\"" << kHeight << "\" viewBox=\"0 0 " << kWidth << ' '
      << kHeight << "\">\n";
  svg << "<rect width=\"100%\" height=\"100%\" fill=\"#f7f1e3\"/>\n";
  svg << "<text x=\"" << kLeft << "\" y=\"24\" font-size=\"18\" "
      << "font-family=\"Arial\" fill=\"#222\">FFT Magnitude Spectrum</text>\n";
  svg << "<line x1=\"" << kLeft << "\" y1=\"" << (kTop + plot_height)
      << "\" x2=\"" << (kLeft + plot_width) << "\" y2=\""
      << (kTop + plot_height)
      << "\" stroke=\"#333\" stroke-width=\"1\"/>\n";
  svg << "<line x1=\"" << kLeft << "\" y1=\"" << kTop << "\" x2=\""
      << kLeft << "\" y2=\"" << (kTop + plot_height)
      << "\" stroke=\"#333\" stroke-width=\"1\"/>\n";

  for (int tick = 0; tick <= 5; ++tick) {
    const double frequency = max_frequency_hz * static_cast<double>(tick) / 5.0;
    const double x = static_cast<double>(kLeft) +
                     static_cast<double>(plot_width) *
                         (frequency / max_frequency_hz);
    svg << "<line x1=\"" << x << "\" y1=\"" << kTop << "\" x2=\"" << x
        << "\" y2=\"" << (kTop + plot_height)
        << "\" stroke=\"#d5cdb9\" stroke-width=\"1\"/>\n";
    svg << "<text x=\"" << x << "\" y=\"" << (kTop + plot_height + 25)
        << "\" text-anchor=\"middle\" font-size=\"12\" "
        << "font-family=\"Arial\" fill=\"#333\">" << static_cast<int>(frequency)
        << " Hz</text>\n";
  }

  svg << "<polyline fill=\"none\" stroke=\"#1f6f5b\" stroke-width=\"2\" "
      << "points=\"";
  for (std::size_t bin = 0; bin <= max_bin; ++bin) {
    const double frequency =
        tunerlock::fft::BinToFrequency(bin, sample_rate, fft_size);
    const double x = static_cast<double>(kLeft) +
                     static_cast<double>(plot_width) *
                         (frequency / max_frequency_hz);
    const double normalized = magnitudes[bin] / max_magnitude;
    const double y = static_cast<double>(kTop + plot_height) -
                     static_cast<double>(plot_height) * normalized;
    svg << std::fixed << std::setprecision(2) << x << ',' << y << ' ';
  }
  svg << "\"/>\n";

  svg << "<text x=\"" << (kLeft + plot_width / 2) << "\" y=\""
      << (kHeight - 12)
      << "\" text-anchor=\"middle\" font-size=\"13\" font-family=\"Arial\" "
      << "fill=\"#333\">Frequency</text>\n";
  svg << "<text x=\"18\" y=\"" << (kTop + plot_height / 2)
      << "\" transform=\"rotate(-90 18 " << (kTop + plot_height / 2)
      << ")\" text-anchor=\"middle\" font-size=\"13\" "
      << "font-family=\"Arial\" fill=\"#333\">Magnitude</text>\n";
  svg << "</svg>\n";
}

}  // namespace

int main(int argc, char** argv) {
  const std::string wav_path =
      argc > 1 ? argv[1] : "datasets/violin/violin_a.wav";
  const std::string csv_path =
      argc > 2 ? argv[2] : "tracking_engine/build/fft_magnitudes.csv";
  const std::string svg_path =
      argc > 3 ? argv[3] : "tracking_engine/build/fft_magnitudes.svg";
  const std::string harmonic_csv_path =
      argc > 4 ? argv[4] : "tracking_engine/build/harmonic_contributions.csv";
  const std::string frame_features_csv_path =
      argc > 5 ? argv[5] : "tracking_engine/build/frame_features.csv";

  tunerlock::audio::WavReadResult wav =
      tunerlock::audio::ReadWavFile(wav_path);
  if (!wav.ok) {
    std::cerr << wav.error << '\n';
    return 1;
  }

  tunerlock::audio::RemoveDcOffsetInPlace(wav.buffer);
  tunerlock::audio::NormalizeInPlace(wav.buffer);

  constexpr std::size_t kFrameSize = tunerlock::audio::kDefaultFrameSize;
  constexpr std::size_t kHopSize = kFrameSize;
  std::vector<FrameAnalysis> frames;
  for (std::size_t start = 0, frame_index = 0;
       start < wav.buffer.samples.size();
       start += kHopSize, ++frame_index) {
    frames.push_back(AnalyzeFrame(wav.buffer, frame_index, start, kFrameSize));
  }

  if (frames.empty()) {
    std::cerr << "No frames to analyze.\n";
    return 1;
  }

  const FrameAnalysis& first_frame = frames.front();

  WriteCsv(
      csv_path,
      first_frame.magnitudes,
      wav.buffer.sample_rate,
      kFrameSize);
  WriteSvg(
      svg_path,
      first_frame.magnitudes,
      wav.buffer.sample_rate,
      kFrameSize,
      5000.0);
  WriteHarmonicCsv(
      harmonic_csv_path,
      first_frame.harmonic_score,
      wav.buffer.sample_rate,
      kFrameSize);
  WriteFrameFeaturesCsv(frame_features_csv_path, frames);

  std::cout << "csv=" << csv_path << '\n';
  std::cout << "svg=" << svg_path << '\n';
  std::cout << "harmonic_csv=" << harmonic_csv_path << '\n';
  std::cout << "frame_features_csv=" << frame_features_csv_path << '\n';
  std::cout << "frame_count=" << frames.size() << '\n';
  std::cout << "frame_size=" << kFrameSize << '\n';
  std::cout << "hop_size=" << kHopSize << '\n';
  std::cout << "frame.0.yin_pitch_hz=" << first_frame.yin_pitch_hz << '\n';
  std::cout << "frame.0.harmonic.weighted_sum="
            << first_frame.harmonic_score.weighted_sum << '\n';
  std::cout << "harmonic.normalized_score="
            << first_frame.harmonic_score.normalized_score << '\n';
  std::cout << "harmonic.energy_ratio="
            << first_frame.harmonic_score.harmonic_energy_ratio << '\n';
  std::cout << "harmonic.count="
            << first_frame.harmonic_score.harmonic_count << '\n';
  for (const auto& contribution : first_frame.harmonic_score.contributions) {
    std::cout << "harmonic." << contribution.harmonic_number
              << " expected_hz=" << contribution.expected_frequency_hz
              << " bin=" << contribution.bin
              << " bin_hz="
              << tunerlock::fft::BinToFrequency(
                     contribution.bin,
                     wav.buffer.sample_rate,
                     kFrameSize)
              << " magnitude=" << contribution.magnitude
              << " weighted=" << contribution.weighted_magnitude << '\n';
  }

  const std::size_t preview_count = std::min<std::size_t>(frames.size(), 8);
  for (std::size_t i = 0; i < preview_count; ++i) {
    const FrameAnalysis& frame = frames[i];
    std::cout << "frame." << frame.frame_index
              << " time_s=" << frame.start_time_seconds
              << " yin_hz=" << frame.features.yin_pitch_hz
              << " fft_peak_hz=" << frame.features.fft_peak_frequency_hz
              << " harmonic_ratio=" << frame.features.harmonic_energy_ratio
              << " rms=" << frame.features.rms << '\n';
  }
  return 0;
}
