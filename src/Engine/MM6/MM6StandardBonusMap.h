#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

#include "Engine/Objects/CharacterEnums.h"

namespace mm6 {

// The original MM6 item's Bonus word is one-based into the first 14 MM6
// stats. The final three resistance names are electricity, cold and poison
// in MM6; the shared runtime represents those as air, water and body.
inline constexpr std::array<Attribute, 14> kStandardBonusAttributes = {
    ATTRIBUTE_MIGHT, ATTRIBUTE_INTELLIGENCE, ATTRIBUTE_PERSONALITY, ATTRIBUTE_ENDURANCE,
    ATTRIBUTE_ACCURACY, ATTRIBUTE_SPEED, ATTRIBUTE_LUCK, ATTRIBUTE_HEALTH, ATTRIBUTE_MANA,
    ATTRIBUTE_AC_BONUS, ATTRIBUTE_RESIST_FIRE, ATTRIBUTE_RESIST_AIR, ATTRIBUTE_RESIST_WATER,
    ATTRIBUTE_RESIST_BODY
};

constexpr std::optional<Attribute> attributeForStandardBonus(std::uint32_t oneBasedIndex) {
    if (oneBasedIndex < 1 || oneBasedIndex > kStandardBonusAttributes.size())
        return std::nullopt;
    return kStandardBonusAttributes[oneBasedIndex - 1];
}

constexpr std::optional<std::uint32_t> standardBonusForAttribute(Attribute attribute) {
    for (std::size_t index = 0; index < kStandardBonusAttributes.size(); ++index)
        if (kStandardBonusAttributes[index] == attribute)
            return static_cast<std::uint32_t>(index + 1);
    return std::nullopt;
}

} // namespace mm6
