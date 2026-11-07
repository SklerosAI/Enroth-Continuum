#include "Engine/Evt/MM6CircusPrizeEvent.h"

#include <utility>

#include "Engine/Objects/Character.h"

namespace mm6 {

int circusPrizeScore(std::span<const Character> characters) {
    int score = 0;
    for (const Character &character : characters) {
        for (InventoryConstEntry entry : character.inventory.entries()) {
            switch (std::to_underlying(entry->itemId)) {
            case 470: score += 1; break;
            case 471: score += 3; break;
            case 477: score += 5; break;
            default: break;
            }
        }
    }
    return score;
}

} // namespace mm6
