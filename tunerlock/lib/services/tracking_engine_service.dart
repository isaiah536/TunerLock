import 'dart:async';
import 'dart:typed_data';

import '../features/tuner/models/tuner_reading.dart';
import '../features/tuner/providers/tuner_state.dart';
import 'audio/audio_pcm_frame.dart';
import 'audio/microphone_input_factory.dart';
import 'audio/microphone_input_service.dart';
import 'tracking_engine_bridge.dart';

class TrackingEngineService {
  TrackingEngineService({
    MicrophoneInputService? microphoneInput,
    TrackingEngineBridge? bridge,
  }) : _microphoneInput = microphoneInput ?? MicrophoneInputFactory.create(),
       _bridge = bridge ?? createTrackingEngineBridge() {
    _bridge?.setInstrumentProfile(_profile);
    _bridge?.setTrackingMode(TrackingMode.tuning);
  }

  static const _analysisFrameSize = 2048;

  final MicrophoneInputService _microphoneInput;
  final TrackingEngineBridge? _bridge;
  final _frameSlicer = _AnalysisFrameSlicer(frameSize: _analysisFrameSize);
  InstrumentProfile _profile = InstrumentProfile.strings;
  TrackingMode _mode = TrackingMode.tuning;

  bool get isAvailable => _bridge != null;
  Stream<AudioPcmFrame> get audioFrames => _microphoneInput.frames;
  Stream<TunerReading> get readings async* {
    await for (final inputFrame in audioFrames) {
      for (final analysisFrame in _frameSlicer.push(
        inputFrame,
        hopSize: _hopSizeFor(_profile, _mode),
      )) {
        final engineReading = _bridge?.process(analysisFrame);
        if (engineReading == null || engineReading.frequencyHz <= 0) {
          yield TunerState.previewReading;
          continue;
        }
        yield TunerState.previewReading.copyWith(
          note: _midiNoteName(engineReading.midiNote),
          referenceFrequency: engineReading.targetFrequencyHz,
          currentFrequency: engineReading.frequencyHz,
          isLocked: engineReading.locked,
          confidence: engineReading.confidence,
          cents: engineReading.cents,
        );
      }
    }
  }

  Future<void> startListening() {
    return _microphoneInput.start();
  }

  Future<void> stopListening() {
    return _microphoneInput.stop();
  }

  void setTrackingMode(TrackingMode mode) {
    if (_mode == mode) {
      return;
    }
    _mode = mode;
    _frameSlicer.reset();
    _bridge?.setTrackingMode(mode);
  }

  void setInstrumentProfile(InstrumentProfile profile) {
    if (_profile == profile) {
      return;
    }
    _profile = profile;
    _frameSlicer.reset();
    _bridge?.setInstrumentProfile(profile);
  }

  Future<TunerReading> latestReading() async {
    return TunerState.previewReading;
  }

  Future<void> dispose() async {
    await stopListening();
    _bridge?.dispose();
  }

  String _midiNoteName(int midiNote) {
    if (midiNote < 0) {
      return '--';
    }
    const names = [
      'C',
      'C#',
      'D',
      'D#',
      'E',
      'F',
      'F#',
      'G',
      'G#',
      'A',
      'A#',
      'B',
    ];
    final name = names[midiNote % 12];
    final octave = midiNote ~/ 12 - 1;
    return '$name$octave';
  }

  int _hopSizeFor(InstrumentProfile profile, TrackingMode mode) {
    if (mode != TrackingMode.performance) {
      return _analysisFrameSize;
    }
    switch (profile) {
      case InstrumentProfile.strings:
      case InstrumentProfile.wind:
        return _analysisFrameSize ~/ 2;
      case InstrumentProfile.brass:
        return _analysisFrameSize;
    }
  }
}

class _AnalysisFrameSlicer {
  _AnalysisFrameSlicer({required this.frameSize});

  final int frameSize;
  final List<double> _buffer = <double>[];

  Iterable<AudioPcmFrame> push(
    AudioPcmFrame frame, {
    required int hopSize,
  }) sync* {
    if (frame.samples.isEmpty || frame.sampleRate <= 0 || hopSize <= 0) {
      return;
    }

    _buffer.addAll(frame.samples);
    while (_buffer.length >= frameSize) {
      yield AudioPcmFrame(
        samples: Float32List.fromList(_buffer.take(frameSize).toList()),
        sampleRate: frame.sampleRate,
        capturedAt: frame.capturedAt,
      );
      _buffer.removeRange(0, hopSize.clamp(1, frameSize));
    }
  }

  void reset() {
    _buffer.clear();
  }
}
