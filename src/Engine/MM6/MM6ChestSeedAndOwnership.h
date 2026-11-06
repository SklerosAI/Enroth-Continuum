#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <utility>

#include "Engine/MM6/MM6ChestTitleState.h"
#include "Engine/Party.h"

namespace mm6 {

// The retained chest trace proves the LCG step, but not the original new-game
// seed source. A caller must explicitly supply a deterministic seed; an
// unverified implicit source may never make the title owner ready.
enum class NewGameChestSeedSource {
    Unverified,
    ExplicitDeterministic
};

inline bool initializeNewGameChestTitleState(GameTitle title, ChestTitleState *owner,
                                             std::uint32_t explicitSeed,
                                             NewGameChestSeedSource source) {
    if (source != NewGameChestSeedSource::ExplicitDeterministic)
        return false;
    return restoreChestTitleState(title, owner, explicitSeed, {});
}

// The personal-fork direct-artifact policy uses actual character-owned Item
// records, including equipment and the item held on the cursor. It never scans
// raw Party memory or the MM7 artifact flag range. Inventory::entries visits
// each occupied record once, regardless of its grid footprint.
inline std::optional<std::array<std::uint8_t, 30>> scanPartyOwnedChestArtifacts(
    GameTitle title, const Party &party) {
    if (title != GameTitle::MM6)
        return std::nullopt;

    std::array<std::uint8_t, 30> owned = {};
    auto mark = [&owned](ItemId itemId) {
        const int id = std::to_underlying(itemId);
        if (id >= 400 && id <= 429)
            owned[id - 400] = 1;
    };
    for (const Character &character : party.pCharacters)
        for (InventoryConstEntry entry : character.inventory.entries())
            mark(entry->itemId);
    mark(party.pPickedItem.itemId);
    return owned;
}

}  // namespace mm6
