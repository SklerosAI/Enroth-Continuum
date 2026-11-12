#include "Engine/MM6/MM6Finale.h"

#include <array>
#include <memory>
#include <utility>

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Evt/EvtInterpreter.h"
#include "Engine/Evt/MM6ItemEventCompare.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/MM6/MM6MapTransition.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Party.h"
#include "Engine/Tables/MapTable.h"

#include "GUI/GUIMessageQueue.h"
#include "GUI/UI/UIGameOver.h"
#include "GUI/UI/UIStatusBar.h"

#include "Media/Audio/AudioPlayer.h"
#include "Media/MediaPlayer.h"

#include "Utility/String/Ascii.h"

namespace mm6 {

static constexpr MonsterId demonQueen = static_cast<MonsterId>(172);
static constexpr MonsterId reactor = static_cast<MonsterId>(173);
static constexpr QuestBit reactorDestroyed = static_cast<QuestBit>(180);
static constexpr QuestBit demonQueenKilled = static_cast<QuestBit>(202);
static constexpr QuestBit escapedWithRitual = static_cast<QuestBit>(237); // Set by hive:60.
static constexpr int ritualOfTheVoid = 544;

/**
 * One monster the reactor's death summons, from an encounter slot of the hive at tier C.
 */
struct FinaleSummon {
    int16_t encounterSlot;
    int16_t x;
    int16_t y;
};

static constexpr std::array<FinaleSummon, 16> reactorSummons = {{
    {2, 4352, 20096}, {2, 6016, 21504}, {2, 2816, 22016}, {1, 4352, 24704},
    {1, 2944, 23552}, {1, 6144, 23424}, {2, 2688, 19840}, {2, 1920, 21760},
    {2, 6144, 19840}, {2, 7168, 21760}, {1, 2584, 25728}, {1, 5248, 25728},
    {1, 1792, 23168}, {1, 2688, 25216}, {1, 7296, 23040}, {1, 6144, 25088}
}};

static FinaleEnding shownEnding = FINALE_ENDING_LOSE;

std::optional<FinaleEnding> finaleEndingForHouse(HouseId houseId) {
    switch (std::to_underlying(houseId)) {
    case 600: return FINALE_ENDING_WIN;
    case 601: return FINALE_ENDING_LOSE;
    default: return std::nullopt;
    }
}

void queueFinaleEnding(FinaleEnding ending) {
    engine->_statusBar->clearAll();
    engine->_messageQueue->clear();
    engine->_messageQueue->addMessageCurrentFrame(UIMSG_ShowGameOverWindow, std::to_underlying(ending), 0);
}

static bool partyCarriesRitual() {
    for (const Character &character : pParty->pCharacters)
        if (hasEventItem(character, pParty->pPickedItem, 17, ritualOfTheVoid))
            return true;
    return false;
}

void onFinaleMonsterDeath(const Actor &actor) {
    if (actor.monsterInfo.id == demonQueen) {
        pParty->_questBits.set(demonQueenKilled);
        pAudioPlayer->playUISound(static_cast<SoundId>(997));
    }
    if (actor.monsterInfo.id != reactor)
        return;

    if (!partyCarriesRitual()) {
        queueFinaleEnding(FINALE_ENDING_LOSE);
        return;
    }
    pParty->_questBits.set(reactorDestroyed);
    for (const FinaleSummon &summon : reactorSummons)
        spawnMonsters(summon.encounterSlot, 3, 1, Vec3f(summon.x, summon.y, -2256), 0, 0);
    switchDoorAnimation(28, DOOR_ACTION_OPEN);
    switchDoorAnimation(30, DOOR_ACTION_CLOSE);
    switchDoorAnimation(51, DOOR_ACTION_OPEN);
    switchDoorAnimation(52, DOOR_ACTION_OPEN);
    switchDoorAnimation(53, DOOR_ACTION_CLOSE);
    pParty->pos.x = 3328;
    pParty->pos.y = 25920;
    pParty->_viewYaw = 512;
    pAudioPlayer->playUISound(static_cast<SoundId>(232));
    for (Character &character : pParty->pCharacters)
        character.conditions.resetAll();
}

void removeDefeatedFinaleBosses() {
    for (Actor &actor : pActors)
        if ((actor.monsterInfo.id == reactor && pParty->_questBits.test(reactorDestroyed)) ||
            (actor.monsterInfo.id == demonQueen && pParty->_questBits.test(demonQueenKilled)))
            actor.aiState = Removed;
}

bool isHiveFinaleUnderway() {
    return ascii::noCaseEquals(pMapTable->pInfos[engine->_currentLoadedMapId].fileName, "hive.blv") &&
           pParty->_questBits.test(reactorDestroyed) && !pParty->_questBits.test(escapedWithRitual);
}

void playFinaleEnding(FinaleEnding ending) {
    shownEnding = ending;
    pMediaPlayer->PlayFullscreenMovie("comped");
    pMediaPlayer->PlayFullscreenMovie(ending == FINALE_ENDING_WIN ? "end_dome" : "planetxp");
    if (ending == FINALE_ENDING_WIN)
        pMediaPlayer->PlayFullscreenMovie("end_seq1");
    pGameOverWindow = std::make_unique<GUIWindow_MM6GameOver>(ending);
    uGameState = GAME_STATE_FINAL_WINDOW;
}

void closeFinaleEnding() {
    if (shownEnding == FINALE_ENDING_LOSE) {
        engine->_messageQueue->clear();
        current_screen_type = SCREEN_GAME;
        engine->_statusBar->clearAll();
        uGameState = GAME_STATE_GAME_QUITTING_TO_MAIN_MENU;
        return;
    }
    const MapId newSorpigal = pMapTable->GetMapInfo("oute3.odm");
    leaveMap(newSorpigal);
    engine->_pendingTransition = MapDestination(newSorpigal, MAP_START_POINT_PARTY);
    uGameState = GAME_STATE_CHANGE_LOCATION;
}

bool loseOnLeavingHive() {
    if (!isHiveFinaleUnderway())
        return false;
    engine->_pendingTransition.reset();
    playFinaleEnding(FINALE_ENDING_LOSE);
    return true;
}

} // namespace mm6
