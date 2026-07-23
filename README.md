# TunerLock

TunerLock은 Flutter UI와 C++ DSP Tracking Engine을 Flutter FFI로 연결하는 실시간 튜너 앱입니다.

## 구조

```text
tracking_engine/
├── include/
│   ├── tracking_engine.h
│   └── types.h
├── src/
│   ├── api/
│   │   └── tracking_engine_api.cpp
│   ├── audio/
│   │   ├── audio_buffer.cpp
│   │   └── window.cpp
│   ├── fft/
│   │   └── fft.cpp
│   ├── pitch/
│   │   ├── yin.cpp
│   │   └── mpm.cpp
│   ├── feature/
│   │   ├── feature_extractor.cpp
│   │   ├── spectral.cpp
│   │   └── harmonic.cpp
│   ├── tracking/
│   │   ├── attack_detector.cpp
│   │   ├── confidence.cpp
│   │   ├── target_manager.cpp
│   │   └── tracking_lock.cpp
│   └── filter/
│       └── kalman.cpp
├── tests/
└── CMakeLists.txt
```

## 아키텍처

```text
Flutter UI
        ↓
Dart Service
tracking_engine_service.dart
        ↓
Flutter FFI
        ↓
C++ Tracking Engine
        ├── Audio Buffer
        ├── FFT / Windowing
        ├── YIN Pitch Detection
        ├── Feature Extraction
        ├── Attack Detector
        ├── Tracking Lock
        ├── Confidence Engine
        └── Kalman Filter
        ↓
PitchResult 반환
        ↓
Flutter UI 업데이트
```

## 데이터 흐름

```text
마이크 입력 o
↓
PCM Audio Buffer o
↓
Windowing o
↓
FFT o
↓
YIN으로 기본 피치 검출o
↓
Feature 추출 o
↓
Attack 발생 여부 확인ㅇ
↓
Tracking Lock 생성 또는 유지ㅇ
↓
Confidence 계산ㅇ
↓
Kalman Filter로 피치 안정화ㅇ
↓
PitchResult 반환ㅇ
↓
UI에서 사인파 위치, Hz, cents, 상태 표시
```

## 상태 흐름

```text
IDLE
  ↓ Attack 감지
LOCKING
  ↓ 신뢰도 높음
LOCKED
  ↓ 신뢰도 하락
RECOVERING
  ↓ 복구 실패
LOST
  ↓ 새 Attack 대기
IDLE
```

## 모듈

| 모듈 | 역할 |
| --- | --- |
| `tracking_engine.h` | 외부에서 호출하는 공개 API |
| `types.h` | 공통 타입, 상태, 결과 구조체 정의 |
| `tracking_engine_api.cpp` | 전체 엔진 흐름 연결 |
| `audio_buffer.cpp` | PCM 오디오 버퍼 처리 |
| `window.cpp` | Hann window 등 윈도우 처리 |
| `fft.cpp` | FFT/DFT 및 magnitude 계산 |
| `yin.cpp` | 기본 피치 검출 |
| `mpm.cpp` | 추후 비교/복구용 보조 피치 검출 |
| `kalman.cpp` | 피치 흔들림 안정화 |
| `feature_extractor.cpp` | RMS, centroid, flux 등 특징 통합 |
| `spectral.cpp` | 스펙트럼 특징 계산 |
| `harmonic.cpp` | 배음 관련 특징 계산 |
| `attack_detector.cpp` | 새 음 시작점 감지 |
| `tracking_lock.cpp` | 추적 상태 관리 |
| `target_manager.cpp` | 추적 대상 후보 관리 |
| `confidence.cpp` | 신뢰도 계산 |

## 개발 워크플로우

1. `tracking_engine.h` / `types.h` 정의
2. `CMakeLists.txt` 빌드 확인
3. `yin.cpp`에서 440Hz 감지
4. `kalman.cpp`로 pitch 안정화
5. `feature_extractor.cpp` 구현
6. `attack_detector.cpp` 구현
7. `tracking_lock.cpp` 구현
8. `confidence.cpp` 구현
9. `tracking_engine_api.cpp`에서 전체 연결
10. Flutter FFI 연결

## Feature

일반 튜너:

```text
Audio → Pitch → UI
```

TunerLock:

```text
Audio → Pitch + Feature → Attack → Lock → Confidence → Kalman → UI
```

## feature

rms 에너지의 크기

peak_amplitude 진폭의 크기

yin_pitch_hz 소리의 기음

yin_detected 신뢰도

fft_peak_frequency_hz 에너지가 가장 강한 주파수

fft_peak_magnitude 에너지가 강한 주파수의 에너지 크기

spectral_centroid_hz 스펙트럼의 중심 - 소리의 밝기

harmonic_weighted_sum 배음의 크기 합

harmonic_normalized_score 배음 정규화

harmonic_energy_ratio 배음의 비율

harmonic_count 배음의 개수

## attack

Frame N
│
├─ Peak/RMS 급상승?
├─ Spectral Centroid 상승?
└─ Harmonic Ratio 불안정 또는 낮음?
          │
          ├─ 아니오 → 기존 상태 유지
          │
          └─ 예 → ATTACK_CANDIDATE 생성
                    │
                    ▼
          Frame N+1 ~ N+4 확인
                    │
                    ├─ YIN 피치가 검출되는가?
                    ├─ 피치가 프레임 간 안정적인가?
                    ├─ Harmonic Ratio가 상승하는가?
                    └─ RMS가 유효 수준을 유지하는가?
                              │
                 ┌────────────┴────────────┐
                 ▼                         ▼
             조건 충족                  조건 실패
                 │                         │
                 ▼                         ▼
          ATTACK_CONFIRMED           후보 폐기(NOISE)
                 │
                 ▼
          Target 후보 생성
                 │
                 ├─ 기존 Lock 피치와 가까움
                 │      → 기존 Tracking Lock 유지/갱신
                 │
                 └─ 기존 Lock 피치와 멂
                        → 새 타깃 점수 평가
                        → 충분히 강할 때만 Lock 전환
