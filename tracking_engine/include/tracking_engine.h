#pragma once

#include "attack_detector.h"
#include "confidence.h"
#include "instrument_profile.h"
#include "kalman.h"
#include "target_manager.h"
#include "tracking_lock.h"
#include "types.h"

namespace tunerlock {

class TrackingEngine {
 public:
  explicit TrackingEngine(
      tracking::InstrumentProfile profile =
          tracking::InstrumentProfile::Strings,
      tracking::TrackingMode mode = tracking::TrackingMode::Tuning);

  TrackingResult ProcessFrame(const float* samples, int sample_count, int sample_rate);
  TrackingResult ProcessFrameWithDebug(
      const float* samples,
      int sample_count,
      int sample_rate,
      DebugTrace* trace);

  void Reset();
  void SetReferencePitchHz(double reference_pitch_hz);
  void SetTrackingMode(tracking::TrackingMode mode);
  double reference_pitch_hz() const;
  tracking::TrackingMode tracking_mode() const;

 private:
  tracking::InstrumentProfile profile_;
  tracking::TrackingMode mode_;
  tracking::AttackDetector attack_detector_;
  tracking::TargetManager target_manager_;
  tracking::TrackingLock tracking_lock_;
  tracking::ConfidenceCalculator confidence_calculator_;
  filter::PitchKalmanFilter pitch_filter_;
  double reference_pitch_hz_ = 440.0;
  double low_string_reference_hz_ = 0.0;
  int performance_snap_frames_ = 0;
  int displayed_midi_note_ = -1;
  int candidate_midi_note_ = -1;
  int candidate_note_frames_ = 0;
  double previous_output_frequency_hz_ = 0.0;
  double previous_output_rms_ = 0.0;
  double pending_downward_frequency_hz_ = 0.0;
  int pending_downward_frame_count_ = 0;
};

}  // namespace tunerlock
