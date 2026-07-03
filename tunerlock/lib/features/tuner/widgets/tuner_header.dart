import 'package:flutter/material.dart';

import '../models/tuner_reading.dart';

class TunerHeader extends StatelessWidget {
  const TunerHeader({required this.reading, super.key});

  final TunerReading reading;

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.fromLTRB(18, 10, 18, 12),
      child: Row(
        children: [
          IconButton(
            tooltip: 'Menu',
            onPressed: () {},
            icon: const Icon(Icons.menu_rounded, size: 34),
          ),
          const Spacer(),
          Column(
            children: [
              Row(
                mainAxisSize: MainAxisSize.min,
                children: [
                  Text(
                    reading.note,
                    style: const TextStyle(
                      fontSize: 32,
                      height: 1,
                      fontWeight: FontWeight.w500,
                      color: Color(0xFF151511),
                    ),
                  ),
                  const SizedBox(width: 4),
                  const Icon(Icons.keyboard_arrow_down_rounded, size: 26),
                ],
              ),
              const SizedBox(height: 6),
              Text(
                'Standard ${reading.referenceFrequency.toStringAsFixed(1)} Hz',
                style: const TextStyle(
                  fontSize: 16,
                  fontWeight: FontWeight.w500,
                  color: Color(0xFF24221D),
                ),
              ),
            ],
          ),
          const Spacer(),
          IconButton(
            tooltip: 'Settings',
            onPressed: () {},
            icon: const Icon(Icons.settings_outlined, size: 30),
          ),
        ],
      ),
    );
  }
}
