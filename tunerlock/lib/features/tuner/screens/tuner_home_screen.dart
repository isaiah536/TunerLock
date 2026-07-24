import 'dart:async';
import 'dart:math' as math;

import 'package:flutter/material.dart';

import '../../../services/tracking_engine_bridge.dart';
import '../../../services/tracking_engine_service.dart';
import '../../settings/screens/settings_screen.dart';
import '../models/tuner_reading.dart';
import '../providers/tuner_state.dart';
import '../widgets/bottom_tabs.dart';
import '../widgets/score_tuner_view.dart';

class TunerHomeScreen extends StatefulWidget {
  const TunerHomeScreen({super.key});

  @override
  State<TunerHomeScreen> createState() => _TunerHomeScreenState();
}

class _TunerHomeScreenState extends State<TunerHomeScreen> {
  static const _uiUpdateInterval = Duration(milliseconds: 120);
  static const _confidenceBlend = 0.3;
  static const _maxPitchTrailSamples = 72;

  final TrackingEngineService _engine = TrackingEngineService();
  TunerReading _reading = TunerState.previewReading;
  InstrumentProfile _profile = InstrumentProfile.strings;
  TrackingMode _mode = TrackingMode.tuning;
  double _referencePitchHz = 440.0;
  final List<double> _pitchTrailCents = <double>[];
  StreamSubscription<TunerReading>? _subscription;
  DateTime? _lastUiUpdate;

  @override
  void initState() {
    super.initState();
    if (!_engine.isAvailable) {
      return;
    }
    _subscription = _engine.readings.listen((reading) {
      if (!mounted) {
        return;
      }
      final now = DateTime.now();
      final lockChanged = reading.isLocked != _reading.isLocked;
      final detectionChanged =
          (reading.currentFrequency > 0) != (_reading.currentFrequency > 0);
      final updateDue =
          _lastUiUpdate == null ||
          now.difference(_lastUiUpdate!) >= _uiUpdateInterval;
      if (!lockChanged && !detectionChanged && !updateDue) {
        return;
      }

      final displayedConfidence =
          _reading.confidence * (1 - _confidenceBlend) +
          reading.confidence * _confidenceBlend;
      setState(() {
        _reading = reading.copyWith(confidence: displayedConfidence);
        _appendPitchTrail(reading);
        _lastUiUpdate = now;
      });
    });
    unawaited(_engine.startListening());
  }

  @override
  void dispose() {
    unawaited(_subscription?.cancel());
    unawaited(_engine.dispose());
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: const Color(0xFFF7F0E2),
      body: Stack(
        children: [
          const Positioned.fill(child: CustomPaint(painter: _PaperPainter())),
          SafeArea(
            child: Column(
              children: [
                _TopBar(
                  referencePitchHz: _referencePitchHz,
                  onReferencePitchChanged: _selectReferencePitch,
                ),
                Expanded(
                  child: LayoutBuilder(
                    builder: (context, constraints) {
                      return SingleChildScrollView(
                        padding: const EdgeInsets.fromLTRB(30, 34, 30, 28),
                        child: ConstrainedBox(
                          constraints: BoxConstraints(
                            minHeight: (constraints.maxHeight - 18).clamp(
                              600.0,
                              820.0,
                            ),
                          ),
                          child: ScoreTunerView(
                            reading: _reading,
                            profile: _profile,
                            mode: _mode,
                            pitchTrailCents: _pitchTrailCents,
                            onModeChanged: _selectMode,
                          ),
                        ),
                      );
                    },
                  ),
                ),
                BottomTabs(
                  activeProfile: _profile,
                  onProfileSelected: _selectProfile,
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }

  void _selectMode(TrackingMode mode) {
    if (_mode == mode) {
      return;
    }
    setState(() {
      _mode = mode;
      _reading = TunerState.previewReading;
      _lastUiUpdate = null;
      _pitchTrailCents.clear();
    });
    _engine.setTrackingProfile(profile: _profile, mode: mode);
  }

  void _selectProfile(InstrumentProfile profile) {
    if (_profile == profile) {
      return;
    }
    setState(() {
      _profile = profile;
      _reading = TunerState.previewReading;
      _lastUiUpdate = null;
      _pitchTrailCents.clear();
    });
    _engine.setTrackingProfile(profile: profile, mode: _mode);
  }

  void _selectReferencePitch(double referencePitchHz) {
    if (_referencePitchHz == referencePitchHz) {
      return;
    }
    setState(() {
      _referencePitchHz = referencePitchHz;
      _reading = TunerState.previewReading;
      _lastUiUpdate = null;
      _pitchTrailCents.clear();
    });
    _engine.setReferencePitch(referencePitchHz);
  }

  void _appendPitchTrail(TunerReading reading) {
    if (reading.currentFrequency <= 0) {
      return;
    }
    _pitchTrailCents.add(reading.cents.clamp(-80.0, 80.0));
    if (_pitchTrailCents.length > _maxPitchTrailSamples) {
      _pitchTrailCents.removeRange(
        0,
        _pitchTrailCents.length - _maxPitchTrailSamples,
      );
    }
  }
}

class _TopBar extends StatelessWidget {
  const _TopBar({
    required this.referencePitchHz,
    required this.onReferencePitchChanged,
  });

  final double referencePitchHz;
  final ValueChanged<double> onReferencePitchChanged;

  @override
  Widget build(BuildContext context) {
    return Container(
      height: 72,
      padding: const EdgeInsets.symmetric(horizontal: 24),
      decoration: BoxDecoration(
        border: Border(
          bottom: BorderSide(
            color: const Color(0xFFB8B0A4).withValues(alpha: 0.5),
          ),
        ),
      ),
      child: Row(
        children: [
          _RoundGlassIconButton(
            tooltip: 'Menu',
            icon: Icons.menu_rounded,
            onTap: () {},
          ),
          Expanded(
            child: Center(
              child: _ReferencePitchSelector(
                referencePitchHz: referencePitchHz,
                onReferencePitchChanged: onReferencePitchChanged,
              ),
            ),
          ),
          _RoundGlassIconButton(
            tooltip: 'Settings',
            icon: Icons.settings_outlined,
            onTap: () {
              Navigator.of(context).push(
                MaterialPageRoute<void>(
                  builder: (context) => const SettingsScreen(),
                ),
              );
            },
          ),
        ],
      ),
    );
  }
}

class _RoundGlassIconButton extends StatefulWidget {
  const _RoundGlassIconButton({
    required this.icon,
    required this.tooltip,
    this.onTap,
  });

  final IconData icon;
  final String tooltip;
  final VoidCallback? onTap;

  @override
  State<_RoundGlassIconButton> createState() => _RoundGlassIconButtonState();
}

class _RoundGlassIconButtonState extends State<_RoundGlassIconButton> {
  bool _pressed = false;

  void _setPressed(bool value) {
    if (_pressed == value) {
      return;
    }
    setState(() {
      _pressed = value;
    });
  }

  @override
  Widget build(BuildContext context) {
    return Tooltip(
      message: widget.tooltip,
      child: GestureDetector(
        behavior: HitTestBehavior.opaque,
        onTapDown: (_) => _setPressed(true),
        onTapUp: (_) => _setPressed(false),
        onTapCancel: () => _setPressed(false),
        onTap: widget.onTap,
        child: AnimatedScale(
          scale: _pressed ? 0.96 : 1,
          duration: const Duration(milliseconds: 100),
          curve: Curves.easeOut,
          child: SizedBox(
            width: 46,
            height: 46,
            child: CustomPaint(
              painter: _RoundGlassButtonPainter(pressed: _pressed),
              child: Icon(
                widget.icon,
                color: const Color(0xFF2A2927),
                size: 25,
              ),
            ),
          ),
        ),
      ),
    );
  }
}

class _RoundGlassButtonPainter extends CustomPainter {
  const _RoundGlassButtonPainter({required this.pressed});

  final bool pressed;

  @override
  void paint(Canvas canvas, Size size) {
    final rect = Offset.zero & size;
    final shape = RRect.fromRectAndRadius(
      rect.deflate(4),
      Radius.circular(size.height / 2),
    );

    final shadowPaint = Paint()
      ..color = const Color(0xFF6C6258).withValues(alpha: pressed ? 0.10 : 0.15)
      ..maskFilter = MaskFilter.blur(BlurStyle.normal, pressed ? 4 : 6);
    canvas.drawRRect(
      shape.shift(pressed ? const Offset(0, 1.5) : const Offset(0, 3)),
      shadowPaint,
    );

    final surfacePaint = Paint()
      ..shader = const LinearGradient(
        begin: Alignment.topLeft,
        end: Alignment.bottomRight,
        colors: [Color(0xBFECE6DC), Color(0x99D8D1C6), Color(0x88C9C2B7)],
        stops: [0, 0.56, 1],
      ).createShader(rect);
    canvas.drawRRect(shape, surfacePaint);

    final edgePaint = Paint()
      ..style = PaintingStyle.stroke
      ..strokeWidth = 1.0
      ..color = Colors.black.withValues(alpha: pressed ? 0.16 : 0.08)
      ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 0.8);
    canvas.drawRRect(shape.deflate(1.4), edgePaint);

    final highlightRect = Rect.fromLTRB(
      shape.left + 6,
      shape.top + 4,
      shape.right - 6,
      shape.top + 15,
    );
    final highlightPaint = Paint()
      ..shader = LinearGradient(
        begin: Alignment.topCenter,
        end: Alignment.bottomCenter,
        colors: [
          Colors.white.withValues(alpha: pressed ? 0.18 : 0.32),
          Colors.white.withValues(alpha: 0),
        ],
      ).createShader(highlightRect)
      ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 2.5);
    canvas.drawRRect(
      RRect.fromRectAndRadius(highlightRect, Radius.circular(size.height / 2)),
      highlightPaint,
    );

    final reflectionRect = Rect.fromLTRB(
      shape.left + 7,
      size.height * 0.62,
      shape.right - 7,
      shape.bottom - 4,
    );
    final reflectionPaint = Paint()
      ..shader = LinearGradient(
        begin: Alignment.topCenter,
        end: Alignment.bottomCenter,
        colors: [
          Colors.white.withValues(alpha: 0),
          Colors.white.withValues(alpha: pressed ? 0.03 : 0.07),
        ],
      ).createShader(reflectionRect)
      ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 2.5);
    canvas.drawRRect(
      RRect.fromRectAndRadius(reflectionRect, Radius.circular(size.height / 2)),
      reflectionPaint,
    );
  }

  @override
  bool shouldRepaint(covariant _RoundGlassButtonPainter oldDelegate) {
    return oldDelegate.pressed != pressed;
  }
}

class _ReferencePitchSelector extends StatelessWidget {
  const _ReferencePitchSelector({
    required this.referencePitchHz,
    required this.onReferencePitchChanged,
  });

  static const _minReferencePitchHz = 338;
  static const _maxReferencePitchHz = 466;

  final double referencePitchHz;
  final ValueChanged<double> onReferencePitchChanged;

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      behavior: HitTestBehavior.opaque,
      onTap: () => _showReferencePitchPicker(context),
      child: Padding(
        padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 10),
        child: Row(
          mainAxisSize: MainAxisSize.min,
          children: [
            Text(
              'A = ${referencePitchHz.toStringAsFixed(0)} Hz',
              style: const TextStyle(
                color: Color(0xFF151719),
                fontSize: 20,
                fontWeight: FontWeight.w600,
                fontFamily: 'Georgia',
              ),
            ),
            const SizedBox(width: 3),
            const Icon(
              Icons.keyboard_arrow_down_rounded,
              color: Color(0xFF6B655D),
              size: 20,
            ),
          ],
        ),
      ),
    );
  }

  Future<void> _showReferencePitchPicker(BuildContext context) async {
    final initial = referencePitchHz.round().clamp(
      _minReferencePitchHz,
      _maxReferencePitchHz,
    );
    var selectedHz = initial;
    final controller = FixedExtentScrollController(
      initialItem: initial - _minReferencePitchHz,
    );

    final result = await showModalBottomSheet<int>(
      context: context,
      backgroundColor: Colors.transparent,
      builder: (context) {
        return StatefulBuilder(
          builder: (context, setPickerState) {
            return Container(
              height: 310,
              margin: const EdgeInsets.fromLTRB(14, 0, 14, 14),
              decoration: BoxDecoration(
                color: const Color(0xFFFFF8ED),
                borderRadius: BorderRadius.circular(18),
                border: Border.all(
                  color: const Color(0xFF9B9284).withValues(alpha: 0.25),
                ),
                boxShadow: [
                  BoxShadow(
                    color: Colors.black.withValues(alpha: 0.18),
                    blurRadius: 24,
                    offset: const Offset(0, 10),
                  ),
                ],
              ),
              child: Column(
                children: [
                  Padding(
                    padding: const EdgeInsets.fromLTRB(20, 16, 12, 6),
                    child: Row(
                      children: [
                        const Expanded(
                          child: Text(
                            'Reference Pitch',
                            style: TextStyle(
                              color: Color(0xFF151719),
                              fontSize: 20,
                              fontWeight: FontWeight.w700,
                              fontFamily: 'Georgia',
                            ),
                          ),
                        ),
                        TextButton(
                          onPressed: () =>
                              Navigator.of(context).pop(selectedHz),
                          child: const Text('Done'),
                        ),
                      ],
                    ),
                  ),
                  Expanded(
                    child: Stack(
                      alignment: Alignment.center,
                      children: [
                        Container(
                          height: 48,
                          margin: const EdgeInsets.symmetric(horizontal: 44),
                          decoration: BoxDecoration(
                            color: const Color(
                              0xFFE7DED0,
                            ).withValues(alpha: 0.45),
                            borderRadius: BorderRadius.circular(8),
                          ),
                        ),
                        ListWheelScrollView.useDelegate(
                          controller: controller,
                          itemExtent: 44,
                          physics: const FixedExtentScrollPhysics(),
                          perspective: 0.0025,
                          diameterRatio: 1.25,
                          onSelectedItemChanged: (index) {
                            setPickerState(() {
                              selectedHz = _minReferencePitchHz + index;
                            });
                          },
                          childDelegate: ListWheelChildBuilderDelegate(
                            childCount:
                                _maxReferencePitchHz - _minReferencePitchHz + 1,
                            builder: (context, index) {
                              if (index < 0) {
                                return null;
                              }
                              final hz = _minReferencePitchHz + index;
                              return Center(
                                child: Text(
                                  'A = $hz Hz',
                                  style: TextStyle(
                                    color: hz == selectedHz
                                        ? const Color(0xFF151719)
                                        : const Color(0xFF6C665E),
                                    fontSize: hz == selectedHz ? 24 : 20,
                                    fontWeight: hz == selectedHz
                                        ? FontWeight.w800
                                        : FontWeight.w500,
                                    fontFamily: 'Georgia',
                                  ),
                                ),
                              );
                            },
                          ),
                        ),
                      ],
                    ),
                  ),
                ],
              ),
            );
          },
        );
      },
    );

    if (result != null) {
      onReferencePitchChanged(result.toDouble());
    }
  }
}

class _PaperPainter extends CustomPainter {
  const _PaperPainter();

  @override
  void paint(Canvas canvas, Size size) {
    final background = Paint()
      ..shader = const LinearGradient(
        begin: Alignment.topCenter,
        end: Alignment.bottomCenter,
        colors: [Color(0xFFFBF4E8), Color(0xFFF2E9DA)],
      ).createShader(Offset.zero & size);
    canvas.drawRect(Offset.zero & size, background);

    final vignette = Paint()
      ..shader = RadialGradient(
        radius: 0.95,
        colors: [
          Colors.transparent,
          const Color(0xFF9F8D72).withValues(alpha: 0.16),
        ],
      ).createShader(Offset.zero & size);
    canvas.drawRect(Offset.zero & size, vignette);

    final speckPaint = Paint()..style = PaintingStyle.fill;
    for (var i = 0; i < 170; i++) {
      final x = (math.sin(i * 19.19) * 0.5 + 0.5) * size.width;
      final y = (math.cos(i * 37.77) * 0.5 + 0.5) * size.height;
      final opacity = 0.035 + (i % 5) * 0.014;
      speckPaint.color = const Color(0xFF383028).withValues(alpha: opacity);
      canvas.drawCircle(Offset(x, y), 0.45 + (i % 3) * 0.35, speckPaint);
    }
  }

  @override
  bool shouldRepaint(covariant _PaperPainter oldDelegate) {
    return false;
  }
}
