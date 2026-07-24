import 'dart:math' as math;

import 'package:flutter/material.dart';

import '../../../services/tracking_engine_bridge.dart';
import '../models/tuner_reading.dart';

class CelestialTunerView extends StatefulWidget {
  const CelestialTunerView({
    required this.reading,
    required this.profile,
    super.key,
  });

  final TunerReading reading;
  final InstrumentProfile profile;

  @override
  State<CelestialTunerView> createState() => _CelestialTunerViewState();
}

class _CelestialTunerViewState extends State<CelestialTunerView>
    with SingleTickerProviderStateMixin {
  late final AnimationController _controller;

  @override
  void initState() {
    super.initState();
    _controller = AnimationController(
      vsync: this,
      duration: const Duration(seconds: 8),
    )..repeat();
  }

  @override
  void dispose() {
    _controller.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final hasFrequency = widget.reading.currentFrequency > 0;
    final targetText = hasFrequency
        ? '${widget.reading.note} ${widget.reading.referenceFrequency.toStringAsFixed(1)} Hz'
        : '--';
    final frequencyText = hasFrequency
        ? widget.reading.currentFrequency.toStringAsFixed(1)
        : '--';

    return Column(
      mainAxisSize: MainAxisSize.min,
      children: [
        Text(
          targetText,
          style: TextStyle(
            color: _profileAccent(widget.profile),
            fontSize: 18,
            fontWeight: FontWeight.w700,
          ),
        ),
        const SizedBox(height: 12),
        SizedBox(
          width: 330,
          height: 330,
          child: AnimatedBuilder(
            animation: _controller,
            builder: (context, _) {
              return Stack(
                alignment: Alignment.center,
                children: [
                  Positioned.fill(
                    child: CustomPaint(
                      painter: _CelestialPainter(
                        profile: widget.profile,
                        reading: widget.reading,
                        time: _controller.value,
                      ),
                    ),
                  ),
                  _FrequencyCenter(
                    frequencyText: frequencyText,
                    hasFrequency: hasFrequency,
                    profile: widget.profile,
                  ),
                ],
              );
            },
          ),
        ),
      ],
    );
  }
}

class _FrequencyCenter extends StatelessWidget {
  const _FrequencyCenter({
    required this.frequencyText,
    required this.hasFrequency,
    required this.profile,
  });

  final String frequencyText;
  final bool hasFrequency;
  final InstrumentProfile profile;

  @override
  Widget build(BuildContext context) {
    final color = profile == InstrumentProfile.brass
        ? const Color(0xFF120B05)
        : Colors.white;
    final mutedColor = color.withValues(alpha: 0.45);

    return Column(
      mainAxisSize: MainAxisSize.min,
      children: [
        Text(
          frequencyText,
          style: TextStyle(
            color: hasFrequency ? color : mutedColor,
            fontSize: 54,
            fontWeight: FontWeight.w700,
            height: 0.95,
            fontFeatures: const [FontFeature.tabularFigures()],
          ),
        ),
        const SizedBox(height: 7),
        Text(
          'Hz',
          style: TextStyle(
            color: color.withValues(alpha: hasFrequency ? 0.72 : 0.36),
            fontSize: 17,
            fontWeight: FontWeight.w700,
          ),
        ),
      ],
    );
  }
}

class _CelestialPainter extends CustomPainter {
  const _CelestialPainter({
    required this.profile,
    required this.reading,
    required this.time,
  });

  final InstrumentProfile profile;
  final TunerReading reading;
  final double time;

  @override
  void paint(Canvas canvas, Size size) {
    final center = size.center(Offset.zero);
    final shortest = math.min(size.width, size.height);
    final hasFrequency = reading.currentFrequency > 0;
    final closeness = hasFrequency
        ? (1 - (reading.cents.abs() / 50)).clamp(0.0, 1.0)
        : 0.0;
    final confidence = reading.confidence.clamp(0.0, 1.0);
    final energy = (closeness * 0.72 + confidence * 0.28).clamp(0.0, 1.0);
    final accent = _statusColor(profile, hasFrequency, reading.cents, energy);

    _paintStarField(canvas, size, accent, energy);

    switch (profile) {
      case InstrumentProfile.strings:
        _paintBlackHole(canvas, center, shortest, accent, energy, closeness);
      case InstrumentProfile.wind:
        _paintGasPlanet(canvas, center, shortest, accent, energy, closeness);
      case InstrumentProfile.brass:
        _paintSun(canvas, center, shortest, accent, energy, closeness);
    }
  }

  void _paintStarField(Canvas canvas, Size size, Color accent, double energy) {
    final paint = Paint()..style = PaintingStyle.fill;
    for (var i = 0; i < 36; i++) {
      final x =
          (math.sin(i * 12.989 + time * math.pi * 2) * 0.5 + 0.5) * size.width;
      final y =
          (math.cos(i * 7.233 + time * math.pi) * 0.5 + 0.5) * size.height;
      final alpha = 0.08 + 0.16 * energy * ((i % 4) + 1) / 4;
      paint.color = accent.withValues(alpha: alpha);
      canvas.drawCircle(Offset(x, y), 0.8 + (i % 3) * 0.35, paint);
    }
  }

  void _paintBlackHole(
    Canvas canvas,
    Offset center,
    double shortest,
    Color accent,
    double energy,
    double closeness,
  ) {
    final outerRadius = shortest * (0.43 - closeness * 0.07);
    final coreRadius = shortest * 0.285;
    final glowPaint = Paint()
      ..shader =
          RadialGradient(
            colors: [
              accent.withValues(alpha: 0.42 * energy),
              const Color(0xFF5B35D6).withValues(alpha: 0.12 + 0.16 * energy),
              Colors.transparent,
            ],
          ).createShader(
            Rect.fromCircle(center: center, radius: outerRadius * 1.55),
          )
      ..maskFilter = MaskFilter.blur(BlurStyle.normal, 20 + 18 * energy);
    canvas.drawCircle(center, outerRadius * 1.18, glowPaint);

    final ringPaint = Paint()
      ..style = PaintingStyle.stroke
      ..strokeCap = StrokeCap.round;
    for (var i = 0; i < 4; i++) {
      final radius = outerRadius - i * shortest * 0.028;
      ringPaint
        ..color = Color.lerp(
          const Color(0xFF6B46FF),
          accent,
          energy,
        )!.withValues(alpha: 0.18 + energy * 0.32)
        ..strokeWidth = 3.2 + energy * 3.8 - i * 0.35
        ..maskFilter = MaskFilter.blur(BlurStyle.normal, 2 + energy * 4);
      final rect = Rect.fromCenter(
        center: center,
        width: radius * 2.16,
        height: radius * (0.42 + i * 0.035),
      );
      final start = time * math.pi * 2 + i * 0.8;
      canvas.drawArc(rect, start, math.pi * 1.48, false, ringPaint);
    }

    final particlePaint = Paint()..style = PaintingStyle.fill;
    for (var i = 0; i < 24; i++) {
      final phase = time * math.pi * 2 + i * math.pi / 12;
      final radius = outerRadius + math.sin(phase * 1.7) * 9;
      final point = Offset(
        center.dx + math.cos(phase) * radius,
        center.dy + math.sin(phase) * radius * 0.33,
      );
      particlePaint.color = accent.withValues(alpha: 0.22 + energy * 0.48);
      canvas.drawCircle(point, 1.6 + energy * 2.6, particlePaint);
    }

    final corePaint = Paint()
      ..shader = const RadialGradient(
        colors: [Color(0xFF111A2D), Color(0xFF070912), Color(0xFF000000)],
        stops: [0, 0.62, 1],
      ).createShader(Rect.fromCircle(center: center, radius: coreRadius));
    canvas.drawCircle(center, coreRadius, corePaint);
  }

  void _paintGasPlanet(
    Canvas canvas,
    Offset center,
    double shortest,
    Color accent,
    double energy,
    double closeness,
  ) {
    final ringRadius = shortest * (0.51 - closeness * 0.09);
    final planetRadius = shortest * 0.31;
    final ringPaint = Paint()
      ..style = PaintingStyle.stroke
      ..strokeWidth = 13 + energy * 9
      ..strokeCap = StrokeCap.round
      ..color = accent.withValues(alpha: 0.38 + energy * 0.28)
      ..maskFilter = MaskFilter.blur(BlurStyle.normal, 4 + energy * 8);

    for (var i = 0; i < 3; i++) {
      final rect = Rect.fromCenter(
        center: center,
        width: ringRadius * (2.3 - i * 0.18),
        height: ringRadius * (0.52 + i * 0.04),
      );
      canvas.drawArc(rect, math.pi * 0.08, math.pi * 0.86, false, ringPaint);
      canvas.drawArc(rect, math.pi * 1.08, math.pi * 0.86, false, ringPaint);
    }

    final planetPaint = Paint()
      ..shader = RadialGradient(
        center: const Alignment(-0.36, -0.42),
        colors: [
          const Color(0xFF51E4FF).withValues(alpha: 0.76 + energy * 0.18),
          Color.lerp(const Color(0xFF202B56), accent, energy * 0.36)!,
          const Color(0xFF090E22),
        ],
        stops: const [0, 0.56, 1],
      ).createShader(Rect.fromCircle(center: center, radius: planetRadius));
    canvas.drawCircle(center, planetRadius, planetPaint);
  }

  void _paintSun(
    Canvas canvas,
    Offset center,
    double shortest,
    Color accent,
    double energy,
    double closeness,
  ) {
    final sunRadius = shortest * 0.33;
    final flarePaint = Paint()
      ..style = PaintingStyle.stroke
      ..strokeCap = StrokeCap.round
      ..maskFilter = MaskFilter.blur(BlurStyle.normal, 7 + energy * 10);

    for (var i = 0; i < 56; i++) {
      final angle = time * math.pi * 0.5 + i * math.pi * 2 / 56;
      final wobble = math.sin(i * 1.9 + time * math.pi * 6) * 0.12;
      final startRadius = sunRadius * (0.92 + wobble * 0.2);
      final endRadius = sunRadius * (1.12 + energy * 0.72 + wobble);
      final start = Offset(
        center.dx + math.cos(angle) * startRadius,
        center.dy + math.sin(angle) * startRadius,
      );
      final end = Offset(
        center.dx + math.cos(angle) * endRadius,
        center.dy + math.sin(angle) * endRadius,
      );
      flarePaint
        ..color = accent.withValues(alpha: 0.08 + energy * 0.24)
        ..strokeWidth = 3 + energy * 4;
      canvas.drawLine(start, end, flarePaint);
    }

    final glowPaint = Paint()
      ..shader = RadialGradient(
        colors: [
          accent.withValues(alpha: 0.35 + energy * 0.36),
          const Color(0xFFFF8A00).withValues(alpha: 0.12 + energy * 0.24),
          Colors.transparent,
        ],
      ).createShader(Rect.fromCircle(center: center, radius: sunRadius * 1.95))
      ..maskFilter = MaskFilter.blur(BlurStyle.normal, 20 + energy * 18);
    canvas.drawCircle(center, sunRadius * 1.45, glowPaint);

    final coreColor = Color.lerp(
      const Color(0xFF5A2608),
      const Color(0xFFFFCB18),
      closeness * 0.82 + energy * 0.18,
    )!;
    final corePaint = Paint()
      ..shader = RadialGradient(
        center: const Alignment(-0.35, -0.42),
        colors: [
          Color.lerp(coreColor, Colors.white, energy * 0.22)!,
          coreColor,
          const Color(0xFF341305),
        ],
        stops: const [0, 0.62, 1],
      ).createShader(Rect.fromCircle(center: center, radius: sunRadius));
    canvas.drawCircle(center, sunRadius, corePaint);
  }

  @override
  bool shouldRepaint(covariant _CelestialPainter oldDelegate) {
    return oldDelegate.profile != profile ||
        oldDelegate.reading != reading ||
        oldDelegate.time != time;
  }
}

Color _profileAccent(InstrumentProfile profile) {
  switch (profile) {
    case InstrumentProfile.strings:
      return const Color(0xFF4EDEA3);
    case InstrumentProfile.wind:
      return const Color(0xFF67E8F9);
    case InstrumentProfile.brass:
      return const Color(0xFFFFC21A);
  }
}

Color _statusColor(
  InstrumentProfile profile,
  bool hasFrequency,
  double cents,
  double energy,
) {
  if (!hasFrequency) {
    return const Color(0xFF5B6477);
  }
  if (cents.abs() <= 8 || energy > 0.72) {
    return _profileAccent(profile);
  }
  if (cents < 0) {
    return profile == InstrumentProfile.brass
        ? const Color(0xFFFF9A3D)
        : const Color(0xFF8FB5FF);
  }
  return profile == InstrumentProfile.wind
      ? const Color(0xFFFFB7DE)
      : const Color(0xFFFF8A72);
}
