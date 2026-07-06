import 'dart:math' as math;
import 'dart:typed_data';

class AudioPcmFrame {
  const AudioPcmFrame({
    required this.samples,
    required this.sampleRate,
    required this.capturedAt,
  });

  final Float32List samples;
  final int sampleRate;
  final DateTime capturedAt;

  int get sampleCount => samples.length;

  double get durationSeconds {
    if (sampleRate <= 0) {
      return 0;
    }
    return sampleCount / sampleRate;
  }

  double get rms {
    if (samples.isEmpty) {
      return 0;
    }

    var sum = 0.0;
    for (final sample in samples) {
      sum += sample * sample;
    }
    return math.sqrt(sum / samples.length);
  }
}
