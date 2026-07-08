#include "audio_buffer.h"
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

  tunerlock::audio::WavReadResult wav =
      tunerlock::audio::ReadWavFile(wav_path);
  if (!wav.ok) {
    std::cerr << wav.error << '\n';
    return 1;
  }

  tunerlock::audio::RemoveDcOffsetInPlace(wav.buffer);
  tunerlock::audio::NormalizeInPlace(wav.buffer);

  std::vector<float> window_input =
      tunerlock::audio::CopySamplesForWindowing(wav.buffer);
  const std::vector<float> window =
      tunerlock::audio::HannWindow(window_input.size());
  tunerlock::audio::ApplyWindowInPlace(window_input, window);

  const auto spectrum =
      tunerlock::fft::DiscreteFourierTransform(window_input);
  const std::vector<double> magnitudes =
      tunerlock::fft::Magnitudes(spectrum);

  const double pitch_hz = tunerlock::pitch::EstimatePitchYin(
      wav.buffer.samples.data(),
      static_cast<int>(wav.buffer.samples.size()),
      wav.buffer.sample_rate);
  const tunerlock::feature::HarmonicScore harmonic_score =
      tunerlock::feature::ComputeHarmonicSumSpectrum(
          magnitudes,
          wav.buffer.sample_rate,
          window_input.size(),
          pitch_hz);

  WriteCsv(csv_path, magnitudes, wav.buffer.sample_rate, window_input.size());
  WriteSvg(svg_path, magnitudes, wav.buffer.sample_rate, window_input.size(), 5000.0);
  WriteHarmonicCsv(
      harmonic_csv_path,
      harmonic_score,
      wav.buffer.sample_rate,
      window_input.size());

  std::cout << "csv=" << csv_path << '\n';
  std::cout << "svg=" << svg_path << '\n';
  std::cout << "harmonic_csv=" << harmonic_csv_path << '\n';
  std::cout << "yin_pitch_hz=" << pitch_hz << '\n';
  std::cout << "harmonic.weighted_sum=" << harmonic_score.weighted_sum << '\n';
  std::cout << "harmonic.normalized_score="
            << harmonic_score.normalized_score << '\n';
  std::cout << "harmonic.energy_ratio="
            << harmonic_score.harmonic_energy_ratio << '\n';
  std::cout << "harmonic.count=" << harmonic_score.harmonic_count << '\n';
  for (const auto& contribution : harmonic_score.contributions) {
    std::cout << "harmonic." << contribution.harmonic_number
              << " expected_hz=" << contribution.expected_frequency_hz
              << " bin=" << contribution.bin
              << " bin_hz="
              << tunerlock::fft::BinToFrequency(
                     contribution.bin,
                     wav.buffer.sample_rate,
                     window_input.size())
              << " magnitude=" << contribution.magnitude
              << " weighted=" << contribution.weighted_magnitude << '\n';
  }
  return 0;
}
