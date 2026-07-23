#include "target_manager.h"
#include "tracking_lock.h"

#include <cassert>
#include <cmath>

namespace {

tunerlock::tracking::TargetObservation Observation(
    double pitch_hz,
    double harmonic_ratio,
    double rms,
    bool attack,
    double locked_energy) {
  tunerlock::tracking::TargetObservation observation;
  observation.features.yin_pitch_hz = pitch_hz;
  observation.features.yin_detected = pitch_hz > 0.0;
  observation.features.harmonic_energy_ratio = harmonic_ratio;
  observation.features.harmonic_normalized_score = harmonic_ratio;
  observation.features.rms = rms;
  observation.attack_confirmed = attack;
  observation.locked_target_energy_ratio = locked_energy;
  return observation;
}

tunerlock::tracking::TargetDecision Acquire(
    tunerlock::tracking::TargetManager& manager,
    double pitch_hz) {
  manager.Process(Observation(pitch_hz, 0.30, 0.10, false, 0.0));
  manager.Process(Observation(pitch_hz + 0.1, 0.31, 0.10, false, 0.0));
  return manager.Process(
      Observation(pitch_hz - 0.1, 0.32, 0.10, false, 0.0));
}

void KeepsLockWhenAnotherSoundIsAdded() {
  tunerlock::tracking::TargetManager manager;
  const auto acquired = Acquire(manager, 261.63);
  assert(acquired.type ==
      tunerlock::tracking::TargetDecisionType::Acquired);

  manager.Process(Observation(293.66, 0.35, 0.12, true, 0.20));
  manager.Process(Observation(293.70, 0.36, 0.12, false, 0.18));
  const auto rejected =
      manager.Process(Observation(293.62, 0.37, 0.11, false, 0.17));

  assert(rejected.type ==
      tunerlock::tracking::TargetDecisionType::ChallengerRejected);
  assert(std::fabs(rejected.target_frequency_hz - 261.63) < 1.0);
}

void SwitchesWhenPlayerChangesNote() {
  tunerlock::tracking::TargetManager manager;
  Acquire(manager, 261.63);

  manager.Process(Observation(293.66, 0.35, 0.12, true, 0.02));
  manager.Process(Observation(293.70, 0.36, 0.12, false, 0.01));
  const auto switched =
      manager.Process(Observation(293.62, 0.37, 0.11, false, 0.01));

  assert(switched.type ==
      tunerlock::tracking::TargetDecisionType::Switched);
  assert(std::fabs(switched.target_frequency_hz - 293.66) < 1.0);
}

void TrackingLockFollowsDecisions() {
  tunerlock::tracking::TrackingLock lock;

  const auto acquired = lock.Process({
      tunerlock::tracking::TargetDecisionType::Acquired,
      440.0,
      0.9,
      true,
  });
  assert(acquired.locked);
  assert(acquired.target_changed);

  const auto rejected = lock.Process({
      tunerlock::tracking::TargetDecisionType::ChallengerRejected,
      440.0,
      0.85,
      true,
  });
  assert(rejected.locked);
  assert(!rejected.target_changed);

  const auto switched = lock.Process({
      tunerlock::tracking::TargetDecisionType::Switched,
      493.88,
      0.88,
      true,
  });
  assert(switched.locked);
  assert(switched.target_changed);
  assert(std::fabs(switched.frequency_hz - 493.88) < 0.01);
}

void StringsAllowLegatoTransitionWithoutAttack() {
  tunerlock::tracking::TargetManager manager(
      tunerlock::tracking::InstrumentProfile::Strings);

  for (int i = 0; i < 4; ++i) {
    manager.Process(Observation(261.63, 0.20, 0.10, false, 0.0));
  }
  assert(manager.has_target());

  tunerlock::tracking::TargetDecision decision;
  for (int i = 0; i < 5; ++i) {
    decision =
        manager.Process(Observation(293.66, 0.22, 0.10, false, 0.01));
  }

  assert(decision.type ==
      tunerlock::tracking::TargetDecisionType::Switched);
  assert(std::fabs(decision.target_frequency_hz - 293.66) < 1.0);
}

void BrassRequiresStrongHarmonics() {
  tunerlock::tracking::TargetManager manager(
      tunerlock::tracking::InstrumentProfile::Brass);

  for (int i = 0; i < 6; ++i) {
    const auto decision =
        manager.Process(Observation(233.08, 0.05, 0.15, true, 0.0));
    assert(decision.type !=
        tunerlock::tracking::TargetDecisionType::Acquired);
  }
  assert(!manager.has_target());

  tunerlock::tracking::TargetDecision decision;
  for (int i = 0; i < 4; ++i) {
    decision =
        manager.Process(Observation(233.08, 0.18, 0.15, true, 0.0));
  }
  assert(decision.type ==
      tunerlock::tracking::TargetDecisionType::Acquired);
}

void MissingPitchEntersRecoveringBeforeLost() {
  tunerlock::tracking::TargetManager manager;
  tunerlock::tracking::TrackingLock lock;
  lock.Process(Acquire(manager, 440.0));

  const auto recovering_decision =
      manager.Process(Observation(0.0, 0.0, 0.0, false, 0.0));
  assert(recovering_decision.type ==
      tunerlock::tracking::TargetDecisionType::Recovering);
  const auto recovering = lock.Process(recovering_decision);
  assert(recovering.state == tunerlock::TrackingState::Recovering);
  assert(!recovering.locked);

  tunerlock::tracking::TargetDecision decision;
  for (int frame = 0; frame < 4; ++frame) {
    decision = manager.Process(
        Observation(0.0, 0.0, 0.0, false, 0.0));
  }
  assert(decision.type ==
      tunerlock::tracking::TargetDecisionType::Lost);
  assert(lock.Process(decision).state == tunerlock::TrackingState::Lost);
}

}  // namespace

int main() {
  KeepsLockWhenAnotherSoundIsAdded();
  SwitchesWhenPlayerChangesNote();
  TrackingLockFollowsDecisions();
  StringsAllowLegatoTransitionWithoutAttack();
  BrassRequiresStrongHarmonics();
  MissingPitchEntersRecoveringBeforeLost();
  return 0;
}
