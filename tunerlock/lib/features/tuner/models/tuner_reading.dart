class TunerReading {
  const TunerReading({
    required this.note,
    required this.referenceFrequency,
    required this.currentFrequency,
    required this.isLocked,
    required this.confidence,
  });

  final String note;
  final double referenceFrequency;
  final double currentFrequency;
  final bool isLocked;
  final double confidence;
}
