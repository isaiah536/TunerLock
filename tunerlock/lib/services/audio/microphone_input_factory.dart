import 'microphone_input_service.dart';
import 'mock_microphone_input_service.dart';
import 'native_microphone_input_service.dart';

class MicrophoneInputFactory {
  const MicrophoneInputFactory._();

  static MicrophoneInputService create({bool useMockInput = true}) {
    if (useMockInput) {
      return MockMicrophoneInputService();
    }

    return NativeMicrophoneInputService();
  }
}
