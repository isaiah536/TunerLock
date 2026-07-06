import 'package:flutter/material.dart';

import '../core/theme/app_theme.dart';
import '../shared/widgets/iphone_viewport.dart';
import 'router.dart';

class AuralockApp extends StatelessWidget {
  const AuralockApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      title: 'Auralock',
      theme: AppTheme.light,
      builder: (context, child) {
        return IphoneViewport(child: child ?? const SizedBox.shrink());
      },
      home: TunerAppRouter.home,
    );
  }
}
