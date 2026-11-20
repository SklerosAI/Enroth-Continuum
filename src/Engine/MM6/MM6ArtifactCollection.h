#pragma once

#include <array>
#include <cstdint>
#include <utility>

#include "Engine/Engine.h"
#include "Engine/MM6/MM6ArtifactCollectionSection.h"
#include "Engine/MM6/MM6ChestSeedAndOwnership.h"
#include "Engine/Mods/PrivateMods.h"
#include "Engine/Party.h"

namespace mm6 {

inline ArtifactCollection &artifactCollection() {
    static ArtifactCollection owner;
    return owner;
}

/**
 * Starts the ledger with the artifacts the party carries, in the backpacks, on the paperdolls and on the cursor. An
 * artifact left in a chest is marked when the party takes it.
 */
inline void initializeArtifactCollection() {
    ArtifactCollection &owner = artifactCollection();
    if (owner.initialized)
        return;
    owner.collected = scanPartyOwnedChestArtifacts(GameTitle::MM6, *pParty).value();
    owner.initialized = true;
}

/**
 * @return                              The collected flags the random artifact rolls skip instead of the generated ones,
 *                                      or `nullptr` when the All Artifacts mod is off and MM6.exe's rule applies.
 */
inline const std::array<std::uint8_t, 30> *collectedArtifactExclusion() {
    if (!engine || !pParty || !mods::allArtifactsEnabled(engine->gameTitle()))
        return nullptr;
    initializeArtifactCollection();
    return &artifactCollection().collected;
}

/**
 * Marks an artifact collected when the All Artifacts mod is on. Called when an item reaches the cursor or a backpack.
 *
 * @param itemId                        Item the party just got.
 */
inline void recordArtifactAcquisition(ItemId itemId) {
    if (!collectedArtifactExclusion())
        return;
    const int id = std::to_underlying(itemId);
    if (id >= 400 && id <= 429)
        artifactCollection().collected[id - 400] = 1;
}

} // namespace mm6
