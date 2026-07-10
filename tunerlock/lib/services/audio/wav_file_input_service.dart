import 'dart:async';
import 'dart:io';
import 'dart:typed_data';

import 'audio_pcm_frame.dart';
import 'microphone_input_service.dart';

class WavFileInputService implements MicrophoneInputService {
  WavFileInputService({
    required this.filePath,
    this.frameSize = 2048,
    this.loop = true,
  });

  final String filePath;
  final int frameSize;
  final bool loop;

  final StreamController<AudioPcmFrame> _controller =
      StreamController<AudioPcmFrame>.broadcast();

  Timer? _timer;
  Float32List? _samples;
  int _sampleRate = 48000;
  int _cursor = 0;

  @override
  Stream<AudioPcmFrame> get frames => _controller.stream;

  @override
  Future<void> start() async {
    if (_timer != null) {
      return;
    }

    _load();
    final samples = _samples;
    if (samples == null || samples.isEmpty) {
      throw StateError('WAV file contains no samples: $filePath');
    }

    final frameDuration = Duration(
      microseconds:
          (frameSize / _sampleRate * Duration.microsecondsPerSecond).round(),
    );
    _timer = Timer.periodic(frameDuration, (_) => _emitFrame());
  }

  @override
  Future<void> stop() async {
    _timer?.cancel();
    _timer = null;
  }

  void _load() {
    if (_samples != null) {
      return;
    }

    final bytes = File(filePath).readAsBytesSync();
    if (bytes.length < 44 ||
        _ascii(bytes, 0, 4) != 'RIFF' ||
        _ascii(bytes, 8, 4) != 'WAVE') {
      throw FormatException('Unsupported WAV header: $filePath');
    }

    final data = ByteData.sublistView(bytes);
    var offset = 12;
    int? audioFormat;
    int? channelCount;
    int? bitsPerSample;
    int? dataOffset;
    int? dataLength;

    while (offset + 8 <= bytes.length) {
      final chunkId = _ascii(bytes, offset, 4);
      final chunkLength = data.getUint32(offset + 4, Endian.little);
      final chunkData = offset + 8;
      if (chunkData + chunkLength > bytes.length) {
        break;
      }

      if (chunkId == 'fmt ' && chunkLength >= 16) {
        audioFormat = data.getUint16(chunkData, Endian.little);
        channelCount = data.getUint16(chunkData + 2, Endian.little);
        _sampleRate = data.getUint32(chunkData + 4, Endian.little);
        bitsPerSample = data.getUint16(chunkData + 14, Endian.little);
      } else if (chunkId == 'data') {
        dataOffset = chunkData;
        dataLength = chunkLength;
      }

      offset = chunkData + chunkLength + (chunkLength.isOdd ? 1 : 0);
    }

    if (audioFormat == null ||
        channelCount == null ||
        bitsPerSample == null ||
        dataOffset == null ||
        dataLength == null ||
        channelCount <= 0 ||
        _sampleRate <= 0) {
      throw FormatException('Incomplete WAV chunks: $filePath');
    }

    final bytesPerSample = bitsPerSample ~/ 8;
    if (bytesPerSample <= 0 ||
        !((audioFormat == 1 &&
                (bitsPerSample == 16 ||
                    bitsPerSample == 24 ||
                    bitsPerSample == 32)) ||
            (audioFormat == 3 && bitsPerSample == 32))) {
      throw FormatException(
        'Only PCM16/24/32 and float32 WAV files are supported.',
      );
    }

    final sampleStride = bytesPerSample * channelCount;
    final frameCount = dataLength ~/ sampleStride;
    final mono = Float32List(frameCount);
    for (var frame = 0; frame < frameCount; frame++) {
      var sum = 0.0;
      final frameOffset = dataOffset + frame * sampleStride;
      for (var channel = 0; channel < channelCount; channel++) {
        final sampleOffset = frameOffset + channel * bytesPerSample;
        sum += _decodeSample(
          data,
          sampleOffset,
          audioFormat,
          bitsPerSample,
        );
      }
      mono[frame] = sum / channelCount;
    }
    _samples = mono;
  }

  void _emitFrame() {
    final samples = _samples;
    if (samples == null || samples.isEmpty || _controller.isClosed) {
      return;
    }

    if (_cursor >= samples.length) {
      if (!loop) {
        unawaited(stop());
        return;
      }
      _cursor = 0;
    }

    final frame = Float32List(frameSize);
    final available =
        (samples.length - _cursor).clamp(0, frameSize);
    frame.setRange(0, available, samples, _cursor);
    _cursor += available;

    _controller.add(
      AudioPcmFrame(
        samples: frame,
        sampleRate: _sampleRate,
        capturedAt: DateTime.now(),
      ),
    );
  }

  double _decodeSample(
    ByteData data,
    int offset,
    int audioFormat,
    int bitsPerSample,
  ) {
    if (audioFormat == 3) {
      return data.getFloat32(offset, Endian.little);
    }
    if (bitsPerSample == 16) {
      return data.getInt16(offset, Endian.little) / 32768.0;
    }
    if (bitsPerSample == 24) {
      var value = data.getUint8(offset) |
          (data.getUint8(offset + 1) << 8) |
          (data.getUint8(offset + 2) << 16);
      if ((value & 0x800000) != 0) {
        value |= ~0xffffff;
      }
      return value / 8388608.0;
    }
    return data.getInt32(offset, Endian.little) / 2147483648.0;
  }

  String _ascii(Uint8List bytes, int offset, int length) {
    return String.fromCharCodes(bytes.sublist(offset, offset + length));
  }
}
