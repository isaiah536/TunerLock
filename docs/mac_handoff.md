# TunerLock Mac Handoff

This file is a handoff note for continuing TunerLock development on macOS.

## Project Layout

Repository root:

```text
TunerLock/
+-- tunerlock/          # Flutter app
+-- tracking_engine/    # C++ DSP / tracking engine
+-- datasets/           # WAV test recordings
+-- tools/              # Python/audio analysis helpers
+-- docs/
+-- README.md
```

Core runtime architecture:

```text
Flutter UI
  ↓
Dart service / FFI bridge
  ↓
C++ TrackingEngine
  ↓
AudioBuffer → Windowing → FFT/YIN → Harmonic → Feature
  ↓
AttackDetector → TargetManager → TrackingLock → Confidence → Kalman/output
```

Profiles:

- `Strings`
- `Wind`
- `Brass`

Modes:

- `Tuning`
- `Performance`

## Current Development Goal

The current goal is to improve pitch tracking for performance mode, especially in noisy or reverberant environments.

Important behavior:

- Output Hz mostly comes from YIN / filtered pitch.
- Lock and switching decisions use harmonic, FFT energy, features, attack, and confidence.
- Tuning mode should be stable and conservative.
- Performance mode should follow note changes faster while avoiding short tail artifacts.

## Recent Changes

### Tracking State

`TrackingState::Transition` was added.

Purpose:

- Represent a short candidate transition period before a new pitch is fully accepted.
- Keep the engine from immediately jumping on a single suspicious frame.

Relevant files:

- `tracking_engine/include/types.h`
- `tracking_engine/src/tracking/tracking_lock.cpp`

### Wind Performance

Wind Performance was improved with:

- Half-hop WAV test processing: frame size remains `2048`, hop is `1024`.
- Stable pitch transition logic.
- Separate transition state.
- Different hysteresis from tuning mode.

Switch logic:

```text
YIN pitch leaves the current note center
AND new pitch remains similar for 2 frames
AND harmonic score is maintained
AND RMS does not sharply drop
→ accept as new-note transition
```

Relevant file:

- `tracking_engine/src/tracking/target_manager.cpp`

### Strings Performance

Strings Performance now also has:

- Half-hop WAV test processing.
- Transition state enabled.
- Wider transition cents than Wind to avoid false switching from vibrato.
- Downward tail guard.
- 1-octave downward hold.

Current Strings Performance transition parameters:

```text
same_target_cents = 45
stable_challenger_cents = 35
challenger_frames = 3
transition_departure_cents = 70
transition_stable_cents = 38
stable_pitch_transition_frames = 2
transition_switch_score_margin = 0.90
```

### Downward Tail Guard

Added to `TrackingEngine::ProcessFrameWithDebug`.

Applies only to:

```text
profile == Strings
mode == Performance
```

Logic:

```text
pitch drop > 700 cents
AND attack_confirmed == false
AND (
  confidence < 0.45
  AND rms < previous_stable_rms * 0.90
  OR
  pitch drop >= 1150 cents
  AND confidence < 0.30
)
→ mute this frame
```

Frame hold logic:

```text
If a non-attack downward jump is >= 1150 cents,
hold it for 2 frames.
If it remains stable, allow it from the 3rd frame.
```

This reduced tail artifacts in `violin_scale.wav`.

Relevant files:

- `tracking_engine/include/tracking_engine.h`
- `tracking_engine/src/api/tracking_engine_api.cpp`

### WAV Analyzer

`engine_wav_analyzer` now:

- Uses hop `1024` for `Strings + Performance`.
- Uses hop `1024` for `Wind + Performance`.
- Keeps hop `2048` for other profiles/modes.
- Accepts optional A4 reference pitch as the 6th argument.
- Writes debug columns:
  - `rms`
  - `attack_confirmed`
  - `downward_pitch_drop_cents`
  - `downward_tail`
  - `downward_hold`
  - `pending_downward_frame_count`

Relevant file:

- `tracking_engine/tools/engine_wav_analyzer.cpp`

## Recent Test Results

Validated on Windows with Visual Studio CMake/NMake.

C++ test status:

```text
ctest: 12/12 passed
```

Important WAV test:

```text
datasets/violin/violin_scale.wav
```

Latest output:

```text
CSV: tracking_engine/build/violin_scale_strings_performance_downward_hold.csv
mode=performance
profile=strings
reference_pitch_hz=440.000000
hop_size=1024
frame_count=969
locked_frame_count=841
confidence.mean=0.889725
```

Result:

- Before downward hold, the final `A2` tail appeared briefly.
- After downward hold, the visible `A2` tail segment was removed from the segment summary.
- Some short high-register tail fragments may still exist earlier in the tail, but the large downward octave tail is much cleaner.

## Important Caveat

The half-hop behavior is currently implemented in the WAV analyzer, not yet in the real microphone stream.

For the actual app to match the WAV test responsiveness, add a 50% overlap frame buffer in the live audio input path before sending frames to the C++ engine.

Likely Flutter-side areas:

- `tunerlock/lib/services/tracking_engine_service.dart`
- `tunerlock/lib/services/audio/*`
- native microphone bridge code under `ios/` or `android/`

## Mac Continuation Tasks

Start by pulling the latest repository state.

Then run:

```bash
cd TunerLock

cmake -S tracking_engine -B tracking_engine/build
cmake --build tracking_engine/build
ctest --test-dir tracking_engine/build --output-on-failure

cd tunerlock
flutter pub get
flutter analyze
flutter build ios --no-codesign
```

If `flutter build ios --no-codesign` succeeds, next tasks are:

1. Confirm iOS FFI library packaging.
2. Confirm microphone permission and live audio frame delivery on iOS.
3. Implement or verify 50% overlap frames for live performance mode.
4. Connect UI profile selection to engine profile selection.
5. Test live `Strings + Performance` on iPhone with violin recordings or real input.

## Useful Commands

Windows C++ build command used before handoff:

```powershell
& cmd /c '"C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 && cmake -S tracking_engine -B tracking_engine\build -G "NMake Makefiles" && cmake --build tracking_engine\build && ctest --test-dir tracking_engine\build --output-on-failure'
```

WAV analyzer examples:

```bash
tracking_engine/build/engine_wav_analyzer \
  datasets/violin/violin_scale.wav \
  tracking_engine/build/violin_scale_strings_performance_downward_hold.csv \
  performance \
  strings

tracking_engine/build/engine_wav_analyzer \
  datasets/clarinet/clarinet\ c\ scale.wav \
  tracking_engine/build/clarinet_c_scale_wind_performance_transition.csv \
  performance \
  wind
```

Reference pitch example:

```bash
tracking_engine/build/engine_wav_analyzer \
  datasets/violin/violin_mzt_test.wav \
  tracking_engine/build/violin_mzt_test_strings_performance_ref442.csv \
  performance \
  strings \
  442
```

## Suggested First Prompt For Mac Codex

```text
This is the TunerLock project continued from Windows.
Please read docs/mac_handoff.md first, then inspect the repo state.
The next goal is to build on macOS/iOS, verify Flutter iOS FFI packaging, and then prepare live microphone 50% overlap for performance mode.
Do not undo existing tracking_engine changes.
```
