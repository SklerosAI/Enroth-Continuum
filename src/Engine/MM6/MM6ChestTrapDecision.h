#pragma once

#include <array>
#include <cstdint>
#include <optional>

#include "Engine/MM6/MM6ChestRecordTransaction.h"

namespace mm6 {

struct ChestTrapDecision {
    bool disarmed = false;
    int variant = -1;  // 0..3 when triggered; no sprite/effect is emitted here.
    int spriteId = 0;  // Original variant order: fire, electricity, cold, poison in MM6 terms.
    int drawsConsumed = 0;
    std::uint32_t nextLcgState = 0;
};

// The retained MM6 chest-entry PE shows placement first, then active-party
// and map checks, then this disarm comparison. The original score function is
// separate; caller supplies its nonnegative result and the map difficulty.
// This stage returns no changed chest, damage or UI state, see
// `stageChestOpenOwner` for the chest and `MM6ChestTrap.h` for the effects.
inline std::optional<ChestTrapDecision> stageChestTrapDecision(
    GameTitle title, const ChestRecord &placedRecord, bool activeCharacter,
    int disarmScore, int mapDifficulty, std::uint32_t lcgState) {
    if (title != GameTitle::MM6 || !activeCharacter || disarmScore < 0 ||
        mapDifficulty < 0 || mapDifficulty > 255 ||
        (readChest16(placedRecord, 2) & 3u) != 3u)
        return std::nullopt;

    ChestTrapDecision result;
    result.nextLcgState = lcgState;
    // The original jumps straight to the triggered branch at score zero.
    if (disarmScore > 0) {
        const int roll = nextChestRandom15(result.nextLcgState) % 10;
        ++result.drawsConsumed;
        if (disarmScore + roll > 5 * mapDifficulty) {
            result.disarmed = true;
            return result;
        }
    }
    result.variant = nextChestRandom15(result.nextLcgState) % 4;
    constexpr std::array<int, 4> sprites = {811, 813, 812, 814};
    result.spriteId = sprites[result.variant];
    ++result.drawsConsumed;
    return result;
}

}  // namespace mm6
