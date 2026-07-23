cmake_minimum_required(VERSION 3.16)

project(tunerlock_tracking_engine LANGUAGES CXX)

enable_testing()

add_library(tunerlock_tracking_engine STATIC
  src/common/types.cpp
  src/api/tracking_engine_api.cpp
  src/audio/audio_buffer.cpp
  src/audio/wav_reader.cpp
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

add_executable(harmonic_test
  tests/harmonic_test.cpp
)

target_link_libraries(harmonic_test PRIVATE tunerlock_tracking_engine)
target_compile_features(harmonic_test PRIVATE cxx_std_17)

add_test(NAME harmonic_test COMMAND harmonic_test)

add_executable(feature_extractor_test
  tests/feature_extractor_test.cpp
)

target_link_libraries(feature_extractor_test PRIVATE tunerlock_tracking_engine)
target_compile_features(feature_extractor_test PRIVATE cxx_std_17)

add_test(NAME feature_extractor_test COMMAND feature_extractor_test)

add_executable(attack_detector_test
  tests/attack_detector_test.cpp
)

target_link_libraries(attack_detector_test PRIVATE tunerlock_tracking_engine)
target_compile_features(attack_detector_test PRIVATE cxx_std_17)

add_test(NAME attack_detector_test COMMAND attack_detector_test)

add_executable(target_tracking_test
  tests/target_tracking_test.cpp
)

target_link_libraries(target_tracking_test PRIVATE tunerlock_tracking_engine)
target_compile_features(target_tracking_test PRIVATE cxx_std_17)

add_test(NAME target_tracking_test COMMAND target_tracking_test)

add_executable(confidence_test
  tests/confidence_test.cpp
)

target_link_libraries(confidence_test PRIVATE tunerlock_tracking_engine)
target_compile_features(confidence_test PRIVATE cxx_std_17)

add_test(NAME confidence_test COMMAND confidence_test)

add_executable(kalman_test
  tests/kalman_test.cpp
)

target_link_libraries(kalman_test PRIVATE tunerlock_tracking_engine)
target_compile_features(kalman_test PRIVATE cxx_std_17)

add_test(NAME kalman_test COMMAND kalman_test)

add_executable(yin_test
  tests/yin_test.cpp
)

target_link_libraries(yin_test PRIVATE tunerlock_tracking_engine)
target_compile_features(yin_test PRIVATE cxx_std_17)

add_test(NAME yin_test COMMAND yin_test)

add_executable(debug_trace_test
  tests/debug_trace_test.cpp
)

target_link_libraries(debug_trace_test PRIVATE tunerlock_tracking_engine)
target_compile_features(debug_trace_test PRIVATE cxx_std_17)

add_test(NAME debug_trace_test COMMAND debug_trace_test)

add_executable(wav_reader_test
  tests/wav_reader_test.cpp
)

target_link_libraries(wav_reader_test PRIVATE tunerlock_tracking_engine)
target_compile_features(wav_reader_test PRIVATE cxx_std_17)

add_test(NAME wav_reader_test COMMAND wav_reader_test)
set_tests_properties(wav_reader_test PROPERTIES
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/.."
)

add_executable(fft_vector_plot
  tools/fft_vector_plot.cpp
)

target_link_libraries(fft_vector_plot PRIVATE tunerlock_tracking_engine)
target_compile_features(fft_vector_plot PRIVATE cxx_std_17)

add_executable(engine_wav_analyzer
  tools/engine_wav_analyzer.cpp
)

target_link_libraries(engine_wav_analyzer PRIVATE tunerlock_tracking_engine)
target_compile_features(engine_wav_analyzer PRIVATE cxx_std_17)
