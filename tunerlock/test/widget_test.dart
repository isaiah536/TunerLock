import 'package:flutter_test/flutter_test.dart';

import 'package:tunerlock/app/app.dart';

void main() {
  testWidgets('Tuner home renders the first design pass', (
    WidgetTester tester,
  ) async {
    await tester.pumpWidget(const AuralockApp());

    expect(find.text('A4'), findsOneWidget);
    expect(find.text('Standard 440.0 Hz'), findsOneWidget);
    expect(find.text('LOCKED'), findsOneWidget);
    expect(find.text('440.1'), findsOneWidget);
    expect(find.text('Tuner'), findsOneWidget);
  });
}
