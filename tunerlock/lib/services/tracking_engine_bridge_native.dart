import 'dart:ffi';

import 'audio/audio_pcm_frame.dart';
import 'tracking_engine_bridge.dart';

typedef _CreateNative = Pointer<Void> Function(Int32);
typedef _CreateDart = Pointer<Void> Function(int);
typedef _DestroyNative = Void Function(Pointer<Void>);
typedef _DestroyDart = void Function(Pointer<Void>);
typedef _SetReferenceNative = Void Function(Pointer<Void>, Double);
typedef _SetReferenceDart = void Function(Pointer<Void>, double);
typedef _AllocateNative = Pointer<Float> Function(Int32);
typedef _AllocateDart = Pointer<Float> Function(int);
typedef _FreeNative = Void Function(Pointer<Float>);
typedef _FreeDart = void Function(Pointer<Float>);
typedef _ProcessNative =
    Int32 Function(Pointer<Void>, Pointer<Float>, Int32, Int32);
typedef _ProcessDart =
    int Function(Pointer<Void>, Pointer<Float>, int, int);
typedef _FrequencyNative = Double Function(Pointer<Void>);
typedef _FrequencyDart = double Function(Pointer<Void>);
typedef _IntegerResultNative = Int32 Function(Pointer<Void>);
typedef _IntegerResultDart = int Function(Pointer<Void>);

TrackingEngineBridge? createTrackingEngineBridge() {
  try {
    return _NativeTrackingEngineBridge(DynamicLibrary.process());
  } on ArgumentError {
    return null;
  }
}

final class _NativeTrackingEngineBridge implements TrackingEngineBridge {
  _NativeTrackingEngineBridge(DynamicLibrary library)
      : _destroy = library
            .lookupFunction<_DestroyNative, _DestroyDart>(
              'tunerlock_engine_destroy',
            ),
        _setReference = library
            .lookupFunction<_SetReferenceNative, _SetReferenceDart>(
              'tunerlock_engine_set_reference_pitch',
            ),
        _allocate = library.lookupFunction<_AllocateNative, _AllocateDart>(
          'tunerlock_samples_allocate',
        ),
        _free = library.lookupFunction<_FreeNative, _FreeDart>(
          'tunerlock_samples_free',
        ),
        _process = library.lookupFunction<_ProcessNative, _ProcessDart>(
          'tunerlock_engine_process',
        ),
        _frequency = library.lookupFunction<_FrequencyNative, _FrequencyDart>(
          'tunerlock_engine_frequency_hz',
        ),
        _targetFrequency =
            library.lookupFunction<_FrequencyNative, _FrequencyDart>(
              'tunerlock_engine_target_frequency_hz',
            ),
        _cents = library.lookupFunction<_FrequencyNative, _FrequencyDart>(
          'tunerlock_engine_cents',
        ),
        _confidence = library.lookupFunction<_FrequencyNative, _FrequencyDart>(
          'tunerlock_engine_confidence',
        ),
        _midiNote =
            library.lookupFunction<_IntegerResultNative, _IntegerResultDart>(
              'tunerlock_engine_midi_note',
            ),
        _locked =
            library.lookupFunction<_IntegerResultNative, _IntegerResultDart>(
              'tunerlock_engine_locked',
            ),
        _handle = library.lookupFunction<_CreateNative, _CreateDart>(
          'tunerlock_engine_create',
        )(0) {
    if (_handle == nullptr) {
      throw StateError('Tracking engine could not be created.');
    }
  }

  final Pointer<Void> _handle;
  final _DestroyDart _destroy;
  final _SetReferenceDart _setReference;
  final _AllocateDart _allocate;
  final _FreeDart _free;
  final _ProcessDart _process;
  final _FrequencyDart _frequency;
  final _FrequencyDart _targetFrequency;
  final _FrequencyDart _cents;
  final _FrequencyDart _confidence;
  final _IntegerResultDart _midiNote;
  final _IntegerResultDart _locked;
  bool _disposed = false;

  @override
  TrackingEngineReading? process(AudioPcmFrame frame) {
    if (_disposed || frame.samples.isEmpty) {
      return null;
    }

    final samples = _allocate(frame.sampleCount);
    if (samples == nullptr) {
      return null;
    }

    try {
      samples.asTypedList(frame.sampleCount).setAll(0, frame.samples);
      final succeeded = _process(
        _handle,
        samples,
        frame.sampleCount,
        frame.sampleRate,
      );
      if (succeeded != 1) {
        return null;
      }
      return TrackingEngineReading(
        frequencyHz: _frequency(_handle),
        targetFrequencyHz: _targetFrequency(_handle),
        cents: _cents(_handle),
        confidence: _confidence(_handle),
        midiNote: _midiNote(_handle),
        locked: _locked(_handle) == 1,
      );
    } finally {
      _free(samples);
    }
  }

  @override
  void setReferencePitch(double frequencyHz) {
    if (!_disposed && frequencyHz > 0) {
      _setReference(_handle, frequencyHz);
    }
  }

  @override
  void dispose() {
    if (_disposed) {
      return;
    }
    _destroy(_handle);
    _disposed = true;
  }
}
