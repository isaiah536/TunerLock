import 'dart:typed_data';

import 'package:flutter/services.dart';

import 'audio_pcm_frame.dart';
import 'microphone_input_service.dart';

class NativeMicrophoneInputService implements MicrophoneInputService {
  NativeMicrophoneInputService({
    MethodChannel? controlChannel,
    EventChannel? frameChannel,
  })  : _controlChannel =
            controlChannel ?? const MethodChannel('tunerlock/microphone'),
        _frameChannel =
            frameChannel ?? const EventChannel('tunerlock/microphone_frames');

  final MethodChannel _controlChannel;
  final EventChannel _frameChannel;

  Stream<AudioPcmFrame>? _frames;

  @override
  Stream<AudioPcmFrame> get frames {
    return _frames ??= _frameChannel.receiveBroadcastStream().map(_decodeFrame);
  }

  @override
  Future<void> start() async {
    await _controlChannel.invokeMethod<void>('start');
  }

  @override
  Future<void> stop() async {
    await _controlChannel.invokeMethod<void>('stop');
  }

  AudioPcmFrame _decodeFrame(Object? payload) {
    if (payload is Map) {
      return AudioPcmFrame(
        samples: _toFloat32List(payload['samples']),
        sampleRate: (payload['sampleRate'] as num?)?.toInt() ?? 48000,
        capturedAt: DateTime.now(),
      );
    }

    return AudioPcmFrame(
      samples: _toFloat32List(payload),
      sampleRate: 48000,
      capturedAt: DateTime.now(),
    );
  }

  Float32List _toFloat32List(Object? value) {
    if (value is Float32List) {
      return value;
    }

    if (value is ByteData) {
      return value.buffer.asFloat32List(
        value.offsetInBytes,
        value.lengthInBytes ~/ Float32List.bytesPerElement,
      );
    }

    if (value is List) {
      return Float32List.fromList(
        value.map((sample) => (sample as num).toDouble()).toList(),
      );
    }

    return Float32List(0);
  }
}
