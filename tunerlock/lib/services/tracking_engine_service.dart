import '../features/tuner/models/tuner_reading.dart';
import '../features/tuner/providers/tuner_state.dart';

class TrackingEngineService {
  const TrackingEngineService();

  Future<TunerReading> latestReading() async {
    return TunerState.previewReading;
  }
}
