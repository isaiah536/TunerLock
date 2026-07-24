import 'package:flutter/material.dart';

import '../../../services/tracking_engine_bridge.dart';

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
      height: 90,
      margin: const EdgeInsets.fromLTRB(18, 0, 18, 14),
      decoration: BoxDecoration(
        color: Colors.white.withValues(alpha: 0.08),
        border: Border.all(color: Colors.white.withValues(alpha: 0.13)),
        borderRadius: BorderRadius.circular(8),
        boxShadow: [
          BoxShadow(
            color: Colors.black.withValues(alpha: 0.32),
            blurRadius: 26,
            offset: const Offset(0, 10),
          ),
        ],
      ),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceEvenly,
        children: [
          BottomTab(
            icon: Icons.music_note_rounded,
            label: 'Strings',
            active: activeProfile == InstrumentProfile.strings,
            activeColor: const Color(0xFF4EDEA3),
            onTap: () => onProfileSelected(InstrumentProfile.strings),
          ),
          BottomTab(
            icon: Icons.air_rounded,
            label: 'Wind',
            active: activeProfile == InstrumentProfile.wind,
            activeColor: const Color(0xFF67E8F9),
            onTap: () => onProfileSelected(InstrumentProfile.wind),
          ),
          BottomTab(
            icon: Icons.campaign_outlined,
            label: 'Brass',
            active: activeProfile == InstrumentProfile.brass,
            activeColor: const Color(0xFFFFC21A),
            onTap: () => onProfileSelected(InstrumentProfile.brass),
          ),
          BottomTab(
            icon: Icons.timer_outlined,
            label: 'Metronom',
            active: false,
            activeColor: const Color(0xFFFFD166),
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
    final color = active ? activeColor : Colors.white.withValues(alpha: 0.46);
    return InkWell(
      onTap: onTap,
      borderRadius: BorderRadius.circular(8),
      child: SizedBox(
        width: 78,
        height: 74,
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Icon(icon, size: 27, color: color),
            const SizedBox(height: 7),
            Text(
              label,
              maxLines: 1,
              overflow: TextOverflow.fade,
              softWrap: false,
              style: TextStyle(
                fontSize: 12,
                fontWeight: active ? FontWeight.w800 : FontWeight.w600,
                color: color,
              ),
            ),
          ],
        ),
      ),
    );
  }
}
