import 'dart:math' as math;

import 'package:flutter/material.dart';

import '../../../services/tracking_engine_bridge.dart';
import '../models/tuner_reading.dart';

class ScoreTunerView extends StatelessWidget {
  const ScoreTunerView({
    required this.reading,
    required this.profile,
    required this.mode,
    required this.pitchTrailCents,
    required this.onModeChanged,
    super.key,
  });

  final TunerReading reading;
  final InstrumentProfile profile;
  final TrackingMode mode;
  final List<double> pitchTrailCents;
  final ValueChanged<TrackingMode> onModeChanged;

  @override
  Widget build(BuildContext context) {
    final accent = profileAccent(profile);
    final hasFrequency = reading.currentFrequency > 0;
    final frequencyText = hasFrequency
        ? reading.currentFrequency.toStringAsFixed(1)
        : '--';
    final centsText = hasFrequency
        ? '${reading.cents >= 0 ? '+' : ''}${reading.cents.round()} CENTS'
        : '-- CENTS';
    final lockText = hasFrequency
        ? (reading.isLocked ? 'LOCKED' : 'TRACKING')
        : 'IDLE';
    final confidenceText = hasFrequency
        ? 'CONFIDENCE ${(reading.confidence * 100).clamp(0, 100).round()}%'
        : 'CONFIDENCE --';

    return Column(
      mainAxisSize: MainAxisSize.min,
      children: [
        RichText(
          textAlign: TextAlign.center,
          text: TextSpan(
            children: [
              TextSpan(
                text: hasFrequency ? reading.note : '--',
                style: const TextStyle(
                  color: Color(0xFF151719),
                  fontSize: 58,
                  fontWeight: FontWeight.w500,
                  height: 1,
                  fontFamily: 'Georgia',
                ),
              ),
              TextSpan(
                text: hasFrequency
                    ? ' / ${reading.referenceFrequency.toStringAsFixed(1)} Hz'
                    : ' / -- Hz',
                style: const TextStyle(
                  color: Color(0xFF2E3438),
                  fontSize: 20,
                  fontWeight: FontWeight.w500,
                  fontStyle: FontStyle.italic,
                  fontFamily: 'Georgia',
                ),
              ),
            ],
          ),
        ),
        const SizedBox(height: 28),
        _FrequencyDisplay(value: frequencyText),
        const SizedBox(height: 40),
        _StaffCard(
          reading: reading,
          profile: profile,
          pitchTrailCents: pitchTrailCents,
        ),
        const SizedBox(height: 22),
        _PlainStatusRow(
          lockText: lockText,
          confidenceText: confidenceText,
          accent: accent,
          locked: hasFrequency && reading.isLocked,
        ),
        const SizedBox(height: 28),
        _CentsScale(reading: reading, accent: accent),
        const SizedBox(height: 14),
        Text(
          centsText,
          style: TextStyle(
            color: hasFrequency ? accent : const Color(0xFF8C867A),
            fontSize: 24,
            fontWeight: FontWeight.w500,
            fontFamily: 'Georgia',
          ),
        ),
        const SizedBox(height: 32),
        _ModeSegmentedControl(
          mode: mode,
          accent: accent,
          onChanged: onModeChanged,
        ),
      ],
    );
  }
}

class _FrequencyDisplay extends StatelessWidget {
  const _FrequencyDisplay({required this.value});

  final String value;

  @override
  Widget build(BuildContext context) {
    return Row(
      mainAxisAlignment: MainAxisAlignment.center,
      crossAxisAlignment: CrossAxisAlignment.end,
      children: [
        Flexible(
          child: Text(
            value,
            maxLines: 1,
            overflow: TextOverflow.ellipsis,
            style: const TextStyle(
              color: Color(0xFF121416),
              fontSize: 86,
              fontWeight: FontWeight.w400,
              height: 0.9,
              fontFamily: 'Georgia',
              fontFeatures: [FontFeature.tabularFigures()],
            ),
          ),
        ),
        const SizedBox(width: 8),
        const Padding(
          padding: EdgeInsets.only(bottom: 10),
          child: Text(
            'Hz',
            style: TextStyle(
              color: Color(0xFF23272B),
              fontSize: 23,
              fontWeight: FontWeight.w500,
              fontStyle: FontStyle.italic,
              fontFamily: 'Georgia',
            ),
          ),
        ),
      ],
    );
  }
}

class _StaffCard extends StatelessWidget {
  const _StaffCard({
    required this.reading,
    required this.profile,
    required this.pitchTrailCents,
  });

  final TunerReading reading;
  final InstrumentProfile profile;
  final List<double> pitchTrailCents;

  @override
  Widget build(BuildContext context) {
    return AspectRatio(
      aspectRatio: 2.22,
      child: CustomPaint(
        painter: _StaffPainter(
          reading: reading,
          profile: profile,
          pitchTrailCents: pitchTrailCents,
        ),
      ),
    );
  }
}

class _StaffPainter extends CustomPainter {
  const _StaffPainter({
    required this.reading,
    required this.profile,
    required this.pitchTrailCents,
  });

  final TunerReading reading;
  final InstrumentProfile profile;
  final List<double> pitchTrailCents;

  @override
  void paint(Canvas canvas, Size size) {
    final accent = profileAccent(profile);
    final hasFrequency = reading.currentFrequency > 0;
    final cardRect = Offset.zero & size;
    final cardRRect = RRect.fromRectAndRadius(
      cardRect,
      const Radius.circular(18),
    );

    final cardPaint = Paint()
      ..color = const Color(0xFFFBF5E9).withValues(alpha: 0.34)
      ..style = PaintingStyle.fill;
    final borderPaint = Paint()
      ..color = const Color(0xFFBEB6A8).withValues(alpha: 0.36)
      ..style = PaintingStyle.stroke
      ..strokeWidth = 1.2;

    canvas.drawRRect(cardRRect, cardPaint);
    canvas.drawRRect(cardRRect, borderPaint);

    final left = size.width * 0.02;
    final right = size.width * 0.98;
    final centerY = size.height * 0.52;
    final gap = size.height * 0.115;
    final staffPaint = Paint()
      ..color = profile == InstrumentProfile.wind
          ? const Color(0xFF2A5D79).withValues(alpha: 0.46)
          : profile == InstrumentProfile.brass
          ? const Color(0xFF8C682A).withValues(alpha: 0.42)
          : const Color(0xFF3D3A35).withValues(alpha: 0.28)
      ..strokeWidth = 1.05;

    for (var i = -2; i <= 2; i++) {
      final y = centerY + i * gap;
      canvas.drawLine(Offset(left, y), Offset(right, y), staffPaint);
    }

    final cents = hasFrequency ? reading.cents.clamp(-50.0, 50.0) : 0.0;
    final closeness = hasFrequency
        ? (1 - (cents.abs() / 50)).clamp(0.0, 1.0)
        : 0.0;
    final confidence = reading.confidence.clamp(0.0, 1.0);
    final clearInk = closeness > 0.82 || reading.isLocked;
    final blurryInk = hasFrequency && !clearInk;
    final trailPaint = Paint()
      ..color = hasFrequency
          ? Color.lerp(
              const Color(0xFF111111),
              accent,
              closeness,
            )!.withValues(alpha: clearInk ? 0.96 : 0.72)
          : const Color(0xFF928B80).withValues(alpha: 0.24)
      ..style = PaintingStyle.stroke
      ..strokeWidth =
          (profile == InstrumentProfile.brass ? 3.5 : 2.4) +
          (1 - closeness) * 1.0
      ..strokeCap = StrokeCap.round
      ..strokeJoin = StrokeJoin.round;

    final path = Path();
    final trail = pitchTrailCents.isEmpty
        ? <double>[hasFrequency ? cents : 0.0]
        : pitchTrailCents;
    for (var i = 0; i < trail.length; i++) {
      final t = trail.length == 1 ? 1.0 : i / (trail.length - 1);
      final sampleCents = trail[i].clamp(-80.0, 80.0);
      final sampleCloseness = (1 - (sampleCents.abs() / 50)).clamp(0.0, 1.0);
      final tremble =
          math.sin((i * 1.7) + sampleCents * 0.06) *
          gap *
          (0.03 + (1 - sampleCloseness) * 0.16);
      final x = left + (right - left) * (0.02 + 0.86 * t);
      final y = centerY - sampleCents / 50.0 * gap * 1.35 + tremble;
      i == 0 ? path.moveTo(x, y) : path.lineTo(x, y);
    }

    if (blurryInk) {
      final blurPaint = Paint()
        ..color = trailPaint.color.withValues(alpha: 0.24)
        ..style = PaintingStyle.stroke
        ..strokeWidth = trailPaint.strokeWidth + 4.0
        ..strokeCap = StrokeCap.round
        ..strokeJoin = StrokeJoin.round
        ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 5);
      canvas.drawPath(path, blurPaint);

      final bleedPaint = Paint()
        ..color = trailPaint.color.withValues(alpha: 0.18)
        ..style = PaintingStyle.stroke
        ..strokeWidth = 1.1
        ..strokeCap = StrokeCap.round;
      for (var i = 0; i < trail.length; i += 6) {
        final t = trail.length == 1 ? 1.0 : i / (trail.length - 1);
        final sampleCents = trail[i].clamp(-80.0, 80.0);
        final x = left + (right - left) * (0.02 + 0.86 * t);
        final y = centerY - sampleCents / 50.0 * gap * 1.35;
        canvas.drawCircle(
          Offset(x, y + gap * 0.22),
          1.1 + (1 - confidence) * 2.2,
          bleedPaint,
        );
      }
    }
    canvas.drawPath(path, trailPaint);

    final noteCenter = Offset(
      left + (right - left) * 0.88,
      centerY - cents / 50.0 * gap * 1.35,
    );
    final shadowPaint = Paint()
      ..color = accent.withValues(
        alpha: hasFrequency ? 0.20 + closeness * 0.2 : 0.08,
      )
      ..maskFilter = MaskFilter.blur(BlurStyle.normal, clearInk ? 5 : 10);
    canvas.drawOval(
      Rect.fromCenter(
        center: noteCenter.translate(1.5, 2),
        width: 34,
        height: 27,
      ),
      shadowPaint,
    );
    final notePaint = Paint()
      ..color = hasFrequency ? accent : const Color(0xFF373330);
    canvas.save();
    canvas.translate(noteCenter.dx, noteCenter.dy);
    canvas.rotate(-0.16);
    canvas.drawOval(
      Rect.fromCenter(center: Offset.zero, width: 31, height: 25),
      notePaint,
    );
    canvas.restore();
  }

  @override
  bool shouldRepaint(covariant _StaffPainter oldDelegate) {
    return oldDelegate.reading != reading ||
        oldDelegate.profile != profile ||
        oldDelegate.pitchTrailCents != pitchTrailCents;
  }
}

class _PlainStatusRow extends StatelessWidget {
  const _PlainStatusRow({
    required this.lockText,
    required this.confidenceText,
    required this.accent,
    required this.locked,
  });

  final String lockText;
  final String confidenceText;
  final Color accent;
  final bool locked;

  @override
  Widget build(BuildContext context) {
    final activeColor = locked ? accent : const Color(0xFF151719);
    return Padding(
      padding: const EdgeInsets.symmetric(horizontal: 22),
      child: Row(
        children: [
          Expanded(
            child: Text(
              lockText,
              maxLines: 1,
              overflow: TextOverflow.ellipsis,
              style: TextStyle(
                color: activeColor,
                fontSize: 20,
                fontWeight: FontWeight.w600,
                fontFamily: 'Georgia',
              ),
            ),
          ),
          Text(
            confidenceText,
            maxLines: 1,
            overflow: TextOverflow.ellipsis,
            style: const TextStyle(
              color: Color(0xFF151719),
              fontSize: 20,
              fontWeight: FontWeight.w600,
              fontFamily: 'Georgia',
            ),
          ),
        ],
      ),
    );
  }
}

class _CentsScale extends StatelessWidget {
  const _CentsScale({required this.reading, required this.accent});

  final TunerReading reading;
  final Color accent;

  @override
  Widget build(BuildContext context) {
    return SizedBox(
      height: 72,
      child: CustomPaint(
        painter: _CentsScalePainter(reading: reading, accent: accent),
        child: const SizedBox.expand(),
      ),
    );
  }
}

class _CentsScalePainter extends CustomPainter {
  const _CentsScalePainter({required this.reading, required this.accent});

  final TunerReading reading;
  final Color accent;

  @override
  void paint(Canvas canvas, Size size) {
    final hasFrequency = reading.currentFrequency > 0;
    final left = size.width * 0.07;
    final right = size.width * 0.93;
    final baseline = size.height * 0.56;
    final scalePaint = Paint()
      ..color = const Color(0xFF716C63).withValues(alpha: 0.72)
      ..strokeWidth = 1.1
      ..strokeCap = StrokeCap.round;
    canvas.drawLine(
      Offset(left, baseline),
      Offset(right, baseline),
      scalePaint,
    );

    const labels = [-50, -25, 0, 25, 50];
    final textPainter = TextPainter(textDirection: TextDirection.ltr);
    for (final label in labels) {
      final t = (label + 50) / 100;
      final x = left + (right - left) * t;
      final tickHeight = label == 0 ? 17.0 : 11.0;
      canvas.drawLine(
        Offset(x, baseline - tickHeight),
        Offset(x, baseline + tickHeight),
        scalePaint,
      );
      textPainter.text = TextSpan(
        text: label > 0 ? '+$label' : '$label',
        style: const TextStyle(
          color: Color(0xFF312F2C),
          fontSize: 18,
          fontWeight: FontWeight.w500,
          fontFamily: 'Georgia',
        ),
      );
      textPainter.layout();
      textPainter.paint(
        canvas,
        Offset(x - textPainter.width / 2, baseline - 45),
      );
    }

    for (var i = -3; i <= 3; i++) {
      if (i == 0) {
        continue;
      }
      final x = left + (right - left) * ((i * 12.5 + 50) / 100);
      canvas.drawLine(
        Offset(x, baseline - 6),
        Offset(x, baseline + 6),
        scalePaint..color = const Color(0xFF716C63).withValues(alpha: 0.42),
      );
    }

    final cents = hasFrequency ? reading.cents.clamp(-50.0, 50.0) : 0.0;
    final markerX = left + (right - left) * ((cents + 50) / 100);
    final markerPaint = Paint()
      ..color = hasFrequency ? accent : const Color(0xFF151719)
      ..strokeWidth = 3
      ..strokeCap = StrokeCap.round;
    canvas.drawLine(
      Offset(markerX, baseline - 3),
      Offset(markerX, baseline + 34),
      markerPaint,
    );
    canvas.drawCircle(Offset(markerX, baseline - 1), 7, markerPaint);
  }

  @override
  bool shouldRepaint(covariant _CentsScalePainter oldDelegate) {
    return oldDelegate.reading != reading || oldDelegate.accent != accent;
  }
}

class _ModeSegmentedControl extends StatelessWidget {
  const _ModeSegmentedControl({
    required this.mode,
    required this.accent,
    required this.onChanged,
  });

  final TrackingMode mode;
  final Color accent;
  final ValueChanged<TrackingMode> onChanged;

  @override
  Widget build(BuildContext context) {
    return Container(
      height: 60,
      padding: const EdgeInsets.all(4),
      decoration: BoxDecoration(
        gradient: const LinearGradient(
          begin: Alignment.topLeft,
          end: Alignment.bottomRight,
          colors: [Color(0xFFE5E0D7), Color(0xFFC9C2B7)],
        ),
        border: Border.all(color: Colors.white.withValues(alpha: 0.55)),
        borderRadius: BorderRadius.circular(30),
        boxShadow: [
          BoxShadow(
            color: const Color(0xFF6C6258).withValues(alpha: 0.28),
            blurRadius: 18,
            offset: const Offset(0, 8),
          ),
          BoxShadow(
            color: Colors.white.withValues(alpha: 0.55),
            blurRadius: 8,
            offset: const Offset(-2, -2),
          ),
        ],
      ),
      child: Row(
        children: [
          Expanded(
            child: _ModeSegment(
              label: 'TUNING MODE',
              active: mode == TrackingMode.tuning,
              accent: accent,
              onTap: () => onChanged(TrackingMode.tuning),
            ),
          ),
          Expanded(
            child: _ModeSegment(
              label: 'PERFORMANCE MODE',
              active: mode == TrackingMode.performance,
              accent: accent,
              onTap: () => onChanged(TrackingMode.performance),
            ),
          ),
        ],
      ),
    );
  }
}

class _ModeSegment extends StatelessWidget {
  const _ModeSegment({
    required this.label,
    required this.active,
    required this.accent,
    required this.onTap,
  });

  final String label;
  final bool active;
  final Color accent;
  final VoidCallback onTap;

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      behavior: HitTestBehavior.opaque,
      onTap: onTap,
      child: AnimatedContainer(
        duration: const Duration(milliseconds: 180),
        height: 52,
        alignment: Alignment.center,
        decoration: BoxDecoration(
          gradient: active
              ? LinearGradient(
                  begin: Alignment.topLeft,
                  end: Alignment.bottomRight,
                  colors: [
                    Color.lerp(accent, Colors.white, 0.16)!,
                    accent,
                    Color.lerp(accent, Colors.black, 0.18)!,
                  ],
                )
              : null,
          borderRadius: BorderRadius.circular(26),
          border: active
              ? Border.all(color: Colors.white.withValues(alpha: 0.42))
              : null,
          boxShadow: active
              ? [
                  BoxShadow(
                    color: Colors.white.withValues(alpha: 0.32),
                    blurRadius: 5,
                    offset: const Offset(-1, -1),
                  ),
                  BoxShadow(
                    color: accent.withValues(alpha: 0.30),
                    blurRadius: 14,
                    offset: const Offset(0, 5),
                  ),
                ]
              : null,
        ),
        child: Text(
          label,
          maxLines: 1,
          overflow: TextOverflow.ellipsis,
          style: TextStyle(
            color: active ? Colors.white : const Color(0xFF2A2927),
            fontSize: 14,
            fontWeight: FontWeight.w800,
          ),
        ),
      ),
    );
  }
}

Color profileAccent(InstrumentProfile profile) {
  switch (profile) {
    case InstrumentProfile.strings:
      return const Color(0xFF006C72);
    case InstrumentProfile.wind:
      return const Color(0xFF075995);
    case InstrumentProfile.brass:
      return const Color(0xFFA57913);
  }
}
