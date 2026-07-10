import 'dart:async';

import 'package:flutter/material.dart';

import '../../../core/constants/tuner_constants.dart';
import '../../../shared/widgets/glass_panel.dart';
import '../../../shared/widgets/paper_background.dart';
import '../../../services/tracking_engine_service.dart';
import '../models/tuner_reading.dart';
import '../providers/tuner_state.dart';
import '../widgets/bottom_tabs.dart';
import '../widgets/frequency_readout.dart';
import '../widgets/tracking_status_card.dart';
import '../widgets/tuner_header.dart';

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
  StreamSubscription<TunerReading>? _subscription;
  DateTime? _lastUiUpdate;

  @override
  void initState() {
    super.initState();
    if (!_engine.isAvailable) {
      return;
    }
    _subscription = _engine.readings.listen((reading) {
      if (!mounted || reading.currentFrequency <= 0) {
        return;
      }
      final now = DateTime.now();
      final lockChanged = reading.isLocked != _reading.isLocked;
      final updateDue = _lastUiUpdate == null ||
          now.difference(_lastUiUpdate!) >= _uiUpdateInterval;
      if (!lockChanged && !updateDue) {
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
      body: Stack(
        children: [
          const Positioned.fill(
            child: PaperBackground(
              pitchMarkerPosition: TunerConstants.pitchMarkerPosition,
            ),
          ),
          SafeArea(
            child: Column(
              children: [
                TunerHeader(reading: reading),
                TrackingStatusCard(reading: reading),
                const SizedBox(height: 18),
                Center(
                  child: GlassPill(
                    child: Text(
                      '${reading.note} ${reading.referenceFrequency.toStringAsFixed(1)} Hz',
                      style: const TextStyle(
                        fontSize: 16,
                        fontWeight: FontWeight.w600,
                        color: Color(0xFF34322D),
                      ),
                    ),
                  ),
                ),
                const SizedBox(height: 14),
                const Expanded(child: SizedBox.shrink()),
                FrequencyReadout(reading: reading),
                const SizedBox(height: 12),
                const BottomTabs(),
              ],
            ),
          ),
        ],
      ),
    );
  }
}
