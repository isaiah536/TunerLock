#pragma once

#include <cstddef>
#include <vector>

namespace tunerlock::audio {

std::vector<float> HannWindow(std::size_t size);

void ApplyWindowInPlace(
    std::vector<float>& samples,
    const std::vector<float>& window);

}  // namespace tunerlock::audio
