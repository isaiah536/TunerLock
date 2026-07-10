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
  })
      : _microphoneInput =
            microphoneInput ?? MicrophoneInputFactory.create(),
        _bridge = bridge ?? createTrackingEngineBridge();

  final MicrophoneInputService _microphoneInput;
  final TrackingEngineBridge? _bridge;

  bool get isAvailable => _bridge != null;
  Stream<AudioPcmFrame> get audioFrames => _microphoneInput.frames;
  Stream<TunerReading> get readings => audioFrames.map((frame) {
        final engineReading = _bridge?.process(frame);
        if (engineReading == null || engineReading.frequencyHz <= 0) {
          return TunerState.previewReading;
        }
        return TunerState.previewReading.copyWith(
          note: _midiNoteName(engineReading.midiNote),
          referenceFrequency: engineReading.targetFrequencyHz,
          currentFrequency: engineReading.frequencyHz,
          isLocked: engineReading.locked,
          confidence: engineReading.confidence,
          cents: engineReading.cents,
        );
      });

  Future<void> startListening() {
    return _microphoneInput.start();
  }

  Future<void> stopListening() {
    return _microphoneInput.stop();
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
}
