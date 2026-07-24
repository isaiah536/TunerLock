import 'dart:async';

import 'package:flutter/material.dart';

import '../../../services/tracking_engine_bridge.dart';
import '../../../services/tracking_engine_service.dart';
import '../../settings/screens/settings_screen.dart';
import '../models/tuner_reading.dart';
import '../providers/tuner_state.dart';
import '../widgets/bottom_tabs.dart';
import '../widgets/celestial_tuner_view.dart';

class TunerHomeScreen extends StatefulWidget {
  const TunerHomeScreen({super.key});

  @override
  State<TunerHomeScreen> createState() => _TunerHomeScreenState();
}

class _TunerHomeScreenState extends State<TunerHomeScreen> {
  static const _uiUpdateInterval = Duration(milliseconds: 120);
  static const _confidenceBlend = 0.3;

  final TrackingEngineService _engine = TrackingEngineService();
  TunerReading _reading = TunerState.previewReading;
  InstrumentProfile _profile = InstrumentProfile.strings;
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
    final reading = _reading;

    return Scaffold(
      backgroundColor: const Color(0xFF050812),
      body: DecoratedBox(
        decoration: BoxDecoration(
          gradient: RadialGradient(
            center: const Alignment(0, -0.2),
            radius: 1.2,
            colors: _backgroundColors(_profile),
          ),
        ),
        child: SafeArea(
          child: LayoutBuilder(
            builder: (context, constraints) {
              return Column(
                children: [
                  _TopBar(profile: _profile),
                  Expanded(
                    child: Center(
                      child: SingleChildScrollView(
                        padding: const EdgeInsets.fromLTRB(22, 8, 22, 18),
                        child: ConstrainedBox(
                          constraints: BoxConstraints(
                            minHeight: (constraints.maxHeight - 198).clamp(
                              420.0,
                              720.0,
                            ),
                          ),
                          child: Column(
                            mainAxisAlignment: MainAxisAlignment.center,
                            children: [
                              CelestialTunerView(
                                reading: reading,
                                profile: _profile,
                              ),
                              const SizedBox(height: 22),
                              _DeviationPanel(
                                reading: reading,
                                accent: _profileAccent(_profile),
                              ),
                            ],
                          ),
                        ),
                      ),
                    ),
                  ),
                  BottomTabs(
                    activeProfile: _profile,
                    onProfileSelected: _selectProfile,
                  ),
                ],
              );
            },
          ),
        ),
      ),
    );
  }

  void _selectProfile(InstrumentProfile profile) {
    if (_profile == profile) {
      return;
    }
    setState(() {
      _profile = profile;
      _reading = TunerState.previewReading;
      _lastUiUpdate = null;
    });
    _engine.setInstrumentProfile(profile);
  }
}

class _TopBar extends StatelessWidget {
  const _TopBar({required this.profile});

  final InstrumentProfile profile;

  @override
  Widget build(BuildContext context) {
    final accent = _profileAccent(profile);
    return Padding(
      padding: const EdgeInsets.fromLTRB(22, 14, 16, 8),
      child: Row(
        children: [
          Icon(Icons.graphic_eq_rounded, color: accent, size: 27),
          const SizedBox(width: 10),
          const Expanded(
            child: Text(
              'Auralock',
              maxLines: 1,
              overflow: TextOverflow.ellipsis,
              style: TextStyle(
                color: Color(0xFFE8EEFF),
                fontSize: 24,
                fontWeight: FontWeight.w800,
              ),
            ),
          ),
          _ModeChip(profile: profile),
          const SizedBox(width: 8),
          IconButton(
            tooltip: 'Settings',
            onPressed: () {
              Navigator.of(context).push(
                MaterialPageRoute<void>(
                  builder: (context) => const SettingsScreen(),
                ),
              );
            },
            icon: const Icon(Icons.settings_rounded),
            color: Colors.white.withValues(alpha: 0.78),
            style: IconButton.styleFrom(
              backgroundColor: Colors.white.withValues(alpha: 0.08),
              fixedSize: const Size(44, 44),
              shape: RoundedRectangleBorder(
                borderRadius: BorderRadius.circular(8),
                side: BorderSide(color: Colors.white.withValues(alpha: 0.12)),
              ),
            ),
          ),
        ],
      ),
    );
  }
}

class _ModeChip extends StatelessWidget {
  const _ModeChip({required this.profile});

  final InstrumentProfile profile;

  @override
  Widget build(BuildContext context) {
    final accent = _profileAccent(profile);
    return Container(
      height: 38,
      padding: const EdgeInsets.symmetric(horizontal: 14),
      decoration: BoxDecoration(
        color: Colors.white.withValues(alpha: 0.08),
        border: Border.all(color: Colors.white.withValues(alpha: 0.13)),
        borderRadius: BorderRadius.circular(8),
      ),
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          Container(
            width: 9,
            height: 9,
            decoration: BoxDecoration(color: accent, shape: BoxShape.circle),
          ),
          const SizedBox(width: 8),
          Text(
            _profileLabel(profile),
            style: TextStyle(
              color: Colors.white.withValues(alpha: 0.86),
              fontSize: 12,
              fontWeight: FontWeight.w800,
            ),
          ),
        ],
      ),
    );
  }
}

class _DeviationPanel extends StatelessWidget {
  const _DeviationPanel({required this.reading, required this.accent});

  final TunerReading reading;
  final Color accent;

  @override
  Widget build(BuildContext context) {
    final hasFrequency = reading.currentFrequency > 0;
    final showLockedHz = hasFrequency && reading.isLocked;
    final lockState = hasFrequency
        ? (reading.isLocked ? 'LOCKED' : 'TRACKING')
        : 'IDLE';
    final markerPosition = hasFrequency
        ? ((reading.cents.clamp(-50.0, 50.0) + 50.0) / 100.0)
        : 0.5;
    final markerColor = showLockedHz ? accent : const Color(0xFFFFA69E);

    return ConstrainedBox(
      constraints: const BoxConstraints(maxWidth: 430),
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          Padding(
            padding: const EdgeInsets.symmetric(horizontal: 16),
            child: Row(
              children: [
                Expanded(
                  child: _LockStateValue(
                    value: lockState,
                    active: showLockedHz,
                    accent: accent,
                  ),
                ),
                _ConfidenceBars(
                  confidence: hasFrequency ? reading.confidence : 0,
                  accent: accent,
                ),
              ],
            ),
          ),
          const SizedBox(height: 18),
          SizedBox(
            height: 48,
            child: LayoutBuilder(
              builder: (context, constraints) {
                const markerSize = 32.0;
                final left =
                    markerPosition * (constraints.maxWidth - markerSize);
                return Stack(
                  alignment: Alignment.center,
                  children: [
                    Container(
                      height: 48,
                      decoration: BoxDecoration(
                        color: const Color(0xFF182034).withValues(alpha: 0.9),
                        border: Border.all(
                          color: Colors.white.withValues(alpha: 0.11),
                        ),
                        borderRadius: BorderRadius.circular(24),
                      ),
                    ),
                    Positioned(
                      left: 26,
                      child: Text('-50', style: _scaleTextStyle()),
                    ),
                    Positioned(
                      right: 26,
                      child: Text('+50', style: _scaleTextStyle()),
                    ),
                    Positioned(
                      left: constraints.maxWidth / 2 - 0.5,
                      top: 0,
                      bottom: 0,
                      child: Container(
                        width: 1,
                        color: accent.withValues(alpha: 0.58),
                      ),
                    ),
                    Positioned(
                      left: constraints.maxWidth / 2 + 5,
                      top: 17,
                      child: Text(
                        '0',
                        style: TextStyle(
                          color: accent.withValues(alpha: 0.78),
                          fontSize: 13,
                          fontWeight: FontWeight.w800,
                        ),
                      ),
                    ),
                    Positioned(
                      left: left,
                      top: 8,
                      child: Container(
                        width: markerSize,
                        height: markerSize,
                        decoration: BoxDecoration(
                          color: markerColor,
                          shape: BoxShape.circle,
                          boxShadow: [
                            BoxShadow(
                              color: accent.withValues(
                                alpha: showLockedHz ? 0.48 : 0.26,
                              ),
                              blurRadius: showLockedHz ? 24 : 18,
                              spreadRadius: showLockedHz ? 4 : 1,
                            ),
                          ],
                        ),
                      ),
                    ),
                  ],
                );
              },
            ),
          ),
          const SizedBox(height: 15),
          Text(
            'EVENT HORIZON DEVIATION',
            style: TextStyle(
              color: Colors.white.withValues(alpha: 0.42),
              fontSize: 12,
              fontWeight: FontWeight.w900,
            ),
          ),
        ],
      ),
    );
  }
}

class _LockStateValue extends StatelessWidget {
  const _LockStateValue({
    required this.value,
    required this.active,
    required this.accent,
  });

  final String value;
  final bool active;
  final Color accent;

  @override
  Widget build(BuildContext context) {
    final color = active
        ? const Color(0xFFE8EEFF)
        : Colors.white.withValues(alpha: 0.36);
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Text('LOCK STATE', style: _panelLabelStyle()),
        const SizedBox(height: 5),
        Text(
          value,
          maxLines: 1,
          overflow: TextOverflow.ellipsis,
          style: TextStyle(
            color: color,
            fontSize: 25,
            fontWeight: FontWeight.w900,
            height: 0.9,
            shadows: active
                ? [Shadow(color: accent.withValues(alpha: 0.2), blurRadius: 14)]
                : null,
          ),
        ),
      ],
    );
  }
}

class _ConfidenceBars extends StatelessWidget {
  const _ConfidenceBars({required this.confidence, required this.accent});

  final double confidence;
  final Color accent;

  @override
  Widget build(BuildContext context) {
    final activeBars = (confidence.clamp(0.0, 1.0) * 5).ceil();
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Text('CONFIDENCE', style: _panelLabelStyle()),
        const SizedBox(height: 9),
        Row(
          children: List.generate(5, (index) {
            final active = index < activeBars;
            return Padding(
              padding: EdgeInsets.only(right: index == 4 ? 0 : 5),
              child: Container(
                width: 6,
                height: index.isEven ? 17 : 14,
                decoration: BoxDecoration(
                  color: active ? accent : Colors.white.withValues(alpha: 0.18),
                  borderRadius: BorderRadius.circular(6),
                ),
              ),
            );
          }),
        ),
      ],
    );
  }
}

TextStyle _panelLabelStyle() {
  return TextStyle(
    color: Colors.white.withValues(alpha: 0.62),
    fontSize: 12,
    fontWeight: FontWeight.w900,
  );
}

TextStyle _scaleTextStyle() {
  return TextStyle(
    color: Colors.white.withValues(alpha: 0.34),
    fontSize: 13,
    fontWeight: FontWeight.w800,
  );
}

List<Color> _backgroundColors(InstrumentProfile profile) {
  switch (profile) {
    case InstrumentProfile.strings:
      return const [Color(0xFF182445), Color(0xFF07101F), Color(0xFF03050B)];
    case InstrumentProfile.wind:
      return const [Color(0xFF203C56), Color(0xFF111633), Color(0xFF050814)];
    case InstrumentProfile.brass:
      return const [Color(0xFF4C1F0A), Color(0xFF1A0A0A), Color(0xFF050407)];
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

String _profileLabel(InstrumentProfile profile) {
  switch (profile) {
    case InstrumentProfile.strings:
      return 'STRINGS';
    case InstrumentProfile.wind:
      return 'WIND';
    case InstrumentProfile.brass:
      return 'BRASS';
  }
}
