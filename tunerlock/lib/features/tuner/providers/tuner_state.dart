import '../../../core/constants/tuner_constants.dart';
import '../models/tuner_reading.dart';

class TunerState {
  const TunerState._();

  static const TunerReading previewReading = TunerReading(
    note: '--',
    referenceFrequency: TunerConstants.referenceFrequency,
    currentFrequency: 0,
    isLocked: false,
    confidence: 0,
  );
}
