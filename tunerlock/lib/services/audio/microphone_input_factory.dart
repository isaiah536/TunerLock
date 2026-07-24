import 'dart:io';

import 'microphone_input_service.dart';
import 'mock_microphone_input_service.dart';
import 'native_microphone_input_service.dart';
import 'silent_microphone_input_service.dart';
import 'wav_file_input_service.dart';

class MicrophoneInputFactory {
  const MicrophoneInputFactory._();

  static MicrophoneInputService create({
    bool useMockInput = false,
    bool? useWavInput,
    String? wavPath,
  }) {
    if (useMockInput) {
      return MockMicrophoneInputService();
    }

    final shouldUseWavInput = useWavInput ?? false;
    if (shouldUseWavInput) {
      final resolvedPath = _resolveWavPath(wavPath);
      if (resolvedPath != null) {
        return WavFileInputService(filePath: resolvedPath);
      }
      return const SilentMicrophoneInputService();
    }

    if (Platform.isIOS || Platform.isAndroid) {
      return NativeMicrophoneInputService();
    }

    final resolvedPath = _resolveWavPath(wavPath);
    if (resolvedPath != null) {
      return WavFileInputService(filePath: resolvedPath);
    }

    return const SilentMicrophoneInputService();
  }

  static String? _resolveWavPath(String? requestedPath) {
    const definedPath = String.fromEnvironment('TUNERLOCK_WAV_PATH');
    final explicitPath =
        requestedPath ?? (definedPath.isEmpty ? null : definedPath);
    if (explicitPath != null && File(explicitPath).existsSync()) {
      return File(explicitPath).absolute.path;
    }

    return null;
  }
}
