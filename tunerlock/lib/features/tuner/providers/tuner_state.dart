import '../../../core/constants/tuner_constants.dart';
import '../models/tuner_reading.dart';

class TunerState {
  const TunerState._();

  static const TunerReading previewReading = TunerReading(
    note: TunerConstants.referenceNote,
    referenceFrequency: TunerConstants.referenceFrequency,
    currentFrequency: TunerConstants.displayedFrequency,
    isLocked: true,
    confidence: 0.96,
  );
}
