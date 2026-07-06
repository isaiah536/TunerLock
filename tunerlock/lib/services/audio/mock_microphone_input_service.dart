import 'dart:async';
import 'dart:math' as math;
import 'dart:typed_data';

import 'audio_pcm_frame.dart';
import 'microphone_input_service.dart';

class MockMicrophoneInputService implements MicrophoneInputService {
  MockMicrophoneInputService({
    this.sampleRate = 48000,
    this.frameSize = 2048,
    this.frequencyHz = 440,
    this.amplitude = 0.72,
  });

  final int sampleRate;
  final int frameSize;
  final double frequencyHz;
  final double amplitude;

  final StreamController<AudioPcmFrame> _controller =
      StreamController<AudioPcmFrame>.broadcast();

  Timer? _timer;
  int _sampleCursor = 0;

  @override
  Stream<AudioPcmFrame> get frames => _controller.stream;

  @override
  Future<void> start() async {
    if (_timer != null) {
      return;
    }

    final frameDuration = Duration(
      microseconds: (frameSize / sampleRate * Duration.microsecondsPerSecond)
          .round(),
    );

    _timer = Timer.periodic(frameDuration, (_) {
      if (_controller.isClosed) {
        return;
      }

      _controller.add(
        AudioPcmFrame(
          samples: _generateSineFrame(),
          sampleRate: sampleRate,
          capturedAt: DateTime.now(),
        ),
      );
    });
  }

  @override
  Future<void> stop() async {
    _timer?.cancel();
    _timer = null;
  }

  Float32List _generateSineFrame() {
    final samples = Float32List(frameSize);
    for (var i = 0; i < frameSize; i++) {
      final phase =
          2 * math.pi * frequencyHz * (_sampleCursor + i) / sampleRate;
      samples[i] = (math.sin(phase) * amplitude).toDouble();
    }
    _sampleCursor += frameSize;
    return samples;
  }

  Future<void> dispose() async {
    await stop();
    await _controller.close();
  }
}
