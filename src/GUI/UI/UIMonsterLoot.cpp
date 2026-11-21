#include "GUI/UI/UIMonsterLoot.h"

#include <cstdio>
#include <memory>
#include <string>
#include <climits>
#include <utility>
#include <vector>

#include "Engine/AssetsManager.h"
#include "Engine/Engine.h"
#include "Engine/Graphics/Renderer/Renderer.h"
#include "Engine/Localization.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/ItemEnumFunctions.h"
#include "Engine/Party.h"
#include "Engine/Random/Random.h"
#include "Engine/Tables/ItemTable.h"
#include "Engine/Timer.h"
#include "Engine/Mods/PrivateMods.h"
#include "GUI/GUIFont.h"
#include "GUI/UI/UIPopup.h"
#include "Library/Random/MersenneTwisterRandomEngine.h"
#include "Utility/String/Format.h"

namespace {
std::unique_ptr<GUIWindow_MonsterLoot> lootPopup;
bool capturingClickedLoot = false;
struct ClickedLootReward {
    ItemTreasureLevel level;
    RandomItemType type;
    Item item;
};
std::vector<ClickedLootReward> clickedRewards;

std::vector<std::string> itemStatLines(const Item &item) {
    std::vector<std::string> lines;
    switch (item.type()) {
        case ITEM_TYPE_SINGLE_HANDED:
        case ITEM_TYPE_TWO_HANDED:
        case ITEM_TYPE_BOW: {
            const std::string &attack = localization->str(item.type() == ITEM_TYPE_BOW ? LSTR_SHOOT : LSTR_ATTACK);
            std::string damage = fmt::format("{}: {}d{}", localization->str(LSTR_DAMAGE),
                                             item.GetDamageDice(), item.GetDamageRoll());
            if (item.GetDamageMod())
                damage += fmt::format("+{}", item.GetDamageMod());
            lines.push_back(fmt::format("{}: +{}   {}", attack, item.GetDamageMod(), damage));
            break;
        }
        case ITEM_TYPE_ARMOUR:
        case ITEM_TYPE_SHIELD:
        case ITEM_TYPE_HELMET:
        case ITEM_TYPE_BELT:
        case ITEM_TYPE_CLOAK:
        case ITEM_TYPE_GAUNTLETS:
        case ITEM_TYPE_BOOTS:
        case ITEM_TYPE_RING:
        case ITEM_TYPE_AMULET:
            if (item.GetDamageDice() || item.GetDamageMod())
                lines.push_back(fmt::format("{}: +{}", localization->str(LSTR_ARMOR),
                                            item.GetDamageDice() + item.GetDamageMod()));
            break;
        default:
            break;
    }
    if (item.isPotion() && item.potionPower)
        lines.push_back(fmt::format("{}: {}", localization->str(LSTR_POWER), item.potionPower));
    else if (item.isReagent())
        lines.push_back(fmt::format("{}: {}", localization->str(LSTR_POWER), item.GetReagentPower()));
    else if (item.standardEnchantment)
        lines.push_back(fmt::format("{}: {} +{}", localization->str(LSTR_SPECIAL),
                                    pItemTable->standardEnchantments[*item.standardEnchantment].attributeName,
                                    item.standardEnchantmentStrength));
    else if (item.specialEnchantment != ITEM_ENCHANTMENT_NULL)
        lines.push_back(fmt::format("{}: {}", localization->str(LSTR_SPECIAL),
                                    pItemTable->specialEnchantments[item.specialEnchantment].description));
    else if (item.isWand())
        lines.push_back(fmt::sprintf(localization->str(LSTR_FMT_S_U_OUT_OF_U),
                                      localization->str(LSTR_CHARGES), item.numCharges, item.maxCharges));
    lines.push_back(fmt::format("{}: {}", localization->str(LSTR_VALUE), item.GetValue()));
    return lines;
}

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
    CreateButton("MonsterLoot_Reroll", {180, 365}, {112, 30}, BUTTON_TYPE_NORMAL, 0,
                 UIMSG_MONSTER_LOOT_Reroll, 0, INPUT_ACTION_INVALID, "Reroll monster loot");
    CreateButton("MonsterLoot_Take", {345, 365}, {112, 30}, BUTTON_TYPE_NORMAL, 0,
                 UIMSG_MONSTER_LOOT_Take, 0, INPUT_ACTION_INVALID, "Take this loot");
    CreateButton("MonsterLoot_Close", {500, 46}, {28, 28}, BUTTON_TYPE_NORMAL, 0,
                 UIMSG_Escape, 0, INPUT_ACTION_INVALID, "Leave loot");
    current_screen_type = SCREEN_MONSTER_LOOT;
    gameTimer->setPaused(true);
}

GUIWindow_MonsterLoot::GUIWindow_MonsterLoot(ItemTreasureLevel level, RandomItemType type, Item firstItem) :
    GUIWindow(WINDOW_MONSTER_LOOT, {0, 0}, render->GetRenderDimensions()), _actorId(-1),
    _treasureLevel(level), _treasureType(type) {
    _timerWasPaused = gameTimer->isPaused();
    _offer.item = firstItem;
    CreateButton("MonsterLoot_Reroll", {180, 365}, {112, 30}, BUTTON_TYPE_NORMAL, 0,
                 UIMSG_MONSTER_LOOT_Reroll, 0, INPUT_ACTION_INVALID, "Reroll random loot");
    CreateButton("MonsterLoot_Take", {345, 365}, {112, 30}, BUTTON_TYPE_NORMAL, 0,
                 UIMSG_MONSTER_LOOT_Take, 0, INPUT_ACTION_INVALID, "Take this loot");
    CreateButton("MonsterLoot_Close", {500, 46}, {28, 28}, BUTTON_TYPE_NORMAL, 0,
                 UIMSG_Escape, 0, INPUT_ACTION_INVALID, "Leave loot");
    current_screen_type = SCREEN_MONSTER_LOOT;
    gameTimer->setPaused(true);
}

void GUIWindow_MonsterLoot::Update() {
    DrawPopupWindow(95, 35, 450, 380);
    if (_offer.item.itemId != ITEM_NULL) {
        const Item &item = _offer.item;
        const ItemData &data = pItemTable->items[item.itemId];
        render->DrawQuad2D(assets->getIcon(item.GetIconName()), {440, 140});

        const Recti detailsRect(125, 178, 290, 166);
        int y = 0;
        for (const std::string &line : itemStatLines(item)) {
            DrawText(assets->pFontComic.get(), {1, y}, colorTable.PaleCanary,
                     assets->pFontComic->WrapText(line, detailsRect.w, 1), detailsRect, 344);
            y += assets->pFontComic->CalcTextHeight(line, detailsRect.w, 1) + 4;
        }
        if (!data.description.empty()) {
            y += 7;
            DrawText(assets->pFontSmallnum.get(), {1, y}, colorTable.White,
                     assets->pFontSmallnum->WrapText(data.description, detailsRect.w, 1), detailsRect, 344);
        }
        // Flush the bounded details before drawing the unbounded header and controls.
        render->DrawTwodVerts();
    }
    DrawText(assets->pFontArrus.get(), {125, 58}, colorTable.PaleCanary,
             isClickedLoot() ? "Found item" : pActors[_actorId].GetDisplayName(), frameRect);
    DrawText(assets->pFontLucida.get(), {125, 97}, colorTable.PaleCanary,
             _offer.gold ? "Gold: " + std::to_string(_offer.gold) : "No gold", frameRect);
    if (_offer.item.itemId == ITEM_NULL)
        DrawText(assets->pFontLucida.get(), {125, 135}, colorTable.PaleCanary, "No item", frameRect);
    else
        DrawText(assets->pFontArrus.get(), {125, 135}, colorTable.PaleCanary,
                 _offer.item.GetIdentifiedName(), frameRect);
    render->FillRect({180, 365, 112, 30}, colorTable.Tacha);
    render->FillRect({182, 367, 108, 26}, colorTable.CarnabyTan);
    render->FillRect({184, 369, 104, 22}, colorTable.Cioccolato);
    DrawText(assets->pFontLucida.get(),
             {184 + assets->pFontLucida->AlignText_Center(104, "Reroll"), 371},
             colorTable.PaleCanary, "Reroll", frameRect);
    render->FillRect({345, 365, 112, 30}, colorTable.Tacha);
    render->FillRect({347, 367, 108, 26}, colorTable.CarnabyTan);
    render->FillRect({349, 369, 104, 22}, colorTable.Cioccolato);
    DrawText(assets->pFontLucida.get(),
             {349 + assets->pFontLucida->AlignText_Center(104, "Take"), 371},
             colorTable.PaleCanary, "Take", frameRect);
    DrawText(assets->pFontLucida.get(), {510, 55}, colorTable.PaleCanary, "X", frameRect);
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
