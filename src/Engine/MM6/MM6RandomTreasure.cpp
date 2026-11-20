#include "Engine/MM6/MM6RandomTreasure.h"

#include <optional>
#include <stdexcept>
#include <utility>

#include "Engine/Engine.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/MM6/MM6ArtifactCollection.h"
#include "Engine/MM6/MM6ChestCallerAdapter.h"
#include "Engine/MM6/MM6ChestDirectArtifactPicker.h"
#include "Engine/MM6/MM6ChestSeedAndOwnership.h"
#include "Engine/MM6/MM6ChestTitleState.h"
#include "Engine/MM6/MM6ItemGenerator.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Objects/SpriteObject.h"
#include "Engine/Party.h"
#include "Engine/SpawnPoint.h"
#include "Engine/Tables/ItemTable.h"
#include "Engine/Tables/ObjectTable.h"

namespace mm6 {

static ItemKindRoll itemKindRoll() {
    return engine->config->gameplay.FixItemGeneration.value() ? ITEM_KIND_ROLL_FIXED : ITEM_KIND_ROLL_VANILLA;
}

Item generateTitleItem(ItemTreasureLevel treasureLevel, RandomItemType requestedKind) {
    StagedTitleItem staged = stageTitleItem(chestTitleState(), *pItemTable, treasureLevel, requestedKind, itemKindRoll(),
                                           collectedArtifactExclusion());
    chestTitleState() = staged.nextState;
    return staged.item;
}

StagedTitleItem stageRandomTreasure(const ChestTitleState &state, MapTreasureLevel mapTreasureLevel,
                                    ItemTreasureLevel markerLevel) {
    if (!state.ready)
        throw std::runtime_error("MM6 random treasure needs a restored gameplay LCG");
    const auto range = chestTreasureLevelRangeForMarker(-std::to_underlying(markerLevel), mapTreasureLevel);
    if (!range)
        throw std::runtime_error("MM6 random treasure level is outside the original table");

    StagedTitleItem result;
    result.nextState = state;
    ChestTitleState &next = result.nextState;
    const int typeRoll = nextChestRandom15(next.lcgState) % 100;
    const int low = std::to_underlying(range->front());
    const int high = std::to_underlying(range->back());
    const int level = low + nextChestRandom15(next.lcgState) % (high - low + 1);

    if (level == 7) {
        const auto owned = scanPartyOwnedChestArtifacts(GameTitle::MM6, *pParty);
        const auto pick = pickDirectChestArtifactSafePolicy(GameTitle::MM6, owned.value(), next.generatedArtifactFlags,
                                                            next.lcgState, collectedArtifactExclusion());
        if (!pick)
            throw std::runtime_error("MM6 random treasure artifact flags are outside the original range");
        next.lcgState = pick->nextRngState;
        next.generatedArtifactFlags = pick->generatedArtifactFlags;
        result.item.itemId = static_cast<ItemId>(pick->itemId);
    } else if (typeRoll >= 60) {
        const auto kind = static_cast<RandomItemType>(std::to_underlying(RANDOM_ITEM_WEAPON) + nextChestRandom15(next.lcgState) % 24);
        return stageTitleItem(next, *pItemTable, markerLevel, kind, itemKindRoll(), collectedArtifactExclusion());
    } else if (typeRoll >= 20) {
        const auto gold = pickChestGold(GameTitle::MM6, markerLevel, next.lcgState);
        if (!gold)
            throw std::runtime_error("MM6 random treasure gold level is outside the original table");
        next.lcgState = gold->nextRngState;
        result.item.itemId = gold->itemId;
        result.item.goldAmount = gold->amount;
        result.item.SetIdentified();
    }
    return result;
}

void spawnRandomTreasure(MapTreasureLevel mapTreasureLevel, const SpawnPoint &spawn) {
    const StagedTitleItem staged = stageRandomTreasure(chestTitleState(), mapTreasureLevel, spawn.treasureLevel);
    chestTitleState() = staged.nextState;
    if (staged.item.itemId == ITEM_NULL)
        return;

    SpriteObject sprite;
    sprite.containing_item = staged.item;
    sprite.spriteId = pItemTable->items[staged.item.itemId].spriteId;
    sprite.uObjectDescID = pObjectTable->ObjectIDByItemID(sprite.spriteId);
    sprite.vPosition = spawn.position;
    sprite.uSectorID = pIndoor->GetSector(spawn.position);
    sprite.Create(0, 0, 0, 0);
}

} // namespace mm6
