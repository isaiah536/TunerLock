import 'package:flutter_test/flutter_test.dart';
import 'package:tunerlock/services/audio/mock_microphone_input_service.dart';

void main() {
  test('mock microphone input emits normalized PCM frames', () async {
    final microphone = MockMicrophoneInputService(
      sampleRate: 48000,
      frameSize: 256,
      frequencyHz: 440,
      amplitude: 0.5,
    );

    addTearDown(microphone.dispose);

    await microphone.start();
    final frame = await microphone.frames.first;

    expect(frame.sampleRate, 48000);
    expect(frame.sampleCount, 256);
    expect(frame.rms, greaterThan(0));
    expect(frame.samples.every((sample) => sample >= -1 && sample <= 1), isTrue);
  });
}
