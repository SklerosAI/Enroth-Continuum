#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <utility>

#include "Engine/MM6/MM6ChestItemFinalizer.h"
#include "Engine/MM6/MM6ItemGenerator.h"
#include "Engine/Tables/ItemTable.h"

namespace mm6 {

// The outer MM6 chest generator supplies one saved 0..99 probability to the
// first attempt and draws a new one for each additional attempt. This selector
// does not consume a draw or publish any chest state. Tier 7 bypasses it.
enum class ChestRewardSource { Empty, Gold, AnyItem, DirectArtifact };

inline std::optional<ChestRewardSource> chestRewardSource(GameTitle title,
                                                          ItemTreasureLevel tier,
                                                          int probability) {
    if (title != GameTitle::MM6 || probability < 0 || probability >= 100)
        return std::nullopt;
    const int level = std::to_underlying(tier);
    if (level == 7)
        return ChestRewardSource::DirectArtifact;
    if (level < 1 || level > 6)
        return std::nullopt;
    if (probability < 20)
        return ChestRewardSource::Empty;
    if (probability < 60)
        return ChestRewardSource::Gold;
    return ChestRewardSource::AnyItem;
}

// Gold comes from the original six MM6 tier bands, not from rnditems.txt.
struct ChestGoldPick {
    ItemId itemId = ITEM_NULL;
    int amount = 0;
    std::uint32_t nextRngState = 0;
};

inline std::optional<ChestGoldPick> pickChestGold(GameTitle title,
                                                  ItemTreasureLevel tier,
                                                  std::uint32_t rngState) {
    if (title != GameTitle::MM6)
        return std::nullopt;
    static constexpr std::array<int, 6> base = {50, 100, 200, 500, 1000, 2000};
    static constexpr std::array<int, 6> span = {51, 101, 301, 501, 1001, 3001};
    static constexpr std::array<ItemId, 6> ids = {
        ITEM_GOLD_SMALL, ITEM_GOLD_SMALL, ITEM_GOLD_MEDIUM,
        ITEM_GOLD_MEDIUM, ITEM_GOLD_LARGE, ITEM_GOLD_LARGE};
    const int level = std::to_underlying(tier);
    if (level < 1 || level > 6)
        return std::nullopt;
    const int index = level - 1;
    return ChestGoldPick{ids[index], base[index] + static_cast<int>(nextChestRandom15(rngState) % span[index]),
                         rngState};
}

// A direct tier-seven artifact is a separate party-ownership picker. The
// original candidate loop's extra ID is unresolved; no selector is exposed.
inline bool needsDirectChestArtifactPicker(GameTitle title, ItemTreasureLevel tier) {
    return title == GameTitle::MM6 && tier == ITEM_TREASURE_LEVEL_7;
}

inline std::optional<ItemId> mm6ChestItemId(GameTitle title, int rawId) {
    if (title != GameTitle::MM6 || rawId < 1 || rawId > 580)
        return std::nullopt;
    return static_cast<ItemId>(rawId);
}

inline std::optional<ChestFinalizedItem> stageChestAnyFromItemTable(
    GameTitle title, ItemTreasureLevel tier, const ItemTable &table,
    const std::array<std::uint8_t, 30> &generatedArtifactFlags,
    std::uint32_t rngState) {
    return generateItem(title, table, tier, RANDOM_ITEM_ANY, generatedArtifactFlags, rngState);
}

}  // namespace mm6
