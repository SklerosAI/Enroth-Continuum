#include <algorithm>
#include <optional>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/Monsters.h"
#include "Engine/Party.h"
#include "Engine/Resources/EngineFileSystem.h"
#include "Engine/Tables/NPCTable.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/UIHouses.h"

#include "MM7CampaignChain.h"

static bool isXenofex(const Actor &actor) {
    return actor.uniqueNameIndex != 0 && pMonsterStats->uniqueNames[actor.uniqueNameIndex] == "Xenofex";
}

static bool movesToMap(int eventId) {
    if (!engine->_localEventMap.hasEvent(eventId))
        return false;
    return std::ranges::any_of(engine->_localEventMap.function(eventId), [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_MoveToMap;
    });
}

GAME_TEST(MM7CampaignChain, DISABLED_Chapter6) {
    // Fixtures: teleports to the Castle Lambent throne room door, to the Celeste town portal arrival point, to the
    // Colony Zod arrival point, into melee range of Xenofex as he moves, and to the Colony Zod exit.
    assertCampaignChainProfile();
    ASSERT_FALSE(ufs->exists("saves/save006.mm7"));
    loadCampaignChainSlot(game, 5);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_CELESTE);
    ASSERT_TRUE(pParty->_questBits[QBIT_TOLBERTI_ASSASSINATED]);
    ASSERT_FALSE(pParty->_questBits[QBIT_XENOFEX_LIGHT_ACTIVE]);

    auto activeTape = tapes.questBit(QBIT_XENOFEX_LIGHT_ACTIVE);
    auto killedTape = tapes.questBit(QBIT_XENOFEX_KILLED);
    auto finishedTape = tapes.questBit(QBIT_XENOFEX_QUEST_FINISHED);
    auto finalTaskTape = tapes.questBit(QBIT_OSCILLATION_OVERTHRUSTER_LIGHT_ACTIVE);
    auto topicTape = tapes.custom([] { return pNPCStats->pNPCData[80].dialogue_2_evt_id; });
    auto xenofexAliveTape = tapes.custom([] {
        auto xenofex = std::ranges::find_if(pActors, isXenofex);
        return engine->_currentLoadedMapId == MAP_COLONY_ZOD && xenofex != pActors.end() ? std::optional(!xenofex->IsNotAlive()) : std::nullopt;
    });
    auto goldTape = tapes.gold();
    auto mapTape = tapes.map();
    auto houseTape = tapes.house();
    test.startTaping();

    openCampaignChainThroneRoom(game, 80);
    ASSERT_TRUE(selectCampaignChainNpcEvent(game, 80, 149));
    leaveCampaignChainHouse(game);
    enterCampaignChainCelesteHouse(game, HOUSE_TEMPLE_CELESTE); // The party is still hurt from Tolberti.
    auto healer = std::ranges::find(houseNpcs, HOUSE_PROPRIETOR, &HouseNpcDesc::type);
    ASSERT_NE(healer, houseNpcs.end());
    game.pressGuiButton(fmt::format("House_Npc{}", healer - houseNpcs.begin()));
    game.tick();
    for (int i = 0; i < 4; i++) {
        game.pressGuiButton(fmt::format("Game_Character{}", i + 1));
        game.tick();
        game.pressGuiButton("HouseDialogue_Option0"); // Heal.
        game.tick(2); // The heal closes its dialogue on the next frame.
    }
    leaveCampaignChainHouse(game);

    game.teleportTo(MAP_COLONY_ZOD, Vec3f(-1849, 6726, 934), 0);
    std::vector<int> candidates;
    for (const Actor &actor : pActors)
        if (isXenofex(actor))
            candidates.push_back(actor.id);
    ASSERT_EQ(candidates.size(), 1u);
    const int xenofexId = candidates.front();
    game.teleportTo(MAP_COLONY_ZOD, pActors[xenofexId].pos + Vec3f(0, -250, 0), 90);
    game.pointMouseAtActor(xenofexId);
    for (int attack = 0; attack < 1000 && !pActors[xenofexId].IsNotAlive(); attack++) {
        if (attack && attack % 20 == 0) { // Xenofex walks around, the party follows him.
            game.teleportTo(MAP_COLONY_ZOD, pActors[xenofexId].pos + Vec3f(0, -250, 0), 90);
            game.pointMouseAtActor(xenofexId);
        }
        game.pressAndReleaseKey(PlatformKey::KEY_A);
        game.tick();
    }
    ASSERT_TRUE(pActors[xenofexId].IsNotAlive());
    auto exit = std::ranges::find_if(pIndoor->faces, [](const BLVFace &face) {
        return face.Clickable() && movesToMap(face.eventId);
    });
    ASSERT_NE(exit, pIndoor->faces.end());
    useCampaignChainTransition(game, MAP_COLONY_ZOD, exit->eventId);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_LAND_OF_THE_GIANTS);

    openCampaignChainThroneRoom(game, 80);
    ASSERT_TRUE(selectCampaignChainNpcEvent(game, 80, 150));
    leaveCampaignChainHouse(game);
    openCampaignChainThroneRoom(game, 80);
    ASSERT_TRUE(selectCampaignChainNpcEvent(game, 80, 169));
    leaveCampaignChainHouse(game);

    EXPECT_EQ(activeTape, tape(false, true, false));
    EXPECT_EQ(killedTape, tape(false, true));
    EXPECT_EQ(finishedTape, tape(false, true));
    EXPECT_EQ(finalTaskTape, tape(false, true));
    EXPECT_EQ(topicTape, tape(149u, 150u, 169u, 170u));
    EXPECT_EQ(xenofexAliveTape, tape(std::optional<bool>(), std::optional(true), std::optional(false), std::optional<bool>()));
    EXPECT_EQ(goldTape.adjacentDeltas().back(), 50000);
    EXPECT_EQ(mapTape, tape(MAP_CELESTE, MAP_CASTLE_LAMBENT, MAP_CELESTE, MAP_COLONY_ZOD, MAP_LAND_OF_THE_GIANTS, MAP_CASTLE_LAMBENT));
    EXPECT_EQ(houseTape, tape(HOUSE_INVALID, HOUSE_CASTLE_LAMBENT_THRONE_ROOM, HOUSE_INVALID, HOUSE_TEMPLE_CELESTE, HOUSE_INVALID,
                              HOUSE_CASTLE_LAMBENT_THRONE_ROOM, HOUSE_INVALID, HOUSE_CASTLE_LAMBENT_THRONE_ROOM, HOUSE_INVALID));
    ASSERT_FALSE(::testing::Test::HasFailure());

    saveCampaignChainSlot(game, 6);
}
