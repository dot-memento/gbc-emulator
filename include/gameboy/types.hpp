#pragma once

#include <cstdint>
#include <limits>

namespace GbcEmulator {

using Byte = uint8_t;
using Word = uint16_t;
using TCycleCount = unsigned long long;
using MCycleCount = unsigned long long;

static constexpr TCycleCount TCycle_never = std::numeric_limits<TCycleCount>::max();

}  // namespace GbcEmulator