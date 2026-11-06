#pragma once

#include <array>
#include <string_view>

#include "Engine/Objects/ItemEnums.h"

namespace mm6 {

enum class PotionCombinationKind { Product, OpaqueMarker };

struct PotionCombinationOutcome {
    PotionCombinationKind kind = PotionCombinationKind::OpaqueMarker;
    ItemId product = ITEM_NULL;
    std::array<char, 2> marker = {};

    bool operator==(const PotionCombinationOutcome &) const = default;
};

// The retained MM6 useitems.txt axis is 29 herb/bottle IDs, 160–188. Numeric
// cells identify a candidate product. Marker effects are deliberately opaque.
class PotionCombinations {
 public:
    static constexpr int FIRST_ID = 160;
    static constexpr int LAST_ID = 188;
    static constexpr int COUNT = LAST_ID - FIRST_ID + 1;

    static PotionCombinations parse(std::string_view text);
    PotionCombinationOutcome lookup(ItemId first, ItemId second) const;
    bool loaded() const { return _loaded; }

 private:
    std::array<PotionCombinationOutcome, COUNT * COUNT> _cells = {};
    bool _loaded = false;
};

} // namespace mm6
