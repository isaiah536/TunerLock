import 'audio_pcm_frame.dart';
import 'microphone_input_service.dart';

class SilentMicrophoneInputService implements MicrophoneInputService {
  const SilentMicrophoneInputService();

  @override
  Stream<AudioPcmFrame> get frames => const Stream<AudioPcmFrame>.empty();

  @override
  Future<void> start() async {}

  @override
  Future<void> stop() async {}
}
