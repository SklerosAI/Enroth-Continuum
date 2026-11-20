#pragma once

#include "Engine/Objects/Actor.h"
#include "GUI/GUIWindow.h"

class GUIWindow_MonsterLoot : public GUIWindow {
 public:
    explicit GUIWindow_MonsterLoot(int actorId);
    GUIWindow_MonsterLoot(ItemTreasureLevel level, RandomItemType type, Item firstItem);
    void Update() override;
    void reroll();
    int actorId() const { return _actorId; }
    bool isClickedLoot() const { return _actorId < 0; }
    const MonsterLootOffer &offer() const { return _offer; }
    bool timerWasPaused() const { return _timerWasPaused; }

 private:
    int _actorId;
    ItemTreasureLevel _treasureLevel = ITEM_TREASURE_LEVEL_INVALID;
    RandomItemType _treasureType = RANDOM_ITEM_ANY;
    MonsterLootOffer _offer;
    bool _timerWasPaused = false;
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
