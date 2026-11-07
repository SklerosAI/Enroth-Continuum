#pragma once

#include <cstdint>
#include <span>
#include <vector>

#include "MM6V4SaveFields.h"
#include "MM6V5SpriteRuntimeFields.h"

namespace mm6 {

// Detached EC6 v5 candidate: v4's complete 23 owners plus full-list SPRT.
struct V5SaveFields {
    V4SaveFields v4;
    SpriteRuntimeSaveFields sprites;
};

std::vector<std::uint8_t> encodeV5Save(const V5SaveFields &fields, GameTitle title);
V5SaveFields decodeV5Save(std::span<const std::uint8_t> bytes, GameTitle title);

} // namespace mm6
