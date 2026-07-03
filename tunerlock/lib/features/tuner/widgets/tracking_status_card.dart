import 'package:flutter/material.dart';

import '../../../shared/widgets/glass_panel.dart';
import '../models/tuner_reading.dart';

class TrackingStatusCard extends StatelessWidget {
  const TrackingStatusCard({required this.reading, super.key});

  final TunerReading reading;

  @override
  Widget build(BuildContext context) {
    final status = reading.isLocked ? 'LOCKED' : 'SEARCHING';

    return Padding(
      padding: const EdgeInsets.symmetric(horizontal: 20),
      child: GlassPanel(
        borderRadius: 12,
        padding: const EdgeInsets.fromLTRB(14, 12, 14, 10),
        child: Row(
          children: [
            const Icon(Icons.lock_outline_rounded, size: 32),
            const SizedBox(width: 10),
            Expanded(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  const Text(
                    'TRACKING',
                    style: TextStyle(fontSize: 12, fontWeight: FontWeight.w700),
                  ),
                  Text(
                    status,
                    style: const TextStyle(
                      fontSize: 24,
                      height: 1.05,
                      fontWeight: FontWeight.w700,
                    ),
                  ),
                ],
              ),
            ),
            Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                const Text(
                  'CONFIDENCE',
                  style: TextStyle(fontSize: 12, fontWeight: FontWeight.w700),
                ),
                Text(
                  '${(reading.confidence * 100).round()}%',
                  style: const TextStyle(
                    fontSize: 24,
                    height: 1.05,
                    fontWeight: FontWeight.w700,
                  ),
                ),
              ],
            ),
            const SizedBox(width: 14),
            SizedBox(
              width: 48,
              height: 28,
              child: CustomPaint(painter: ConfidencePainter()),
            ),
          ],
        ),
      ),
    );
  }
}

class ConfidencePainter extends CustomPainter {
  @override
  void paint(Canvas canvas, Size size) {
    final outline = Paint()
      ..color = const Color(0xFF777269)
      ..style = PaintingStyle.stroke
      ..strokeWidth = 1.4;
    final fill = Paint()
      ..color = const Color(0x33777269)
      ..style = PaintingStyle.fill;

    final path = Path()
      ..moveTo(2, size.height - 4)
      ..lineTo(size.width - 2, 4)
      ..lineTo(size.width - 2, size.height - 4)
      ..close();
    canvas.drawPath(path, fill);
    canvas.drawPath(path, outline);

    final hatch = Paint()
      ..color = const Color(0x55777269)
      ..strokeWidth = 1;
    for (var x = 11.0; x < size.width - 3; x += 7) {
      canvas.drawLine(
        Offset(x, size.height - 5),
        Offset(x + 8, size.height - 9),
        hatch,
      );
    }
  }

  @override
  bool shouldRepaint(covariant CustomPainter oldDelegate) => false;
}
