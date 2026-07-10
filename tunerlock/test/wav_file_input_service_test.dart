import 'package:flutter_test/flutter_test.dart';
import 'package:tunerlock/services/audio/wav_file_input_service.dart';

void main() {
  test('streams violin WAV as 2048-sample PCM frames', () async {
    final service = WavFileInputService(
      filePath: '../datasets/violin/violin_e.wav',
      loop: false,
    );

    final firstFrame = service.frames.first;
    await service.start();
    final frame = await firstFrame.timeout(const Duration(seconds: 2));
    await service.stop();

    expect(frame.sampleRate, 48000);
    expect(frame.sampleCount, 2048);
    expect(frame.samples, isNotEmpty);
  });
}
