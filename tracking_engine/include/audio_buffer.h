#pragma once

#include "types.h"

#include <cstddef>
#include <vector>

namespace tunerlock::audio {

constexpr int kDefaultSampleRate = 48000;
constexpr std::size_t kDefaultFrameSize = 2048;

bool IsValidAudioFrame(const AudioFrame& frame);

AudioBuffer MakeAudioBuffer(
    const float* samples,
    std::size_t sample_count,
    int sample_rate);

AudioBuffer MakeAudioBuffer(const AudioFrame& frame);

float PeakAmplitude(const AudioBuffer& buffer);

float ComputeRms(const AudioBuffer& buffer);

void RemoveDcOffsetInPlace(AudioBuffer& buffer);

void NormalizeInPlace(AudioBuffer& buffer);

std::vector<float> CopySamplesForWindowing(
    const AudioBuffer& buffer,
    std::size_t window_size = kDefaultFrameSize);

}  // namespace tunerlock::audio
