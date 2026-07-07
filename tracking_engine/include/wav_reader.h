#pragma once

#include "types.h"

#include <string>

namespace tunerlock::audio {

struct WavReadResult {
  AudioBuffer buffer;
  int channel_count = 0;
  int bits_per_sample = 0;
  int audio_format = 0;
  bool ok = false;
  std::string error;
};

WavReadResult ReadWavFile(const std::string& path);

}  // namespace tunerlock::audio
