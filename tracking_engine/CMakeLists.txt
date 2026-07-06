cmake_minimum_required(VERSION 3.16)

project(tunerlock_tracking_engine LANGUAGES CXX)

enable_testing()

add_library(tunerlock_tracking_engine STATIC
  src/common/types.cpp
  src/api/tracking_engine_api.cpp
  src/audio/audio_buffer.cpp
  src/audio/window.cpp
  src/fft/fft.cpp
  src/pitch/yin.cpp
  src/pitch/mpm.cpp
  src/tracking/attack_detector.cpp
  src/tracking/tracking_lock.cpp
  src/tracking/target_manager.cpp
  src/tracking/confidence.cpp
  src/feature/feature_extractor.cpp
  src/feature/harmonic.cpp
  src/feature/spectral.cpp
  src/filter/kalman.cpp
)

target_include_directories(tunerlock_tracking_engine PUBLIC include)
target_compile_features(tunerlock_tracking_engine PUBLIC cxx_std_17)

add_executable(audio_buffer_test
  tests/audio_buffer_test.cpp
)

target_link_libraries(audio_buffer_test PRIVATE tunerlock_tracking_engine)
target_compile_features(audio_buffer_test PRIVATE cxx_std_17)

add_test(NAME audio_buffer_test COMMAND audio_buffer_test)
