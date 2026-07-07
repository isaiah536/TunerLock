#include "wav_reader.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

namespace tunerlock::audio {
namespace {

constexpr int kPcmFormat = 1;
constexpr int kFloatFormat = 3;

std::uint16_t ReadU16(const unsigned char* data) {
  return static_cast<std::uint16_t>(data[0]) |
         static_cast<std::uint16_t>(data[1] << 8);
}

std::uint32_t ReadU32(const unsigned char* data) {
  return static_cast<std::uint32_t>(data[0]) |
         (static_cast<std::uint32_t>(data[1]) << 8) |
         (static_cast<std::uint32_t>(data[2]) << 16) |
         (static_cast<std::uint32_t>(data[3]) << 24);
}

std::int32_t ReadSignedInt(
    const unsigned char* data,
    int bytes_per_sample) {
  std::int32_t value = 0;
  for (int i = 0; i < bytes_per_sample; ++i) {
    value |= static_cast<std::int32_t>(data[i]) << (8 * i);
  }

  const int shift = 32 - bytes_per_sample * 8;
  return (value << shift) >> shift;
}

float DecodePcmSample(
    const unsigned char* data,
    int bits_per_sample,
    int audio_format) {
  if (audio_format == kFloatFormat && bits_per_sample == 32) {
    float value = 0.0F;
    std::memcpy(&value, data, sizeof(float));
    return std::max(-1.0F, std::min(1.0F, value));
  }

  const int bytes_per_sample = bits_per_sample / 8;
  const std::int32_t value = ReadSignedInt(data, bytes_per_sample);
  const float scale =
      static_cast<float>(std::int64_t{1} << (bits_per_sample - 1));
  return std::max(-1.0F, std::min(1.0F, static_cast<float>(value) / scale));
}

bool IdEquals(const unsigned char* data, const char* id) {
  return std::memcmp(data, id, 4) == 0;
}

WavReadResult Error(std::string message) {
  WavReadResult result;
  result.error = std::move(message);
  return result;
}

}  // namespace

WavReadResult ReadWavFile(const std::string& path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    return Error("failed_to_open_file");
  }

  unsigned char riff_header[12] = {};
  file.read(reinterpret_cast<char*>(riff_header), sizeof(riff_header));
  if (file.gcount() != sizeof(riff_header) ||
      !IdEquals(riff_header, "RIFF") ||
      !IdEquals(riff_header + 8, "WAVE")) {
    return Error("invalid_riff_wave_header");
  }

  int audio_format = 0;
  int channel_count = 0;
  int sample_rate = 0;
  int bits_per_sample = 0;
  std::vector<unsigned char> data_chunk;

  while (file) {
    unsigned char chunk_header[8] = {};
    file.read(reinterpret_cast<char*>(chunk_header), sizeof(chunk_header));
    if (file.gcount() == 0) {
      break;
    }
    if (file.gcount() != sizeof(chunk_header)) {
      return Error("truncated_chunk_header");
    }

    const std::uint32_t chunk_size = ReadU32(chunk_header + 4);
    if (IdEquals(chunk_header, "fmt ")) {
      std::vector<unsigned char> fmt(chunk_size);
      file.read(reinterpret_cast<char*>(fmt.data()), fmt.size());
      if (static_cast<std::uint32_t>(file.gcount()) != chunk_size ||
          fmt.size() < 16) {
        return Error("invalid_fmt_chunk");
      }

      audio_format = ReadU16(fmt.data());
      channel_count = ReadU16(fmt.data() + 2);
      sample_rate = static_cast<int>(ReadU32(fmt.data() + 4));
      bits_per_sample = ReadU16(fmt.data() + 14);
    } else if (IdEquals(chunk_header, "data")) {
      data_chunk.resize(chunk_size);
      file.read(reinterpret_cast<char*>(data_chunk.data()), data_chunk.size());
      if (static_cast<std::uint32_t>(file.gcount()) != chunk_size) {
        return Error("invalid_data_chunk");
      }
    } else {
      file.seekg(chunk_size, std::ios::cur);
      if (!file) {
        return Error("invalid_chunk_skip");
      }
    }

    if ((chunk_size % 2) == 1) {
      file.seekg(1, std::ios::cur);
    }
  }

  if (audio_format != kPcmFormat && audio_format != kFloatFormat) {
    return Error("unsupported_audio_format");
  }
  if (channel_count <= 0 || sample_rate <= 0 || bits_per_sample <= 0) {
    return Error("missing_or_invalid_format");
  }
  if (data_chunk.empty()) {
    return Error("missing_data_chunk");
  }
  if ((bits_per_sample % 8) != 0) {
    return Error("unsupported_bit_depth");
  }

  const int bytes_per_sample = bits_per_sample / 8;
  const int bytes_per_frame = bytes_per_sample * channel_count;
  if (bytes_per_frame <= 0 ||
      (data_chunk.size() % static_cast<std::size_t>(bytes_per_frame)) != 0) {
    return Error("invalid_data_size");
  }

  const std::size_t frame_count = data_chunk.size() / bytes_per_frame;
  std::vector<float> mono_samples(frame_count, 0.0F);
  for (std::size_t frame = 0; frame < frame_count; ++frame) {
    double sum = 0.0;
    for (int channel = 0; channel < channel_count; ++channel) {
      const std::size_t offset =
          frame * bytes_per_frame + channel * bytes_per_sample;
      sum += DecodePcmSample(
          data_chunk.data() + offset,
          bits_per_sample,
          audio_format);
    }
    mono_samples[frame] = static_cast<float>(sum / channel_count);
  }

  WavReadResult result;
  result.buffer.samples = std::move(mono_samples);
  result.buffer.sample_rate = sample_rate;
  result.channel_count = channel_count;
  result.bits_per_sample = bits_per_sample;
  result.audio_format = audio_format;
  result.ok = true;
  return result;
}

}  // namespace tunerlock::audio
