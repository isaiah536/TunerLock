import 'package:flutter/material.dart';

class PaperBackground extends StatelessWidget {
  const PaperBackground({required this.pitchMarkerPosition, super.key});

  final double pitchMarkerPosition;

  @override
  Widget build(BuildContext context) {
    return CustomPaint(
      painter: PaperPainter(pitchMarkerPosition: pitchMarkerPosition),
    );
  }
}

class PaperPainter extends CustomPainter {
  const PaperPainter({required this.pitchMarkerPosition});

  final double pitchMarkerPosition;

  @override
  void paint(Canvas canvas, Size size) {
    final base = Paint()..color = const Color(0xFFF8F0DB);
    canvas.drawRect(Offset.zero & size, base);

    final warmWash = Paint()
      ..shader = const RadialGradient(
        center: Alignment(0.25, -0.2),
        radius: 1.1,
        colors: [
          Color(0xE6FFFFE9),
          Color(0x40EDE2C9),
        ],
      ).createShader(Offset.zero & size);
    canvas.drawRect(Offset.zero & size, warmWash);

    final staffPaint = Paint()
      ..color = const Color(0x705B554B)
      ..strokeWidth = 2.6;
    final staffCenterY = size.height * 0.48;
    const staffSpacing = 40.5;
    for (var i = -2; i <= 2; i++) {
      final y = staffCenterY + i * staffSpacing;
      canvas.drawLine(Offset(0, y), Offset(size.width, y), staffPaint);
    }

    final staffTop = staffCenterY - staffSpacing * 2;
    final staffBottom = staffCenterY + staffSpacing * 2;
    final markerTop = staffCenterY - staffSpacing * 2.75;
    final markerBottom = staffCenterY + staffSpacing * 2.75;
    final startLineX = size.width * 0.02;
    final startLinePaint = Paint()
      ..color = const Color(0x805B554B)
      ..strokeWidth = 9;
    canvas.drawLine(
      Offset(startLineX, staffTop),
      Offset(startLineX, staffBottom),
      startLinePaint,
    );

    final markerX = size.width * pitchMarkerPosition;
    final markerPaint = Paint()
      ..color = const Color(0x99544F46)
      ..strokeWidth = 1.6;
    canvas.drawLine(
      Offset(markerX, markerTop),
      Offset(markerX, markerBottom),
      markerPaint,
    );

    final dotCenter = Offset(markerX, staffCenterY);
    canvas.drawCircle(dotCenter, 11, Paint()..color = const Color(0xFF626058));
    canvas.drawCircle(
      dotCenter,
      11,
      Paint()
        ..color = const Color(0xFF2E2C28)
        ..style = PaintingStyle.stroke
        ..strokeWidth = 1.2,
    );
  }

  @override
  bool shouldRepaint(covariant PaperPainter oldDelegate) {
    return oldDelegate.pitchMarkerPosition != pitchMarkerPosition;
  }
}
