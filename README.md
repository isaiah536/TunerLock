# TunerLock

## 구조

tracking_engine/
├── include/
│   └── tracking_engine.h
│   └── types.h
├── src/
│   ├── api/
│   │   └── tracking_engine_api.cpp
│   │
│   ├── audio/
│   │   ├── audio_buffer.cpp
│   │   └── window.cpp
│   │
│   ├── fft/
│   │   └── fft.cpp
│   │
│   ├── pitch/
│   │   ├── yin.cpp
│   │   └── mpm.cpp
│   │
│   ├── feature/
│   │   ├── feature_extractor.cpp
│   │   ├── spectral.cpp
│   │   └── harmonic.cpp
│   │
│   ├── tracking/
│   │   ├── attack_detector.cpp
│   │   ├── confidence.cpp
│   │   ├── target_manager.cpp
│   │   └── tracking_lock.cpp
│   │
│   └── filter/
│       └── kalman.cpp
│
├── tests/
└── CMakeLists.txt

### 아키텍쳐

Flutter UI
(Liquid Glass 화면)
        │
        ▼
Dart Service
tracking_engine_service.dart
        │
        ▼
Flutter FFI
        │
        ▼
C++ Tracking Engine
        │
        ├─ Audio Buffer
        ├─ FFT / Windowing
        ├─ YIN Pitch Detection
        ├─ Feature Extraction
        ├─ Attack Detector
        ├─ Tracking Lock
        ├─ Confidence Engine
        └─ Kalman Filter
        │
        ▼
PitchResult 반환
        │
        ▼
Flutter UI 업데이트

#### 데이터 흐름

마이크 입력
↓
PCM Audio Buffer
↓
Windowing
↓
FFT
↓
YIN으로 기본 피치 검출
↓
Feature 추출
↓
Attack 발생 여부 확인
↓
Tracking Lock 생성 또는 유지
↓
Confidence 계산
↓
Kalman Filter로 피치 안정화
↓
PitchResult 반환
↓
UI에서 사인파 위치, Hz, cents, 상태 표시

##### 상태

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

###### 모듈

| 모듈                      | 역할
| ----------------------------------------------------
| `tracking_engine.h`       | 외부에서 호출하는 공개 API
| `tracking_engine_api.cpp` | 전체 엔진 흐름 연결
| `yin.cpp`                 | 기본 피치 검출
| `kalman.cpp`              | 피치 흔들림 안정화
| `feature_extractor.cpp`   | RMS, centroid, flux 등 특징 통합
| `spectral.cpp`            | 스펙트럼 특징 계산
| `harmonic.cpp`            | 배음 관련 특징 계산
| `attack_detector.cpp`     | 새 음 시작점 감지
| `tracking_lock.cpp`       | 추적 상태 관리
| `target_manager.cpp`      | 추적 대상 후보 관리
| `confidence.cpp`          | 신뢰도 계산
| `mpm.cpp`                 | 추후 비교/복구용 보조 피치 검출

워크플로우

1. tracking_engine.h /types.h 정의
2. CMakeLists.txt 빌드 확인
3. yin.cpp에서 440Hz 감지
4. kalman.cpp로 pitch 안정화
5. feature_extractor.cpp 구현
6. attack_detector.cpp 구현
7. tracking_lock.cpp 구현
8. confidence.cpp 구현
9. tracking_engine_api.cpp에서 전체 연결 10. Flutter FFI 연결

--feature

일반 튜너:
Audio → Pitch → UI

TuneLock:
Audio → Pitch + Feature → Attack → Lock → Confidence → Kalman → UI
