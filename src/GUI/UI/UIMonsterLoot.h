#pragma once

#include "Engine/Objects/Actor.h"
#include "GUI/GUIWindow.h"

class GUIWindow_MonsterLoot : public GUIWindow {
 public:
    enum class Kind { Monster, Clicked, Ore, Bounty, Enchantment };
    explicit GUIWindow_MonsterLoot(int actorId);
    GUIWindow_MonsterLoot(ItemTreasureLevel level, RandomItemType type, Item firstItem);
    GUIWindow_MonsterLoot(ItemTreasureLevel level, RandomItemType type, Item firstItem, Kind kind);
    explicit GUIWindow_MonsterLoot(HouseId bountyHall);
    GUIWindow_MonsterLoot(int characterIndex, int inventoryIndex, Mastery mastery);
    void Update() override;
    void reroll();
    int actorId() const { return _actorId; }
    bool isClickedLoot() const { return _kind == Kind::Clicked; }
    Kind kind() const { return _kind; }
    const MonsterLootOffer &offer() const { return _offer; }
    bool timerWasPaused() const { return _timerWasPaused; }
    ScreenType previousScreen() const { return _previousScreen; }

 private:
    void initializeControls();
    int _actorId;
    ItemTreasureLevel _treasureLevel = ITEM_TREASURE_LEVEL_INVALID;
    RandomItemType _treasureType = RANDOM_ITEM_ANY;
    MonsterLootOffer _offer;
    bool _timerWasPaused = false;
    ScreenType _previousScreen = SCREEN_GAME;
    Kind _kind = Kind::Monster;
    HouseId _bountyHall = HOUSE_INVALID;
    int _characterIndex = -1;
    int _inventoryIndex = -1;
    Mastery _mastery = MASTERY_NONE;
    ItemId _enchantedItemId = ITEM_NULL;
};

void openMonsterLootPopup(int actorId);
void rerollMonsterLootPopup();
void takeMonsterLootPopup();
void closeMonsterLootPopup();
bool monsterLootPopupOpen();

// Scope random script rewards to a direct click on a decoration or face.
void beginClickedLootCapture();
void finishClickedLootCapture();
bool captureClickedRandomLoot(ItemTreasureLevel level, RandomItemType type);

/**
 * @return                              Whether Fortune's Turn is on for the running title, both its launch flag and
 *                                      the in-game dice checkbox.
 */
bool fortunesTurnSwitchedOn();

bool fortuneRerollActive();
void openOreRerollPopup(ItemTreasureLevel level, RandomItemType type);
void openBountyRerollPopup(HouseId hall);
void openWeaponEnchantRerollPopup(int characterIndex, int inventoryIndex, Mastery mastery);
void queueWeaponEnchantReroll(int characterIndex, int inventoryIndex, Mastery mastery);
void openPendingWeaponEnchantRerollPopup();
ItemEnchantment rollWeaponSpellEnchantment(ItemType itemType, Mastery mastery);
