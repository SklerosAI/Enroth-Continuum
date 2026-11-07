#pragma once

#include <cstdint>
#include <span>
#include <vector>

#include "MM6EventRuntimeSaveFields.h"
#include "MM6V3SaveFields.h"

namespace mm6 {

// Detached EC6 v4 candidate. EVRT is a title-local runtime owner independent
// of the native DDM/DLV persistent bytes retained in MAPS. V3 stays strict.
struct V4SaveFields {
    V3SaveFields v3;
    EventRuntimeSaveFields eventRuntime;
};

// Exact 23-section container: v3's 22 required sections plus EVRT.
std::vector<std::uint8_t> encodeV4Save(const V4SaveFields &fields, GameTitle title);
V4SaveFields decodeV4Save(std::span<const std::uint8_t> bytes, GameTitle title);

} // namespace mm6
