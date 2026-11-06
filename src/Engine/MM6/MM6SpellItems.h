#pragma once

#include <stdexcept>
#include <utility>

#include "Engine/Objects/ItemEnums.h"
#include "Engine/Spells/SpellEnums.h"

namespace mm6 {

// The retained MM6 item table has one scroll and one book for each regular
// spell, in spell-ID order. These ranges are unrelated to the MM7 ItemId enum.
inline constexpr int kFirstSpellScrollItem = 200;
inline constexpr int kLastSpellScrollItem = 298;
inline constexpr int kFirstSpellbookItem = 300;
inline constexpr int kLastSpellbookItem = 398;

inline constexpr bool isSpellScrollId(ItemId item) {
    return std::to_underlying(item) >= kFirstSpellScrollItem &&
           std::to_underlying(item) <= kLastSpellScrollItem;
}

inline constexpr bool isSpellbookId(ItemId item) {
    return std::to_underlying(item) >= kFirstSpellbookItem &&
           std::to_underlying(item) <= kLastSpellbookItem;
}

inline SpellId spellForScroll(ItemId item) {
    if (!isSpellScrollId(item))
        throw std::out_of_range("Unsupported MM6 spell scroll item ID");
    return static_cast<SpellId>(std::to_underlying(item) - kFirstSpellScrollItem + 1);
}

inline SpellId spellForSpellbook(ItemId item) {
    if (!isSpellbookId(item))
        throw std::out_of_range("Unsupported MM6 spellbook item ID");
    return static_cast<SpellId>(std::to_underlying(item) - kFirstSpellbookItem + 1);
}

}  // namespace mm6
