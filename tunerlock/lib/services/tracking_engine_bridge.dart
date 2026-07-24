import 'audio/audio_pcm_frame.dart';
import 'tracking_engine_bridge_stub.dart'
    if (dart.library.ffi) 'tracking_engine_bridge_native.dart'
    as implementation;

enum TrackingMode { tuning, performance }

enum InstrumentProfile { strings, wind, brass }

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
  void setInstrumentProfile(InstrumentProfile profile);
  void setTrackingMode(TrackingMode mode);
  void dispose();
}

TrackingEngineBridge? createTrackingEngineBridge() {
  return implementation.createTrackingEngineBridge();
}
