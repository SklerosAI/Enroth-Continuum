#include "GUI/UI/MM6FortunesTurn.h"

#include <algorithm>
#include <string>
#include <string_view>
#include <utility>

#include "Engine/Engine.h"
#include "Engine/Evt/MM6GiveItemEvent.h"
#include "Engine/Graphics/Renderer/Renderer.h"
#include "Engine/MM6/MM6ArtifactCollection.h"
#include "Engine/MM6/MM6ChestTitleState.h"
#include "Engine/MM6/MM6ItemGenerator.h"
#include "Engine/Mods/PrivateMods.h"
#include "Engine/Random/Random.h"
#include "Engine/Tables/ItemTable.h"

#include "GUI/GUIButton.h"
#include "GUI/GUIWindow.h"
#include "GUI/MM6HudLayout.h"

namespace mm6 {

static constexpr Color kToggleFrame = Color(63, 56, 56); // The dark grey of the portrait bars' slots.
static constexpr Color kToggleWell = Color(28, 24, 24);
static constexpr Color kToggleGold = Color(194, 169, 102); // The gold of the bars' caps.
static constexpr std::string_view kToggleButtonId = "MonsterLoot_InGameToggle";

bool fortunesTurnOn() {
    return engine && mods::monsterLootRerollEnabled(engine->gameTitle()) &&
           engine->config->gameplay.MM6MonsterLootRerollInGame.value();
}

MonsterLootOffer rollCorpseFortune(const Actor &actor) {
    MonsterLootOffer offer;
    offer.gold = grng->randomDice(actor.monsterInfo.goldDiceRolls, actor.monsterInfo.goldDiceSides);
    if (grng->random(100) < actor.monsterInfo.treasureDropChance && actor.monsterInfo.treasureLevel != ITEM_TREASURE_LEVEL_INVALID)
        offer.item = rollFortuneItem(actor.monsterInfo.treasureLevel, actor.monsterInfo.treasureType);
    return offer;
}

Item rollFortuneItem(ItemTreasureLevel level, RandomItemType type) {
    const ItemKindRoll kindRoll = engine->config->gameplay.FixItemGeneration.value() ? ITEM_KIND_ROLL_FIXED : ITEM_KIND_ROLL_VANILLA;
    const StagedTitleItem staged = stageTitleItem(chestTitleState(), *pItemTable, level, type, kindRoll, collectedArtifactExclusion());
    chestTitleState().lcgState = staged.nextState.lcgState;
    return staged.item;
}

void claimFortuneItem(ItemId itemId) {
    const int id = std::to_underlying(itemId);
    if (id >= 400 && id <= 429)
        chestTitleState().generatedArtifactFlags[id - 400] = 1;
}

void takeFortuneItem(const Item &item) {
    holdEventItem(item);
    claimFortuneItem(item.itemId);
}

void updateFortunesTurnToggle() {
    const bool shown = mods::monsterLootRerollEnabled(engine->gameTitle());
    const auto button = std::ranges::find(pPrimaryWindow->vButtons, kToggleButtonId, &GUIButton::id);
    if (shown && button == pPrimaryWindow->vButtons.end()) {
        pPrimaryWindow->CreateButton(std::string(kToggleButtonId), mm6HudFortunesTurnButtonRect.topLeft(), mm6HudFortunesTurnButtonRect.size(),
                                     BUTTON_TYPE_NORMAL, 0, UIMSG_MONSTER_LOOT_Toggle, 0, INPUT_ACTION_INVALID, "Toggle Fortune's Turn during play");
    } else if (!shown && button != pPrimaryWindow->vButtons.end()) {
        (*button)->Release();
    }
    if (!shown || current_screen_type != SCREEN_GAME)
        return;

    const Recti box = mm6HudFortunesTurnCheckboxRect;
    render->FillRect(box, kToggleFrame);
    render->FillRect({box.x + 1, box.y + 1, box.w - 2, box.h - 2}, kToggleWell);
    if (engine->config->gameplay.MM6MonsterLootRerollInGame.value()) {
        for (int i = 0; i < box.w - 5; ++i) {
            render->FillRect({box.x + 2 + i, box.y + 2 + i, 2, 2}, kToggleGold);
            render->FillRect({box.x + box.w - 4 - i, box.y + 2 + i, 2, 2}, kToggleGold);
        }
    }

    const Recti dice = mm6HudFortunesTurnDiceRect;
    render->FillRect(dice, kToggleFrame);
    render->FillRect({dice.x + 1, dice.y + 1, dice.w - 2, dice.h - 2}, kToggleGold);
    for (int pip : {2, 5, 8})
        render->FillRect({dice.x + pip, dice.y + pip, 2, 2}, kToggleWell);
}

} // namespace mm6
