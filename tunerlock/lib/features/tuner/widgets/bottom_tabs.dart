import 'package:flutter/material.dart';

import '../../../services/tracking_engine_bridge.dart';
import 'score_tuner_view.dart';

class BottomTabs extends StatelessWidget {
  const BottomTabs({
    required this.activeProfile,
    required this.onProfileSelected,
    this.onMetronomeTap,
    super.key,
  });

  final InstrumentProfile activeProfile;
  final ValueChanged<InstrumentProfile> onProfileSelected;
  final VoidCallback? onMetronomeTap;

  @override
  Widget build(BuildContext context) {
    return Container(
      height: 92,
      margin: const EdgeInsets.fromLTRB(34, 0, 34, 10),
      decoration: BoxDecoration(
        gradient: const LinearGradient(
          begin: Alignment.topLeft,
          end: Alignment.bottomRight,
          colors: [Color(0xFFE5E0D7), Color(0xFFC9C2B7)],
        ),
        border: Border.all(color: Colors.white.withValues(alpha: 0.50)),
        borderRadius: BorderRadius.circular(34),
        boxShadow: [
          BoxShadow(
            color: const Color(0xFF6C6258).withValues(alpha: 0.24),
            blurRadius: 18,
            offset: const Offset(0, 8),
          ),
          BoxShadow(
            color: Colors.white.withValues(alpha: 0.48),
            blurRadius: 8,
            offset: const Offset(-2, -2),
          ),
        ],
      ),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceEvenly,
        children: [
          BottomTab(
            icon: Icons.music_note_rounded,
            label: 'STRINGS',
            active: activeProfile == InstrumentProfile.strings,
            activeColor: profileAccent(InstrumentProfile.strings),
            onTap: () => onProfileSelected(InstrumentProfile.strings),
          ),
          BottomTab(
            icon: Icons.air_rounded,
            label: 'WIND',
            active: activeProfile == InstrumentProfile.wind,
            activeColor: profileAccent(InstrumentProfile.wind),
            onTap: () => onProfileSelected(InstrumentProfile.wind),
          ),
          BottomTab(
            icon: Icons.campaign_outlined,
            label: 'BRASS',
            active: activeProfile == InstrumentProfile.brass,
            activeColor: profileAccent(InstrumentProfile.brass),
            onTap: () => onProfileSelected(InstrumentProfile.brass),
          ),
          BottomTab(
            icon: Icons.av_timer_rounded,
            label: 'TEMPO',
            active: false,
            activeColor: const Color(0xFF7E786E),
            onTap: onMetronomeTap,
          ),
        ],
      ),
    );
  }
}

class BottomTab extends StatelessWidget {
  const BottomTab({
    required this.icon,
    required this.label,
    required this.activeColor,
    this.active = false,
    this.onTap,
    super.key,
  });

  final IconData icon;
  final String label;
  final Color activeColor;
  final bool active;
  final VoidCallback? onTap;

  @override
  Widget build(BuildContext context) {
    final color = active ? activeColor : const Color(0xFF817B72);
    return InkWell(
      onTap: onTap,
      borderRadius: BorderRadius.circular(26),
      child: SizedBox(
        width: 70,
        height: 78,
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            AnimatedContainer(
              duration: const Duration(milliseconds: 180),
              width: 42,
              height: 38,
              alignment: Alignment.center,
              decoration: BoxDecoration(
                shape: BoxShape.circle,
                gradient: active
                    ? const LinearGradient(
                        begin: Alignment.topLeft,
                        end: Alignment.bottomRight,
                        colors: [Color(0xFFF6F3ED), Color(0xFFCFC8BE)],
                      )
                    : null,
                border: active
                    ? Border.all(color: Colors.white.withValues(alpha: 0.48))
                    : null,
                boxShadow: active
                    ? [
                        BoxShadow(
                          color: Colors.white.withValues(alpha: 0.38),
                          blurRadius: 4,
                          offset: const Offset(-1, -1),
                        ),
                        BoxShadow(
                          color: const Color(
                            0xFF6C6258,
                          ).withValues(alpha: 0.20),
                          blurRadius: 8,
                          offset: const Offset(2, 4),
                        ),
                      ]
                    : null,
              ),
              child: Icon(icon, size: active ? 24 : 27, color: color),
            ),
            const SizedBox(height: 5),
            Text(
              label,
              maxLines: 1,
              overflow: TextOverflow.fade,
              softWrap: false,
              style: TextStyle(
                fontSize: 12,
                fontWeight: FontWeight.w800,
                color: color,
                fontFamily: 'Georgia',
              ),
            ),
            const SizedBox(height: 5),
            AnimatedContainer(
              duration: const Duration(milliseconds: 180),
              width: active ? 29 : 0,
              height: 3,
              decoration: BoxDecoration(
                color: activeColor,
                borderRadius: BorderRadius.circular(2),
              ),
            ),
          ],
        ),
      ),
    );
  }
}
