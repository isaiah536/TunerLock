import 'package:flutter/material.dart';

class BottomTabs extends StatelessWidget {
  const BottomTabs({super.key});

  @override
  Widget build(BuildContext context) {
    return Container(
      height: 86,
      decoration: const BoxDecoration(
        color: Color(0xDDF8F0DC),
        border: Border(top: BorderSide(color: Color(0x55544F46))),
      ),
      child: const Row(
        mainAxisAlignment: MainAxisAlignment.spaceEvenly,
        children: [
          BottomTab(icon: Icons.graphic_eq_rounded, label: 'Tuner', active: true),
          BottomTab(icon: Icons.notifications_none_rounded, label: 'Tone'),
          BottomTab(icon: Icons.album_outlined, label: 'Record'),
          BottomTab(icon: Icons.history_rounded, label: 'History'),
        ],
      ),
    );
  }
}

class BottomTab extends StatelessWidget {
  const BottomTab({
    required this.icon,
    required this.label,
    this.active = false,
    super.key,
  });

  final IconData icon;
  final String label;
  final bool active;

  @override
  Widget build(BuildContext context) {
    final color = active ? const Color(0xFF24231F) : const Color(0xFF6E6A60);
    return SizedBox(
      width: 72,
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Icon(icon, size: 34, color: color),
          const SizedBox(height: 5),
          Text(
            label,
            style: TextStyle(
              fontSize: 15,
              fontWeight: active ? FontWeight.w700 : FontWeight.w500,
              color: color,
            ),
          ),
        ],
      ),
    );
  }
}
