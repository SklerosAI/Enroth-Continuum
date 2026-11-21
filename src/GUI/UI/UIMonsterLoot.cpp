#include "GUI/UI/UIMonsterLoot.h"

#include <memory>
#include <string>
#include <climits>
#include <utility>
#include <vector>

#include "Engine/AssetsManager.h"
#include "Engine/Engine.h"
#include "Engine/Graphics/Renderer/Renderer.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/ItemEnumFunctions.h"
#include "Engine/Party.h"
#include "Engine/Random/Random.h"
#include "Engine/Tables/ItemTable.h"
#include "Engine/Timer.h"
#include "Engine/Mods/PrivateMods.h"
#include "GUI/UI/UIPopup.h"
#include "Library/Random/MersenneTwisterRandomEngine.h"

namespace {
std::unique_ptr<GUIWindow_MonsterLoot> lootPopup;
bool capturingClickedLoot = false;
struct ClickedLootReward {
    ItemTreasureLevel level;
    RandomItemType type;
    Item item;
};
std::vector<ClickedLootReward> clickedRewards;

Item rollClickedItem(ItemTreasureLevel level, RandomItemType type) {
    MersenneTwisterRandomEngine seeded;
    seeded.seed(grng->random(INT_MAX) + 1);
    RandomEngine *previous = grng;
    auto generatedBefore = pParty->isArtifactGenerated;
    auto collectedBefore = pParty->isArtifactCollected;
    bool initializedBefore = pParty->artifactCollectionInitialized;
    uint8_t versionBefore = pParty->artifactCollectionVersion;
    Item item;
    grng = &seeded;
    try {
        pItemTable->generateItem(level, type, &item);
    } catch (...) {
        grng = previous;
        pParty->isArtifactGenerated = generatedBefore;
        pParty->isArtifactCollected = collectedBefore;
        pParty->artifactCollectionInitialized = initializedBefore;
        pParty->artifactCollectionVersion = versionBefore;
        throw;
    }
    grng = previous;
    pParty->isArtifactGenerated = generatedBefore;
    pParty->isArtifactCollected = collectedBefore;
    pParty->artifactCollectionInitialized = initializedBefore;
    pParty->artifactCollectionVersion = versionBefore;
    return item;
}

void openNextClickedReward();
}  // namespace

GUIWindow_MonsterLoot::GUIWindow_MonsterLoot(int actorId) :
    GUIWindow(WINDOW_MONSTER_LOOT, {0, 0}, render->GetRenderDimensions()), _actorId(actorId) {
    _timerWasPaused = gameTimer->isPaused();
    _offer = pActors[_actorId].rollRerollLoot();
    CreateButton("MonsterLoot_Reroll", {188, 295}, {112, 30}, BUTTON_TYPE_NORMAL, 0,
                 UIMSG_MONSTER_LOOT_Reroll, 0, INPUT_ACTION_INVALID, "Reroll monster loot");
    CreateButton("MonsterLoot_Take", {340, 295}, {112, 30}, BUTTON_TYPE_NORMAL, 0,
                 UIMSG_MONSTER_LOOT_Take, 0, INPUT_ACTION_INVALID, "Take this loot");
    CreateButton("MonsterLoot_Close", {484, 111}, {28, 28}, BUTTON_TYPE_NORMAL, 0,
                 UIMSG_Escape, 0, INPUT_ACTION_INVALID, "Leave loot");
    current_screen_type = SCREEN_MONSTER_LOOT;
    gameTimer->setPaused(true);
}

GUIWindow_MonsterLoot::GUIWindow_MonsterLoot(ItemTreasureLevel level, RandomItemType type, Item firstItem) :
    GUIWindow(WINDOW_MONSTER_LOOT, {0, 0}, render->GetRenderDimensions()), _actorId(-1),
    _treasureLevel(level), _treasureType(type) {
    _timerWasPaused = gameTimer->isPaused();
    _offer.item = firstItem;
    CreateButton("MonsterLoot_Reroll", {188, 295}, {112, 30}, BUTTON_TYPE_NORMAL, 0,
                 UIMSG_MONSTER_LOOT_Reroll, 0, INPUT_ACTION_INVALID, "Reroll random loot");
    CreateButton("MonsterLoot_Take", {340, 295}, {112, 30}, BUTTON_TYPE_NORMAL, 0,
                 UIMSG_MONSTER_LOOT_Take, 0, INPUT_ACTION_INVALID, "Take this loot");
    CreateButton("MonsterLoot_Close", {484, 111}, {28, 28}, BUTTON_TYPE_NORMAL, 0,
                 UIMSG_Escape, 0, INPUT_ACTION_INVALID, "Leave loot");
    current_screen_type = SCREEN_MONSTER_LOOT;
    gameTimer->setPaused(true);
}

void GUIWindow_MonsterLoot::Update() {
    DrawPopupWindow(115, 105, 410, 235);
    DrawText(assets->pFontArrus.get(), {145, 125}, colorTable.PaleCanary,
             isClickedLoot() ? "Found item" : pActors[_actorId].GetDisplayName(), frameRect);
    DrawText(assets->pFontLucida.get(), {145, 170}, colorTable.PaleCanary,
             _offer.gold ? "Gold: " + std::to_string(_offer.gold) : "No gold", frameRect);
    if (_offer.item.itemId == ITEM_NULL) {
        DrawText(assets->pFontLucida.get(), {145, 205}, colorTable.PaleCanary, "No item", frameRect);
    } else {
        DrawText(assets->pFontLucida.get(), {145, 205}, colorTable.PaleCanary,
                 pItemTable->items[_offer.item.itemId].unidentifiedName, frameRect);
        render->DrawQuad2D(assets->getIcon(_offer.item.GetIconName()), {435, 195});
    }
    render->FillRect({188, 295, 112, 30}, colorTable.Tacha);
    render->FillRect({190, 297, 108, 26}, colorTable.CarnabyTan);
    render->FillRect({192, 299, 104, 22}, colorTable.Cioccolato);
    DrawText(assets->pFontLucida.get(),
             {192 + assets->pFontLucida->AlignText_Center(104, "Reroll"), 301},
             colorTable.PaleCanary, "Reroll", frameRect);
    render->FillRect({340, 295, 112, 30}, colorTable.Tacha);
    render->FillRect({342, 297, 108, 26}, colorTable.CarnabyTan);
    render->FillRect({344, 299, 104, 22}, colorTable.Cioccolato);
    DrawText(assets->pFontLucida.get(),
             {344 + assets->pFontLucida->AlignText_Center(104, "Take"), 301},
             colorTable.PaleCanary, "Take", frameRect);
    DrawText(assets->pFontLucida.get(), {492, 116}, colorTable.PaleCanary, "X", frameRect);
}

void GUIWindow_MonsterLoot::reroll() {
    if (isClickedLoot())
        _offer.item = rollClickedItem(_treasureLevel, _treasureType);
    else
        _offer = pActors[_actorId].rollRerollLoot();
}

void openMonsterLootPopup(int actorId) {
    if (!lootPopup && actorId >= 0 && actorId < static_cast<int>(pActors.size()))
        lootPopup = std::make_unique<GUIWindow_MonsterLoot>(actorId);
}

void rerollMonsterLootPopup() {
    if (lootPopup && current_screen_type == SCREEN_MONSTER_LOOT)
        lootPopup->reroll();
}

void takeMonsterLootPopup() {
    if (!lootPopup || current_screen_type != SCREEN_MONSTER_LOOT)
        return;
    int actorId = lootPopup->actorId();
    MonsterLootOffer offer = lootPopup->offer();
    auto remaining = actorId < 0 ? std::move(clickedRewards) : std::vector<ClickedLootReward>();
    closeMonsterLootPopup();
    if (actorId < 0) {
        if (offer.item.itemId != ITEM_NULL) {
            if (isSpawnableArtifact(offer.item.itemId))
                pParty->isArtifactGenerated[offer.item.itemId] = true;
            pParty->setHoldingItem(offer.item);
        }
        clickedRewards = std::move(remaining);
        openNextClickedReward();
        return;
    }
    if (actorId >= 0 && actorId < static_cast<int>(pActors.size()))
        pActors[actorId].takeRerollLoot(offer);
}

void closeMonsterLootPopup() {
    if (lootPopup && lootPopup->isClickedLoot())
        clickedRewards.clear();
    bool timerWasPaused = lootPopup && lootPopup->timerWasPaused();
    lootPopup.reset();
    if (current_screen_type == SCREEN_MONSTER_LOOT) {
        current_screen_type = SCREEN_GAME;
        gameTimer->setPaused(timerWasPaused);
    }
}

bool monsterLootPopupOpen() {
    return lootPopup != nullptr;
}

namespace {
void openNextClickedReward() {
    if (clickedRewards.empty())
        return;
    ClickedLootReward reward = clickedRewards.front();
    clickedRewards.erase(clickedRewards.begin());
    lootPopup = std::make_unique<GUIWindow_MonsterLoot>(reward.level, reward.type, reward.item);
}
}

void beginClickedLootCapture() {
    capturingClickedLoot = engine && mm7::mods::monsterLootRerollEnabled(engine->gameTitle()) &&
                           engine->config->gameplay.MonsterLootRerollInGame.value() &&
                           current_screen_type == SCREEN_GAME && !lootPopup;
    clickedRewards.clear();
}

void finishClickedLootCapture() {
    capturingClickedLoot = false;
    openNextClickedReward();
}

bool captureClickedRandomLoot(ItemTreasureLevel level, RandomItemType type) {
    if (!capturingClickedLoot || level == ITEM_TREASURE_LEVEL_INVALID)
        return false;
    clickedRewards.push_back({level, type, rollClickedItem(level, type)});
    return true;
}
