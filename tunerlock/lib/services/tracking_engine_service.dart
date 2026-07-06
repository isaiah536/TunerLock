import '../features/tuner/models/tuner_reading.dart';
import '../features/tuner/providers/tuner_state.dart';
import 'audio/audio_pcm_frame.dart';
import 'audio/microphone_input_factory.dart';
import 'audio/microphone_input_service.dart';

class TrackingEngineService {
  TrackingEngineService({MicrophoneInputService? microphoneInput})
      : _microphoneInput =
            microphoneInput ?? MicrophoneInputFactory.create();

  final MicrophoneInputService _microphoneInput;

  Stream<AudioPcmFrame> get audioFrames => _microphoneInput.frames;

  Future<void> startListening() {
    return _microphoneInput.start();
  }

  Future<void> stopListening() {
    return _microphoneInput.stop();
  }

  Future<TunerReading> latestReading() async {
    return TunerState.previewReading;
  }
}
