#include "GUI/UI/UIMonsterLoot.h"

#include <cstdio>
#include <memory>
#include <optional>
#include <string>
#include <climits>
#include <utility>
#include <vector>

#include "Engine/AssetsManager.h"
#include "Engine/Engine.h"
#include "Engine/Graphics/Renderer/Renderer.h"
#include "Engine/Localization.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/MonsterEnumFunctions.h"
#include "Engine/Objects/Character.h"
#include "Engine/Objects/ItemEnumFunctions.h"
#include "Engine/Party.h"
#include "Engine/Random/Random.h"
#include "Engine/Tables/ItemTable.h"
#include "Engine/Timer.h"
#include "Engine/Mods/PrivateMods.h"
#include "GUI/GUIFont.h"
#include "GUI/UI/Houses/MM6House.h"
#include "GUI/UI/Houses/TownHall.h"
#include "GUI/UI/MM6FortunesTurn.h"
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
struct PendingEnchant {
    int characterIndex;
    int inventoryIndex;
    Mastery mastery;
};
std::optional<PendingEnchant> pendingEnchant;

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
    if (engine->gameTitle() == GameTitle::MM6)
        return mm6::rollFortuneItem(level, type);
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
    _offer = pActors[_actorId].rollRerollLoot();
    initializeControls();
}

GUIWindow_MonsterLoot::GUIWindow_MonsterLoot(ItemTreasureLevel level, RandomItemType type, Item firstItem) :
    GUIWindow_MonsterLoot(level, type, firstItem, Kind::Clicked) {}

GUIWindow_MonsterLoot::GUIWindow_MonsterLoot(ItemTreasureLevel level, RandomItemType type, Item firstItem,
                                            Kind kind) :
    GUIWindow(WINDOW_MONSTER_LOOT, {0, 0}, render->GetRenderDimensions()), _actorId(-1),
    _treasureLevel(level), _treasureType(type), _kind(kind) {
    _offer.item = firstItem;
    initializeControls();
}

GUIWindow_MonsterLoot::GUIWindow_MonsterLoot(HouseId bountyHall) :
    GUIWindow(WINDOW_MONSTER_LOOT, {0, 0}, render->GetRenderDimensions()), _actorId(-1),
    _kind(Kind::Bounty), _bountyHall(bountyHall) {
    initializeControls();
}

GUIWindow_MonsterLoot::GUIWindow_MonsterLoot(int characterIndex, int inventoryIndex, Mastery mastery) :
    GUIWindow(WINDOW_MONSTER_LOOT, {0, 0}, render->GetRenderDimensions()), _actorId(-1),
    _kind(Kind::Enchantment), _characterIndex(characterIndex), _inventoryIndex(inventoryIndex), _mastery(mastery) {
    InventoryEntry entry = pParty->pCharacters[characterIndex].inventory.entry(inventoryIndex);
    _offer.item = *entry.get();
    _enchantedItemId = _offer.item.itemId;
    initializeControls();
}

void GUIWindow_MonsterLoot::initializeControls() {
    _timerWasPaused = gameTimer->isPaused();
    _previousScreen = current_screen_type;
    CreateButton("MonsterLoot_Reroll", {180, 365}, {112, 30}, BUTTON_TYPE_NORMAL, 0,
                 UIMSG_MONSTER_LOOT_Reroll, 0, INPUT_ACTION_INVALID, "Reroll fortune");
    CreateButton("MonsterLoot_Take", {345, 365}, {112, 30}, BUTTON_TYPE_NORMAL, 0,
                 UIMSG_MONSTER_LOOT_Take, 0, INPUT_ACTION_INVALID,
                 _kind == Kind::Bounty || _kind == Kind::Enchantment ? "Keep this result" : "Take this loot");
    CreateButton("MonsterLoot_Close", {500, 46}, {28, 28}, BUTTON_TYPE_NORMAL, 0,
                 UIMSG_Escape, 0, INPUT_ACTION_INVALID,
                 _kind == Kind::Monster || _kind == Kind::Clicked ? "Leave loot" : "Keep result and close");
    current_screen_type = SCREEN_MONSTER_LOOT;
    gameTimer->setPaused(true);
}

void GUIWindow_MonsterLoot::Update() {
    if (engine->gameTitle() == GameTitle::MM6)
        render->EndTextNew(); // MM6's status line and house text lie under the frame, and queued text draws after quads.
    DrawPopupWindow(95, 35, 450, 380);
    if (_kind == Kind::Bounty) {
        MonsterId monster = pParty->monster_id_for_hunting[_bountyHall];
        if (monster != MONSTER_INVALID) {
            DrawText(assets->pFontArrus.get(), {125, 58}, colorTable.PaleCanary, "Bounty hunt", frameRect);
            DrawText(assets->pFontArrus.get(), {125, 135}, colorTable.PaleCanary,
                     pMonsterStats->infos[monster].name, frameRect);
            DrawText(assets->pFontLucida.get(), {125, 181}, colorTable.PaleCanary,
                     fmt::format("Level: {}", pMonsterStats->infos[monster].level), frameRect);
            DrawText(assets->pFontLucida.get(), {125, 214}, colorTable.PaleCanary,
                     fmt::format("Reward: {} gold", 100 * pMonsterStats->infos[monster].level), frameRect);
        }
    } else if (_offer.item.itemId != ITEM_NULL) {
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
    if (_kind != Kind::Bounty) {
        std::string title = _kind == Kind::Monster ? pActors[_actorId].GetDisplayName() :
                            _kind == Kind::Ore ? "Ore smith result" :
                            _kind == Kind::Enchantment ? "Enchanted weapon" : "Found item";
        DrawText(assets->pFontArrus.get(), {125, 58}, colorTable.PaleCanary, title, frameRect);
        if (_kind == Kind::Monster)
            DrawText(assets->pFontLucida.get(), {125, 97}, colorTable.PaleCanary,
                     _offer.gold ? "Gold: " + std::to_string(_offer.gold) : "No gold", frameRect);
        if (_offer.item.itemId == ITEM_NULL)
            DrawText(assets->pFontLucida.get(), {125, 135}, colorTable.PaleCanary, "No item", frameRect);
        else
            DrawText(assets->pFontArrus.get(), {125, 135}, colorTable.PaleCanary,
                     _offer.item.GetIdentifiedName(), frameRect);
    }
    render->FillRect({180, 365, 112, 30}, colorTable.Tacha);
    render->FillRect({182, 367, 108, 26}, colorTable.CarnabyTan);
    render->FillRect({184, 369, 104, 22}, colorTable.Cioccolato);
    DrawText(assets->pFontLucida.get(),
             {184 + assets->pFontLucida->AlignText_Center(104, "Reroll"), 371},
             colorTable.PaleCanary, "Reroll", frameRect);
    render->FillRect({345, 365, 112, 30}, colorTable.Tacha);
    render->FillRect({347, 367, 108, 26}, colorTable.CarnabyTan);
    render->FillRect({349, 369, 104, 22}, colorTable.Cioccolato);
    const std::string acceptLabel = _kind == Kind::Bounty || _kind == Kind::Enchantment ? "Keep" : "Take";
    DrawText(assets->pFontLucida.get(),
             {349 + assets->pFontLucida->AlignText_Center(104, acceptLabel), 371},
             colorTable.PaleCanary, acceptLabel, frameRect);
    DrawText(assets->pFontLucida.get(), {510, 55}, colorTable.PaleCanary, "X", frameRect);
}

void GUIWindow_MonsterLoot::reroll() {
    switch (_kind) {
        case Kind::Clicked:
        case Kind::Ore:
            _offer.item = rollClickedItem(_treasureLevel, _treasureType);
            break;
        case Kind::Monster:
            _offer = pActors[_actorId].rollRerollLoot();
            break;
        case Kind::Bounty:
            if (engine->gameTitle() == GameTitle::MM6) {
                if (GUIWindow_MM6House *house = mm6HouseWindow())
                    house->rerollBounty();
                break;
            }
            pParty->monster_id_for_hunting[_bountyHall] = GUIWindow_TownHall::randomMonsterForHunting(_bountyHall);
            if (auto *hall = dynamic_cast<GUIWindow_TownHall *>(window_SpeakInHouse.get()))
                hall->refreshBountyPreview();
            break;
        case Kind::Enchantment: {
            InventoryEntry entry = pParty->pCharacters[_characterIndex].inventory.entry(_inventoryIndex);
            if (!entry || entry->itemId != _enchantedItemId)
                break;
            ItemEnchantment enchantment = rollWeaponSpellEnchantment(entry->type(), _mastery);
            if (enchantment != ITEM_ENCHANTMENT_NULL) {
                entry->specialEnchantment = enchantment;
                _offer.item = *entry.get();
            }
            break;
        }
    }
}

ItemEnchantment rollWeaponSpellEnchantment(ItemType itemType, Mastery mastery) {
    if (!isWeapon(itemType) || (mastery != MASTERY_MASTER && mastery != MASTERY_GRANDMASTER))
        return ITEM_ENCHANTMENT_NULL;
    int totalWeight = 0;
    for (ItemEnchantment enchantment : pItemTable->specialEnchantments.indices()) {
        const auto &data = pItemTable->specialEnchantments[enchantment];
        if (data.description.empty() || data.enchantmentLevel == 3 ||
            (mastery == MASTERY_MASTER && data.enchantmentLevel != 0))
            continue;
        totalWeight += data.chanceByItemType[itemType];
    }
    if (!totalWeight)
        return ITEM_ENCHANTMENT_NULL;
    int choice = grng->random(totalWeight);
    for (ItemEnchantment enchantment : pItemTable->specialEnchantments.indices()) {
        const auto &data = pItemTable->specialEnchantments[enchantment];
        if (data.description.empty() || data.enchantmentLevel == 3 ||
            (mastery == MASTERY_MASTER && data.enchantmentLevel != 0))
            continue;
        choice -= data.chanceByItemType[itemType];
        if (choice < 0)
            return enchantment;
    }
    return ITEM_ENCHANTMENT_NULL;
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
    GUIWindow_MonsterLoot::Kind kind = lootPopup->kind();
    MonsterLootOffer offer = lootPopup->offer();
    auto remaining = kind == GUIWindow_MonsterLoot::Kind::Clicked ?
                     std::move(clickedRewards) : std::vector<ClickedLootReward>();
    closeMonsterLootPopup();
    if (kind == GUIWindow_MonsterLoot::Kind::Ore || kind == GUIWindow_MonsterLoot::Kind::Bounty ||
        kind == GUIWindow_MonsterLoot::Kind::Enchantment)
        return;
    if (kind == GUIWindow_MonsterLoot::Kind::Clicked) {
        if (offer.item.itemId != ITEM_NULL && engine->gameTitle() == GameTitle::MM6) {
            mm6::takeFortuneItem(offer.item);
        } else if (offer.item.itemId != ITEM_NULL) {
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
    if (lootPopup && lootPopup->kind() == GUIWindow_MonsterLoot::Kind::Ore) {
        if (isSpawnableArtifact(lootPopup->offer().item.itemId))
            pParty->isArtifactGenerated[lootPopup->offer().item.itemId] = true;
        pParty->setHoldingItem(lootPopup->offer().item);
    }
    bool timerWasPaused = lootPopup && lootPopup->timerWasPaused();
    ScreenType previousScreen = lootPopup ? lootPopup->previousScreen() : SCREEN_GAME;
    lootPopup.reset();
    if (current_screen_type == SCREEN_MONSTER_LOOT) {
        current_screen_type = previousScreen;
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
    capturingClickedLoot = fortunesTurnSwitchedOn() && current_screen_type == SCREEN_GAME && !lootPopup;
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

bool fortunesTurnSwitchedOn() {
    if (engine && engine->gameTitle() == GameTitle::MM6)
        return mm6::fortunesTurnOn();
    return engine && mm7::mods::monsterLootRerollEnabled(engine->gameTitle()) &&
           engine->config->gameplay.MonsterLootRerollInGame.value();
}

bool fortuneRerollActive() {
    return fortunesTurnSwitchedOn() && !lootPopup;
}

void openOreRerollPopup(ItemTreasureLevel level, RandomItemType type) {
    if (fortuneRerollActive() && (current_screen_type == SCREEN_HOUSE ||
                                 current_screen_type == SCREEN_NPC_DIALOGUE) &&
        level != ITEM_TREASURE_LEVEL_INVALID)
        lootPopup = std::make_unique<GUIWindow_MonsterLoot>(level, type, rollClickedItem(level, type),
                                                            GUIWindow_MonsterLoot::Kind::Ore);
}

void openBountyRerollPopup(HouseId hall) {
    if (fortuneRerollActive() && current_screen_type == SCREEN_HOUSE &&
        pParty->monster_id_for_hunting[hall] != MONSTER_INVALID &&
        !pParty->monster_for_hunting_killed[hall] &&
        pParty->PartyTimes.bountyHuntNextGenTime[hall] >= pParty->GetPlayingTime())
        lootPopup = std::make_unique<GUIWindow_MonsterLoot>(hall);
}

void openWeaponEnchantRerollPopup(int characterIndex, int inventoryIndex, Mastery mastery) {
    if (fortuneRerollActive() && current_screen_type == SCREEN_GAME)
        lootPopup = std::make_unique<GUIWindow_MonsterLoot>(characterIndex, inventoryIndex, mastery);
}

void queueWeaponEnchantReroll(int characterIndex, int inventoryIndex, Mastery mastery) {
    if (!engine || !mm7::mods::monsterLootRerollEnabled(engine->gameTitle()) ||
        !engine->config->gameplay.MonsterLootRerollInGame.value())
        return;
    pendingEnchant = PendingEnchant{characterIndex, inventoryIndex, mastery};
    if (current_screen_type == SCREEN_GAME)
        openPendingWeaponEnchantRerollPopup();
}

void openPendingWeaponEnchantRerollPopup() {
    if (!pendingEnchant || current_screen_type != SCREEN_GAME)
        return;
    PendingEnchant pending = *std::exchange(pendingEnchant, std::nullopt);
    InventoryEntry entry = pParty->pCharacters[pending.characterIndex].inventory.entry(pending.inventoryIndex);
    if (entry && isWeapon(entry->type()) && entry->specialEnchantment != ITEM_ENCHANTMENT_NULL)
        openWeaponEnchantRerollPopup(pending.characterIndex, pending.inventoryIndex, pending.mastery);
}
