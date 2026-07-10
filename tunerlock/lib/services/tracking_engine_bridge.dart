import 'audio/audio_pcm_frame.dart';
import 'tracking_engine_bridge_stub.dart'
    if (dart.library.ffi) 'tracking_engine_bridge_native.dart' as implementation;

class TrackingEngineReading {
  const TrackingEngineReading({
    required this.frequencyHz,
    required this.targetFrequencyHz,
    required this.cents,
    required this.confidence,
    required this.midiNote,
    required this.locked,
  });

  final double frequencyHz;
  final double targetFrequencyHz;
  final double cents;
  final double confidence;
  final int midiNote;
  final bool locked;
}

abstract interface class TrackingEngineBridge {
  TrackingEngineReading? process(AudioPcmFrame frame);
  void setReferencePitch(double frequencyHz);
  void dispose();
}

TrackingEngineBridge? createTrackingEngineBridge() {
  return implementation.createTrackingEngineBridge();
}
