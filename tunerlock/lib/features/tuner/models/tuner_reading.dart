class TunerReading {
  const TunerReading({
    required this.note,
    required this.referenceFrequency,
    required this.currentFrequency,
    required this.isLocked,
    required this.confidence,
    this.cents = 0,
  });

  final String note;
  final double referenceFrequency;
  final double currentFrequency;
  final bool isLocked;
  final double confidence;
  final double cents;

  TunerReading copyWith({
    String? note,
    double? referenceFrequency,
    double? currentFrequency,
    bool? isLocked,
    double? confidence,
    double? cents,
  }) {
    return TunerReading(
      note: note ?? this.note,
      referenceFrequency: referenceFrequency ?? this.referenceFrequency,
      currentFrequency: currentFrequency ?? this.currentFrequency,
      isLocked: isLocked ?? this.isLocked,
      confidence: confidence ?? this.confidence,
      cents: cents ?? this.cents,
    );
  }
}
