#pragma once

#include <cstddef>

#if defined(_WIN32)
#define TUNERLOCK_API extern "C" __declspec(dllexport)
#else
#define TUNERLOCK_API extern "C" __attribute__((visibility("default"))) __attribute__((used))
#endif

TUNERLOCK_API void* tunerlock_engine_create(int instrument_profile);
TUNERLOCK_API void tunerlock_engine_destroy(void* handle);
TUNERLOCK_API void tunerlock_engine_reset(void* handle);
TUNERLOCK_API void tunerlock_engine_set_reference_pitch(
    void* handle,
    double reference_pitch_hz);
TUNERLOCK_API void tunerlock_engine_set_tracking_mode(
    void* handle,
    int tracking_mode);

TUNERLOCK_API float* tunerlock_samples_allocate(int sample_count);
TUNERLOCK_API void tunerlock_samples_free(float* samples);

TUNERLOCK_API int tunerlock_engine_process(
    void* handle,
    const float* samples,
    int sample_count,
    int sample_rate);
TUNERLOCK_API double tunerlock_engine_frequency_hz(void* handle);
TUNERLOCK_API double tunerlock_engine_target_frequency_hz(void* handle);
TUNERLOCK_API double tunerlock_engine_cents(void* handle);
TUNERLOCK_API double tunerlock_engine_confidence(void* handle);
TUNERLOCK_API int tunerlock_engine_midi_note(void* handle);
TUNERLOCK_API int tunerlock_engine_locked(void* handle);
