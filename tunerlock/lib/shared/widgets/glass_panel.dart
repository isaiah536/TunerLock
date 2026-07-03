import 'package:flutter/material.dart';

class GlassPanel extends StatelessWidget {
  const GlassPanel({
    required this.child,
    this.padding = EdgeInsets.zero,
    this.borderRadius = 18,
    super.key,
  });

  final Widget child;
  final EdgeInsets padding;
  final double borderRadius;

  @override
  Widget build(BuildContext context) {
    return Container(
      padding: padding,
      decoration: BoxDecoration(
        color: const Color(0xB8FFF9EB),
        borderRadius: BorderRadius.circular(borderRadius),
        border: Border.all(color: const Color(0x87534F46), width: 1.4),
        boxShadow: const [
          BoxShadow(
            color: Color(0x1F2E2A24),
            blurRadius: 12,
            offset: Offset(0, 5),
          ),
          BoxShadow(
            color: Color(0xAAFFFDF3),
            blurRadius: 8,
            offset: Offset(-2, -2),
          ),
        ],
      ),
      child: child,
    );
  }
}

class GlassPill extends StatelessWidget {
  const GlassPill({required this.child, super.key});

  final Widget child;

  @override
  Widget build(BuildContext context) {
    return GlassPanel(
      borderRadius: 28,
      padding: const EdgeInsets.symmetric(horizontal: 18, vertical: 8),
      child: child,
    );
  }
}
