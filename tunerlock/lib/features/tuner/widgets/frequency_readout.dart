import 'package:flutter/material.dart';

import '../../../core/utils/pitch_formatters.dart';
import '../models/tuner_reading.dart';

class FrequencyReadout extends StatelessWidget {
  const FrequencyReadout({required this.reading, super.key});

  final TunerReading reading;

  @override
  Widget build(BuildContext context) {
    final hasFrequency = reading.currentFrequency > 0;

    return Column(
      children: [
        Row(
          mainAxisAlignment: MainAxisAlignment.center,
          crossAxisAlignment: CrossAxisAlignment.baseline,
          textBaseline: TextBaseline.alphabetic,
          children: [
            Text(
              hasFrequency
                  ? PitchFormatters.frequency(reading.currentFrequency)
                  : '',
              style: const TextStyle(
                fontSize: 78,
                height: 0.95,
                fontWeight: FontWeight.w800,
                letterSpacing: 0,
                color: Color(0xFF6B675E),
                shadows: [
                  Shadow(
                    offset: Offset(1, 1),
                    blurRadius: 0,
                    color: Color(0xFF2C2A26),
                  ),
                ],
              ),
            ),
            const SizedBox(width: 8),
            Text(
              hasFrequency ? 'Hz' : '',
              style: const TextStyle(
                fontSize: 35,
                fontWeight: FontWeight.w600,
                color: Color(0xFF4B4841),
              ),
            ),
          ],
        ),
        const SizedBox(height: 10),
        Text(
          hasFrequency ? 'Auralock' : '',
          style: const TextStyle(
            fontSize: 34,
            height: 1,
            fontWeight: FontWeight.w400,
            color: Color(0xFF686259),
          ),
        ),
        const SizedBox(height: 12),
        Text(
          hasFrequency
              ? (reading.isLocked ? 'Locked' : 'Listening')
              : 'Listening',
          style: const TextStyle(
            fontSize: 33,
            height: 1,
            fontWeight: FontWeight.w400,
            color: Color(0xFF514D46),
          ),
        ),
      ],
    );
  }
}
