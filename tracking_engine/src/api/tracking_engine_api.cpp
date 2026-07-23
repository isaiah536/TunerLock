#include "audio_buffer.h"
#include "attack_detector.h"
#include "confidence.h"
#include "feature_extractor.h"
#include "fft.h"
#include "harmonic.h"
#include "target_manager.h"
#include "tracking_lock.h"
#include "tracking_engine.h"
#include "window.h"
#include "yin.h"

#include <vector>
#include <cmath>

namespace tunerlock {
namespace {

int NearestMidiNote(double frequency_hz, double reference_pitch_hz) {
  return static_cast<int>(std::round(
      69.0 + 12.0 * std::log2(frequency_hz / reference_pitch_hz)));
}

double MidiNoteFrequency(int midi_note, double reference_pitch_hz) {
  return reference_pitch_hz *
      std::exp2((static_cast<double>(midi_note) - 69.0) / 12.0);
}

double HarmonicQuality(const feature::HarmonicScore& score) {
  if (score.harmonic_count <= 0) {
    return 0.0;
  }

  return score.normalized_score * 0.65 + score.harmonic_energy_ratio * 0.35;
}

double FirstHarmonicMagnitude(const feature::HarmonicScore& score) {
  for (const auto& contribution : score.contributions) {
    if (contribution.harmonic_number == 1) {
      return contribution.magnitude;
    }
  }
  return 0.0;
}

bool IsNearRatio(double high_frequency_hz, double low_frequency_hz, int ratio) {
  if (high_frequency_hz <= 0.0 || low_frequency_hz <= 0.0 || ratio <= 1) {
    return false;
  }

  const double expected = low_frequency_hz * static_cast<double>(ratio);
  const double cents = std::abs(1200.0 * std::log2(high_frequency_hz / expected));
  return cents <= 45.0;
}

double PitchDistanceCents(double first_hz, double second_hz) {
  if (first_hz <= 0.0 || second_hz <= 0.0) {
    return INFINITY;
  }

  return std::abs(1200.0 * std::log2(first_hz / second_hz));
}

double DownwardPitchDropCents(
    double previous_frequency_hz,
    double current_frequency_hz) {
  if (previous_frequency_hz <= 0.0 ||
      current_frequency_hz <= 0.0 ||
      current_frequency_hz >= previous_frequency_hz) {
    return 0.0;
  }

  return 1200.0 * std::log2(previous_frequency_hz / current_frequency_hz);
}

pitch::YinResult CorrectStringsOctaveCandidate(
    const pitch::YinResult& raw_yin,
    const std::vector<double>& magnitudes,
    int sample_rate,
    std::size_t fft_size,
    double previous_target_hz,
    feature::HarmonicScore* harmonic_score) {
  if (!raw_yin.detected || raw_yin.frequency_hz <= 0.0 ||
      magnitudes.empty() || harmonic_score == nullptr) {
    return raw_yin;
  }

  constexpr double kCelloLowHz = 45.0;
  constexpr double kCelloLowModeMaxHz = 145.0;
  constexpr double kCelloLowCandidateMaxHz = 155.0;
  constexpr double kMinUsefulHarmonicRatio = 0.06;
  constexpr double kMaxPreviousTargetCents = 850.0;

  pitch::YinResult best_yin = raw_yin;
  feature::HarmonicScore best_score = *harmonic_score;
  double best_quality = HarmonicQuality(best_score);

  if (previous_target_hz >= kCelloLowHz &&
      previous_target_hz <= kCelloLowModeMaxHz &&
      raw_yin.frequency_hz >= previous_target_hz * 2.2) {
    bool found_low_string_candidate = false;
    double best_low_string_cost = 0.0;

    for (int divisor = 2; divisor <= 6; ++divisor) {
      const double candidate_hz =
          raw_yin.frequency_hz / static_cast<double>(divisor);
      if (candidate_hz < kCelloLowHz ||
          candidate_hz > kCelloLowCandidateMaxHz ||
          candidate_hz < previous_target_hz * 0.92) {
        continue;
      }

      feature::HarmonicScore candidate_score =
          feature::ComputeHarmonicSumSpectrum(
              magnitudes,
              sample_rate,
              fft_size,
              candidate_hz);
      const double candidate_quality = HarmonicQuality(candidate_score);
      if (candidate_score.harmonic_count < 4 ||
          candidate_score.harmonic_energy_ratio < 0.03 ||
          candidate_quality < best_quality * 0.15) {
        continue;
      }

      const double cents_from_previous =
          1200.0 * std::log2(candidate_hz / previous_target_hz);
      const double cost =
          cents_from_previous >= 0.0
              ? cents_from_previous
              : std::abs(cents_from_previous) + 600.0;
      if (!found_low_string_candidate || cost < best_low_string_cost) {
        found_low_string_candidate = true;
        best_low_string_cost = cost;
        best_yin.frequency_hz = candidate_hz;
        best_score = candidate_score;
      }
    }

    if (found_low_string_candidate) {
      *harmonic_score = best_score;
      return best_yin;
    }
  }

  for (int divisor = 2; divisor <= 4; ++divisor) {
    const double candidate_hz =
        raw_yin.frequency_hz / static_cast<double>(divisor);
    if (candidate_hz < kCelloLowHz) {
      continue;
    }

    feature::HarmonicScore candidate_score =
        feature::ComputeHarmonicSumSpectrum(
            magnitudes,
            sample_rate,
            fft_size,
            candidate_hz);
    const double candidate_quality = HarmonicQuality(candidate_score);
    if (candidate_score.harmonic_count < 4 ||
        candidate_score.harmonic_energy_ratio < kMinUsefulHarmonicRatio) {
      continue;
    }

    const bool near_previous_target =
        previous_target_hz > 0.0 &&
        std::abs(
            1200.0 * std::log2(candidate_hz / previous_target_hz)) <=
            kMaxPreviousTargetCents;
    const bool candidate_has_low_support =
        FirstHarmonicMagnitude(candidate_score) >
        FirstHarmonicMagnitude(best_score) * 0.12;
    const bool candidate_is_raw_subharmonic =
        IsNearRatio(raw_yin.frequency_hz, candidate_hz, divisor);

    const double required_quality_ratio =
        near_previous_target ? 0.52 : 0.74;
    if (candidate_is_raw_subharmonic &&
        candidate_has_low_support &&
        candidate_quality >= best_quality * required_quality_ratio) {
      best_yin.frequency_hz = candidate_hz;
      best_score = candidate_score;
      best_quality = candidate_quality;
    }
  }

  *harmonic_score = best_score;
  return best_yin;
}

}  // namespace

TrackingEngine::TrackingEngine(
    tracking::InstrumentProfile profile,
    tracking::TrackingMode mode)
    : profile_(profile),
      mode_(mode),
      attack_detector_(profile),
      target_manager_(profile, mode),
      tracking_lock_(),
      confidence_calculator_(profile) {}

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
  const std::vector<float> level_input =
      audio::CopySamplesForWindowing(buffer);
  audio::NormalizeInPlace(buffer);
  if (trace != nullptr) {
    trace->Add("audio_buffer", "peak_after", audio::PeakAmplitude(buffer));
    trace->Add("audio_buffer", "rms_after", audio::ComputeRms(buffer));
  }

  std::vector<float> analysis_input = audio::CopySamplesForWindowing(buffer);
  std::vector<float> window_input = analysis_input;
  std::vector<float> window = audio::HannWindow(window_input.size());
  audio::ApplyWindowInPlace(window_input, window);

  const float rms = audio::ComputeRms(
      AudioBuffer{window_input, buffer.sample_rate});

  const auto spectrum = fft::DiscreteFourierTransform(window_input);
  const std::vector<double> magnitudes = fft::Magnitudes(spectrum);
  const std::size_t peak_bin = fft::FindPeakBin(magnitudes);
  pitch::YinResult yin = pitch::AnalyzePitchYin(
      analysis_input.data(),
      static_cast<int>(analysis_input.size()),
      buffer.sample_rate);

  feature::HarmonicScore harmonic_score;
  if (yin.frequency_hz > 0.0) {
    harmonic_score = feature::ComputeHarmonicSumSpectrum(
        magnitudes,
        buffer.sample_rate,
        window_input.size(),
        yin.frequency_hz);
    if (profile_ == tracking::InstrumentProfile::Strings &&
        mode_ == tracking::TrackingMode::Tuning) {
      const double previous_low_string_reference =
          target_manager_.has_target()
              ? target_manager_.target_frequency_hz()
              : low_string_reference_hz_;
      yin = CorrectStringsOctaveCandidate(
          yin,
          magnitudes,
          buffer.sample_rate,
          window_input.size(),
          previous_low_string_reference,
          &harmonic_score);
    }
  }
  const double yin_pitch = yin.frequency_hz;

  const feature::FeatureVector features = feature::ExtractFeatures(
      feature::FeatureInput{
          level_input.data(),
          level_input.size(),
          buffer.sample_rate,
          &magnitudes,
          window_input.size(),
          yin_pitch,
          harmonic_score,
      });
  const tracking::AttackResult attack =
      attack_detector_.Process(features);

  fft::TargetBandEnergy locked_target_energy;
  if (target_manager_.has_target()) {
    locked_target_energy = fft::ComputeTargetBandEnergy(
        magnitudes,
        buffer.sample_rate,
        window_input.size(),
        target_manager_.target_frequency_hz());
  }
  const double locked_target_energy_ratio = locked_target_energy.ratio;

  const tracking::TargetDecision target_decision =
      target_manager_.Process(tracking::TargetObservation{
          features,
          attack.confirmed,
          locked_target_energy_ratio,
          locked_target_energy.fundamental_ratio,
          locked_target_energy.strongest_harmonic_ratio,
          mode_ == tracking::TrackingMode::Tuning,
      });
  const tracking::TrackingLockResult lock_result =
      tracking_lock_.Process(target_decision);
  const tracking::ConfidenceResult confidence =
      confidence_calculator_.Process(tracking::ConfidenceInput{
          yin,
          features,
          locked_target_energy_ratio,
          lock_result.state,
      });

  if (lock_result.target_changed) {
    pitch_filter_.Reset();
  }
  double filtered_frequency_hz = 0.0;
  const bool brass_performance =
      profile_ == tracking::InstrumentProfile::Brass &&
      mode_ == tracking::TrackingMode::Performance;
  if (brass_performance && yin_pitch > 0.0) {
    if (attack.confirmed || lock_result.target_changed) {
      performance_snap_frames_ = 2;
    }

    const bool high_confidence = confidence.value >= 0.85;
    if (high_confidence || performance_snap_frames_ > 0) {
      pitch_filter_.Reset();
      filtered_frequency_hz = pitch_filter_.Process(yin_pitch, 1.0);
      if (performance_snap_frames_ > 0) {
        --performance_snap_frames_;
      }
    } else {
      filtered_frequency_hz =
          pitch_filter_.Process(yin_pitch, confidence.value);
    }
  } else if (mode_ == tracking::TrackingMode::Performance && yin_pitch > 0.0) {
    filtered_frequency_hz =
        pitch_filter_.Process(yin_pitch, confidence.value);
  } else if (lock_result.locked) {
    filtered_frequency_hz =
        pitch_filter_.Process(lock_result.frequency_hz, confidence.value);
  } else if (lock_result.state == TrackingState::Recovering) {
    filtered_frequency_hz = pitch_filter_.Process(0.0, confidence.value);
  } else {
    pitch_filter_.Reset();
  }

  double output_confidence = confidence.value;
  bool output_locked = lock_result.locked;
  const bool strings_performance =
      profile_ == tracking::InstrumentProfile::Strings &&
      mode_ == tracking::TrackingMode::Performance;
  const double downward_pitch_drop_cents =
      DownwardPitchDropCents(
          previous_output_frequency_hz_,
          filtered_frequency_hz);
  bool downward_hold = false;
  const bool octave_downward_candidate =
      strings_performance &&
      !attack.confirmed &&
      downward_pitch_drop_cents >= 1150.0;
  if (octave_downward_candidate) {
    if (pending_downward_frame_count_ == 0 ||
        PitchDistanceCents(
            filtered_frequency_hz,
            pending_downward_frequency_hz_) > 80.0) {
      pending_downward_frequency_hz_ = filtered_frequency_hz;
      pending_downward_frame_count_ = 1;
    } else {
      ++pending_downward_frame_count_;
    }
    downward_hold = pending_downward_frame_count_ < 3;
  } else if (filtered_frequency_hz > 0.0) {
    pending_downward_frequency_hz_ = 0.0;
    pending_downward_frame_count_ = 0;
  }

  const bool downward_tail =
      strings_performance &&
      !attack.confirmed &&
      downward_pitch_drop_cents > 700.0 &&
      ((confidence.value < 0.45 &&
        previous_output_rms_ > 0.0 &&
        features.rms < previous_output_rms_ * 0.90) ||
       (downward_pitch_drop_cents >= 1150.0 &&
        confidence.value < 0.30));
  if (downward_tail || downward_hold) {
    filtered_frequency_hz = 0.0;
    output_confidence = 0.0;
    output_locked = false;
  }

  if (filtered_frequency_hz > 0.0) {
    if (profile_ == tracking::InstrumentProfile::Strings &&
        mode_ == tracking::TrackingMode::Tuning &&
        filtered_frequency_hz >= 45.0 &&
        filtered_frequency_hz <= 145.0) {
      low_string_reference_hz_ = filtered_frequency_hz;
    }

    const int nearest_note =
        NearestMidiNote(filtered_frequency_hz, reference_pitch_hz_);
    if (displayed_midi_note_ < 0) {
      displayed_midi_note_ = nearest_note;
    } else if (nearest_note == displayed_midi_note_) {
      candidate_midi_note_ = -1;
      candidate_note_frames_ = 0;
    } else if (nearest_note == candidate_midi_note_) {
      ++candidate_note_frames_;
      const bool wind_performance =
          profile_ == tracking::InstrumentProfile::Wind &&
          mode_ == tracking::TrackingMode::Performance;
      const int required_candidate_frames =
          mode_ == tracking::TrackingMode::Performance
              ? (wind_performance ? 2 : 1)
              : 3;
      if (candidate_note_frames_ >= required_candidate_frames) {
        displayed_midi_note_ = candidate_midi_note_;
        candidate_midi_note_ = -1;
        candidate_note_frames_ = 0;
      }
    } else {
      candidate_midi_note_ = nearest_note;
      candidate_note_frames_ = 1;
      const bool immediate_performance_note =
          mode_ == tracking::TrackingMode::Performance &&
          profile_ != tracking::InstrumentProfile::Wind;
      if (immediate_performance_note) {
        displayed_midi_note_ = nearest_note;
        candidate_midi_note_ = -1;
        candidate_note_frames_ = 0;
      }
    }
  }

  const double target_frequency_hz =
      displayed_midi_note_ >= 0
          ? MidiNoteFrequency(displayed_midi_note_, reference_pitch_hz_)
          : 0.0;
  const double cents =
      filtered_frequency_hz > 0.0 && target_frequency_hz > 0.0
          ? 1200.0 *
              std::log2(filtered_frequency_hz / target_frequency_hz)
          : 0.0;

  if (trace != nullptr) {
    trace->Add("windowing", "window_size", window_input.size());
    trace->Add("windowing", "rms_after", rms);

    trace->Add("fft", "bin_count", magnitudes.size());
    trace->Add("fft", "peak_bin", peak_bin);
    trace->Add(
        "fft",
        "peak_frequency_hz",
        fft::BinToFrequency(peak_bin, buffer.sample_rate, window_input.size()));

    trace->Add("yin", "pitch_hz", yin_pitch);
    trace->Add("yin", "periodicity", yin.periodicity);
    trace->Add("yin", "cmndf_value", yin.cmndf_value);

    if (yin_pitch > 0.0) {
      trace->Add("harmonic", "weighted_sum", harmonic_score.weighted_sum);
      trace->Add(
          "harmonic",
          "normalized_score",
          harmonic_score.normalized_score);
      trace->Add(
          "harmonic",
          "energy_ratio",
          harmonic_score.harmonic_energy_ratio);
      trace->Add("harmonic", "count", harmonic_score.harmonic_count);

      trace->Add("feature", "rms", features.rms);
      trace->Add("feature", "peak_amplitude", features.peak_amplitude);
      trace->Add("feature", "yin_pitch_hz", features.yin_pitch_hz);
      trace->Add("feature", "yin_detected", features.yin_detected ? 1.0 : 0.0);
      trace->Add(
          "feature",
          "fft_peak_frequency_hz",
          features.fft_peak_frequency_hz);
      trace->Add(
          "feature",
          "spectral_centroid_hz",
          features.spectral_centroid_hz);
      trace->Add(
          "feature",
          "harmonic_normalized_score",
          features.harmonic_normalized_score);
      trace->Add(
          "feature",
          "harmonic_energy_ratio",
          features.harmonic_energy_ratio);
    }
    trace->Add(
        "tracking",
        "attack_candidate",
        attack.state == tracking::AttackState::Candidate ? 1.0 : 0.0);
    trace->Add(
        "tracking", "attack_confirmed", attack.confirmed ? 1.0 : 0.0);
    trace->Add(
        "tracking",
        "locked_target_energy_ratio",
        locked_target_energy_ratio);
    trace->Add(
        "tracking",
        "locked_target_fundamental_ratio",
        locked_target_energy.fundamental_ratio);
    trace->Add(
        "tracking",
        "locked_target_strongest_harmonic_ratio",
        locked_target_energy.strongest_harmonic_ratio);
    trace->Add(
        "tracking",
        "target_decision",
        static_cast<double>(target_decision.type));
    trace->Add(
        "tracking", "state", static_cast<double>(lock_result.state));
    trace->Add(
        "tracking", "locked", lock_result.locked ? 1.0 : 0.0);
    trace->Add("confidence", "value", confidence.value);
    trace->Add("confidence", "yin", confidence.yin_score);
    trace->Add(
        "confidence", "pitch_stability", confidence.pitch_stability);
    trace->Add("confidence", "harmonic", confidence.harmonic_score);
    trace->Add(
        "confidence", "target_energy", confidence.target_energy_score);
    trace->Add(
        "confidence", "persistence", confidence.persistence_score);
    trace->Add("output", "filtered_frequency_hz", filtered_frequency_hz);
    trace->Add(
        "output",
        "downward_pitch_drop_cents",
        downward_pitch_drop_cents);
    trace->Add("output", "downward_tail", downward_tail ? 1.0 : 0.0);
    trace->Add("output", "downward_hold", downward_hold ? 1.0 : 0.0);
    trace->Add(
        "output",
        "pending_downward_frame_count",
        pending_downward_frame_count_);
    trace->Add("output", "target_frequency_hz", target_frequency_hz);
    trace->Add("output", "midi_note", displayed_midi_note_);
    trace->Add("output", "cents", cents);
    trace->Add("output", "reference_pitch_hz", reference_pitch_hz_);
  }

  const bool stable_output_level =
      previous_output_rms_ <= 0.0 ||
      features.rms >= previous_output_rms_ * 0.90;
  if (filtered_frequency_hz > 0.0 &&
      output_confidence >= 0.45 &&
      stable_output_level) {
    previous_output_frequency_hz_ = filtered_frequency_hz;
    previous_output_rms_ = features.rms;
  }

  return TrackingResult{
      filtered_frequency_hz,
      target_frequency_hz,
      cents,
      output_confidence,
      displayed_midi_note_,
      output_locked,
  };
}

void TrackingEngine::Reset() {
  attack_detector_.Reset();
  target_manager_.Reset();
  tracking_lock_.Reset();
  confidence_calculator_.Reset();
  pitch_filter_.Reset();
  low_string_reference_hz_ = 0.0;
  performance_snap_frames_ = 0;
  displayed_midi_note_ = -1;
  candidate_midi_note_ = -1;
  candidate_note_frames_ = 0;
  previous_output_frequency_hz_ = 0.0;
  previous_output_rms_ = 0.0;
  pending_downward_frequency_hz_ = 0.0;
  pending_downward_frame_count_ = 0;
}

void TrackingEngine::SetReferencePitchHz(double reference_pitch_hz) {
  if (reference_pitch_hz > 0.0 && std::isfinite(reference_pitch_hz)) {
    reference_pitch_hz_ = reference_pitch_hz;
  }
}

void TrackingEngine::SetTrackingMode(tracking::TrackingMode mode) {
  if (mode_ != mode) {
    mode_ = mode;
    target_manager_.Configure(
        tracking::TargetConfigForProfile(profile_, mode_));
    Reset();
  }
}

double TrackingEngine::reference_pitch_hz() const {
  return reference_pitch_hz_;
}

tracking::TrackingMode TrackingEngine::tracking_mode() const {
  return mode_;
}

}  // namespace tunerlock
