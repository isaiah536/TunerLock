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

add_executable(window_test
  tests/window_test.cpp
)

target_link_libraries(window_test PRIVATE tunerlock_tracking_engine)
target_compile_features(window_test PRIVATE cxx_std_17)

add_test(NAME window_test COMMAND window_test)

add_executable(fft_test
  tests/fft_test.cpp
)

target_link_libraries(fft_test PRIVATE tunerlock_tracking_engine)
target_compile_features(fft_test PRIVATE cxx_std_17)

add_test(NAME fft_test COMMAND fft_test)

add_executable(yin_test
  tests/yin_test.cpp
)

target_link_libraries(yin_test PRIVATE tunerlock_tracking_engine)
target_compile_features(yin_test PRIVATE cxx_std_17)

add_test(NAME yin_test COMMAND yin_test)
