#include "tracking_engine_c_api.h"

#include "instrument_profile.h"
#include "tracking_engine.h"

#include <new>

namespace {

struct EngineHandle {
  explicit EngineHandle(tunerlock::tracking::InstrumentProfile profile)
      : engine(profile) {}

  tunerlock::TrackingEngine engine;
  tunerlock::TrackingResult latest;
};

tunerlock::tracking::InstrumentProfile ToProfile(int profile) {
  switch (profile) {
    case 1:
      return tunerlock::tracking::InstrumentProfile::Wind;
    case 2:
      return tunerlock::tracking::InstrumentProfile::Brass;
    default:
      return tunerlock::tracking::InstrumentProfile::Strings;
  }
}

tunerlock::tracking::TrackingMode ToTrackingMode(int mode) {
  switch (mode) {
    case 1:
      return tunerlock::tracking::TrackingMode::Performance;
    default:
      return tunerlock::tracking::TrackingMode::Tuning;
  }
}

EngineHandle* AsHandle(void* handle) {
  return static_cast<EngineHandle*>(handle);
}

}  // namespace

void* tunerlock_engine_create(int instrument_profile) {
  return new (std::nothrow) EngineHandle(ToProfile(instrument_profile));
}

void tunerlock_engine_destroy(void* handle) {
  delete AsHandle(handle);
}

void tunerlock_engine_reset(void* handle) {
  if (auto* engine = AsHandle(handle)) {
    engine->engine.Reset();
    engine->latest = {};
  }
}

void tunerlock_engine_set_reference_pitch(
    void* handle,
    double reference_pitch_hz) {
  if (auto* engine = AsHandle(handle)) {
    engine->engine.SetReferencePitchHz(reference_pitch_hz);
  }
}

void tunerlock_engine_set_tracking_mode(void* handle, int tracking_mode) {
  if (auto* engine = AsHandle(handle)) {
    engine->engine.SetTrackingMode(ToTrackingMode(tracking_mode));
    engine->latest = {};
  }
}

float* tunerlock_samples_allocate(int sample_count) {
  if (sample_count <= 0) {
    return nullptr;
  }
  return new (std::nothrow) float[static_cast<std::size_t>(sample_count)];
}

void tunerlock_samples_free(float* samples) {
  delete[] samples;
}

int tunerlock_engine_process(
    void* handle,
    const float* samples,
    int sample_count,
    int sample_rate) {
  auto* engine = AsHandle(handle);
  if (engine == nullptr || samples == nullptr ||
      sample_count <= 0 || sample_rate <= 0) {
    return 0;
  }

  engine->latest =
      engine->engine.ProcessFrame(samples, sample_count, sample_rate);
  return 1;
}

double tunerlock_engine_frequency_hz(void* handle) {
  const auto* engine = AsHandle(handle);
  return engine != nullptr ? engine->latest.frequency_hz : 0.0;
}

double tunerlock_engine_target_frequency_hz(void* handle) {
  const auto* engine = AsHandle(handle);
  return engine != nullptr ? engine->latest.target_frequency_hz : 0.0;
}

double tunerlock_engine_cents(void* handle) {
  const auto* engine = AsHandle(handle);
  return engine != nullptr ? engine->latest.cents : 0.0;
}

double tunerlock_engine_confidence(void* handle) {
  const auto* engine = AsHandle(handle);
  return engine != nullptr ? engine->latest.confidence : 0.0;
}

int tunerlock_engine_midi_note(void* handle) {
  const auto* engine = AsHandle(handle);
  return engine != nullptr ? engine->latest.midi_note : -1;
}

int tunerlock_engine_locked(void* handle) {
  const auto* engine = AsHandle(handle);
  return engine != nullptr && engine->latest.locked ? 1 : 0;
}
