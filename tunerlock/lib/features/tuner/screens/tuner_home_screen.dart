import 'package:flutter/material.dart';

import '../../../core/constants/tuner_constants.dart';
import '../../../shared/widgets/glass_panel.dart';
import '../../../shared/widgets/paper_background.dart';
import '../providers/tuner_state.dart';
import '../widgets/bottom_tabs.dart';
import '../widgets/frequency_readout.dart';
import '../widgets/tracking_status_card.dart';
import '../widgets/tuner_header.dart';

class TunerHomeScreen extends StatelessWidget {
  const TunerHomeScreen({super.key});

  @override
  Widget build(BuildContext context) {
    final reading = TunerState.previewReading;

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
