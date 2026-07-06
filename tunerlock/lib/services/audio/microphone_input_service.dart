import 'audio_pcm_frame.dart';

abstract class MicrophoneInputService {
  Stream<AudioPcmFrame> get frames;

  Future<void> start();

  Future<void> stop();
}
