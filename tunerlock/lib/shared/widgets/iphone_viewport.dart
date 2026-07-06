import 'dart:math' as math;

import 'package:flutter/material.dart';

class IphoneViewport extends StatelessWidget {
  const IphoneViewport({required this.child, super.key});

  static const double aspectRatio = 9 / 19.5;

  final Widget child;

  @override
  Widget build(BuildContext context) {
    return ColoredBox(
      color: Theme.of(context).scaffoldBackgroundColor,
      child: Center(
        child: LayoutBuilder(
          builder: (context, constraints) {
            final width = math.min(
              constraints.maxWidth,
              constraints.maxHeight * aspectRatio,
            );
            final height = width / aspectRatio;

            return SizedBox(width: width, height: height, child: child);
          },
        ),
      ),
    );
  }
}
