import 'dart:io';

import 'microphone_input_service.dart';
import 'mock_microphone_input_service.dart';
import 'native_microphone_input_service.dart';
import 'wav_file_input_service.dart';

class MicrophoneInputFactory {
  const MicrophoneInputFactory._();

  static MicrophoneInputService create({
    bool useMockInput = false,
    bool useWavInput = true,
    String? wavPath,
  }) {
    if (useMockInput) {
      return MockMicrophoneInputService();
    }

    if (useWavInput) {
      final resolvedPath = _resolveWavPath(wavPath);
      if (resolvedPath != null) {
        return WavFileInputService(filePath: resolvedPath);
      }
      return MockMicrophoneInputService();
    }

    return NativeMicrophoneInputService();
  }

  static String? _resolveWavPath(String? requestedPath) {
    const definedPath = String.fromEnvironment('TUNERLOCK_WAV_PATH');
    final explicitPath =
        requestedPath ?? (definedPath.isEmpty ? null : definedPath);
    if (explicitPath != null && File(explicitPath).existsSync()) {
      return File(explicitPath).absolute.path;
    }

    var directory = Directory.current.absolute;
    for (var depth = 0; depth < 8; depth++) {
      final candidate = File(
        '${directory.path}${Platform.pathSeparator}'
        'datasets${Platform.pathSeparator}violin${Platform.pathSeparator}'
        'violin_e.wav',
      );
      if (candidate.existsSync()) {
        return candidate.path;
      }
      final parent = directory.parent;
      if (parent.path == directory.path) {
        break;
      }
      directory = parent;
    }
    return null;
  }
}
