#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <numbers>
#include <set>
#include <string_view>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Data/AwardEnums.h"
#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/MapEnums.h"
#include "Engine/Party.h"
#include "Engine/Random/Random.h"
#include "Engine/Tables/NPCTable.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/NPCTopics.h"
#include "Utility/Exception.h"

#include "MM7CampaignChain.h"

namespace {

struct RouteStep {
    MapId from;
    int leverEvent;
    int leverIndex;
    int leverValue;
    int transitionEvent;
    MapId to;
    std::string_view label;
};

constexpr std::array allBarrowMaps = {
    MAP_BARROW_DOWNS,
    MAP_BARROW_I,
    MAP_BARROW_II,
    MAP_BARROW_III,
    MAP_BARROW_IV,
    MAP_BARROW_V,
    MAP_BARROW_VI,
    MAP_BARROW_VII,
    MAP_BARROW_VIII,
    MAP_BARROW_IX,
    MAP_BARROW_X,
    MAP_BARROW_XI,
    MAP_BARROW_XII,
    MAP_BARROW_XIII,
    MAP_BARROW_XIV,
    MAP_BARROW_XV,
    MAP_ZOKARRS_TOMB,
};

void recordVisit(std::set<MapId> *visited) {
    visited->insert(engine->_currentLoadedMapId);
}

struct FaceApproach {
    int faceId;
    Vec3f position;
    int yaw;
    int pitch;
};

std::vector<FaceApproach> barrowEventFaceApproaches(int eventId) {
    constexpr size_t maxApproaches = 24;
    std::vector<FaceApproach> approaches;
    auto tryApproach = [&](int faceId, Vec2f offset) {
        const BLVFace &face = pIndoor->faces[faceId];
        Vec3f position = face.boundingBox.center() + Vec3f(offset.x, offset.y, 0);
        int sectorId = 0;
        position.z = GetIndoorFloorZ(position + Vec3f(0, 0, 40), &sectorId);
        if (position.z <= -29000 || position.z > face.boundingBox.z2 + 50 || position.z + 400 < face.boundingBox.z1)
            return;
        int yaw = std::lround(std::atan2(-offset.y, -offset.x) * 180 / std::numbers::pi);
        for (int pitch : {0, -25, 25})
            approaches.push_back({faceId, position, yaw, pitch});
    };
    for (float distance : {150.0f, 250.0f, 100.0f, 350.0f}) {
        for (int faceId = 0; faceId < std::ssize(pIndoor->faces); ++faceId) {
            const BLVFace &face = pIndoor->faces[faceId];
            const Vec2f normal = face.facePlane.normal.xy();
            if (face.eventId == eventId && face.Clickable() && normal.length() > 0.5f)
                tryApproach(faceId, normal / normal.length() * distance);
            if (approaches.size() >= maxApproaches)
                return approaches;
        }
    }
    for (float distance : {150.0f, 250.0f, 350.0f}) {
        for (int faceId = 0; faceId < std::ssize(pIndoor->faces); ++faceId) {
            const BLVFace &face = pIndoor->faces[faceId];
            if (face.eventId != eventId || !face.Clickable())
                continue;
            for (int angle = 0; angle < 360; angle += 45) {
                const float radians = angle * std::numbers::pi / 180;
                tryApproach(faceId, Vec2f(std::cos(radians), std::sin(radians)) * distance);
                if (approaches.size() >= maxApproaches)
                    return approaches;
            }
        }
    }
    return approaches;
}

bool interactWithBarrowFaceApproaches(EngineController &game, MapId map,
                                      const std::vector<FaceApproach> &approaches) {
    for (const FaceApproach &approach : approaches) {
        game.teleportTo(map, approach.position, approach.yaw, approach.pitch);
        try {
            game.pointMouseAtFace(approach.faceId);
        } catch (const Exception &) {
            continue;
        }
        game.pressAndReleaseButton(BUTTON_LEFT);
        game.tick(2);
        return true;
    }
    return false;
}

bool interactWithBarrowEventFace(EngineController &game, MapId map, int eventId) {
    return interactWithBarrowFaceApproaches(game, map, barrowEventFaceApproaches(eventId));
}

bool clickBarrowEventFace(EngineController &game, MapId map, int eventId, const std::function<bool()> &done) {
    for (const FaceApproach &approach : barrowEventFaceApproaches(eventId)) {
        game.teleportTo(map, approach.position, approach.yaw, approach.pitch);
        try {
            game.pointMouseAtFace(approach.faceId);
        } catch (const Exception &) {
            continue;
        }
        game.pressAndReleaseButton(BUTTON_LEFT);
        for (int frame = 0; frame < 8 && !done(); ++frame)
            game.tick();
        if (done())
            return true;
    }
    return false;
}

bool selectBarrowNpcEvent(EngineController &game, int npcId, unsigned int eventId) {
    NPCData &npc = pNPCStats->pNPCData[npcId];
    std::array eventIds = {npc.dialogue_1_evt_id, npc.dialogue_2_evt_id, npc.dialogue_3_evt_id,
                           npc.dialogue_4_evt_id, npc.dialogue_5_evt_id, npc.dialogue_6_evt_id};
    auto event = std::ranges::find(eventIds, eventId);
    if (event == eventIds.end())
        return false;
    DialogueId line = static_cast<DialogueId>(std::to_underlying(DIALOGUE_SCRIPTED_LINE_1) +
                                               (event - eventIds.begin()));
    std::vector<DialogueId> topics = prepareScriptedNPCDialogueTopics(&npc);
    auto option = std::ranges::find(topics, line);
    if (option == topics.end())
        return false;
    game.pressGuiButton(fmt::format("NpcDialogue_Option{}", option - topics.begin()));
    game.tick();
    return true;
}

void leaveBarrowDialogue(EngineController &game) {
    for (int i = 0; i < 4 && current_screen_type != SCREEN_GAME; ++i) {
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
    }
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
}

void useOutdoorBarrowEntrance(EngineController &game, int eventId, MapId destination) {
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_BARROW_DOWNS);
    auto face = std::ranges::find_if(pOutdoor->faces, [eventId](const BLVFace &candidate) {
        return candidate.eventId == eventId && candidate.Clickable();
    });
    ASSERT_NE(face, pOutdoor->faces.end());

    bool opened = false;
    for (float distance : {150.0f, 250.0f, -150.0f, -250.0f}) {
        if (opened)
            break;
        Vec3f position = face->boundingBox.center() + distance * face->facePlane.normal;
        position.z = face->boundingBox.z1;
        int yaw = std::lround(std::atan2(-face->facePlane.normal.y, -face->facePlane.normal.x) *
                              180 / std::numbers::pi);
        if (distance < 0)
            yaw += 180;
        game.teleportTo(MAP_BARROW_DOWNS, position, yaw);
        game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
        game.tick();
        opened = current_screen_type == SCREEN_INPUT_BLV;
    }
    ASSERT_TRUE(opened) << "outdoor Barrow entrance event " << eventId;
    game.pressGuiButton("Transition_Yes");
    game.skipLoadingScreen();
    ASSERT_EQ(engine->_currentLoadedMapId, destination);
}

void setRouteLever(EngineController &game, MapId map, int eventId, int mapVarIndex, int value) {
    ASSERT_EQ(engine->_currentLoadedMapId, map);
    ASSERT_TRUE(clickBarrowEventFace(game, map, eventId, [mapVarIndex, value] {
        return engine->_persistentVariables.mapVars[mapVarIndex] == value;
    })) << "lever event " << eventId << " on map " << std::to_underlying(map);
    ASSERT_EQ(engine->_persistentVariables.mapVars[mapVarIndex], value);
}

void useBarrowTransition(EngineController &game, MapId map, int eventId) {
    ASSERT_EQ(engine->_currentLoadedMapId, map);
    ASSERT_TRUE(clickBarrowEventFace(game, map, eventId, [map] {
        return engine->_currentLoadedMapId != map;
    })) << "transition event " << eventId << " on map " << std::to_underlying(map);
    ASSERT_NE(engine->_currentLoadedMapId, map);
}

void followRouteStep(EngineController &game, const RouteStep &step, std::set<MapId> *visited) {
    SCOPED_TRACE(step.label);
    ASSERT_EQ(engine->_currentLoadedMapId, step.from);
    if (step.leverEvent)
        setRouteLever(game, step.from, step.leverEvent, step.leverIndex, step.leverValue);
    useBarrowTransition(game, step.from, step.transitionEvent);
    ASSERT_EQ(engine->_currentLoadedMapId, step.to);
    recordVisit(visited);
}

void seedZokarrExitForBarrowVI(EngineController &game) {
    int chosenSeed = 0;
    int nextTarget = -1;
    game.runGameRoutine([&] {
        for (int seed = 1; seed <= 1000; ++seed) {
            grng->seed(seed);
            int candidate = grng->peek(6);
            if (candidate >= 3) {
                chosenSeed = seed;
                nextTarget = candidate;
                break;
            }
        }
    });
    ASSERT_NE(chosenSeed, 0);
    ASSERT_GE(nextTarget, 3);
    ASSERT_LT(nextTarget, 6);
}

} // namespace

GAME_TEST(MM7BarrowTraversal, AllBarrowsAndZokarrThroughAuthoredTransitions) {
    engine->config->debug.NoActors.setValue(true);
    game.startNewGame();

    std::set<MapId> visited;
    game.teleportTo(MAP_BARROW_DOWNS, Vec3f(), 0);
    recordVisit(&visited);

    // The Barrow IX entrance is one of the three authored entrances from the outdoor region.
    useOutdoorBarrowEntrance(game, 503, MAP_BARROW_IX);
    recordVisit(&visited);

    constexpr std::array firstHalf = {
        RouteStep{MAP_BARROW_IX, 0, 0, 0, 502, MAP_BARROW_V, "IX -> V"},
        RouteStep{MAP_BARROW_V, 0, 0, 0, 502, MAP_BARROW_II, "V -> II"},
        RouteStep{MAP_BARROW_II, 0, 0, 0, 501, MAP_BARROW_XIV, "II -> XIV"},
        RouteStep{MAP_BARROW_XIV, 0, 0, 0, 502, MAP_BARROW_VIII, "XIV -> VIII"},
        RouteStep{MAP_BARROW_VIII, 0, 0, 0, 501, MAP_BARROW_XV, "VIII -> XV"},
        RouteStep{MAP_BARROW_XV, 0, 0, 0, 501, MAP_BARROW_IV, "XV -> IV"},
        RouteStep{MAP_BARROW_IV, 0, 0, 0, 502, MAP_ZOKARRS_TOMB, "IV -> Zokarr"},
    };
    for (const RouteStep &step : firstHalf)
        followRouteStep(game, step, &visited);

    // Zokarr's only authored exit randomly returns to IV or VI. Seed the next six-way draw into the three entries
    // that select VI, so this coverage is deterministic without replacing or bypassing the authored RandomGoTo.
    seedZokarrExitForBarrowVI(game);
    useBarrowTransition(game, MAP_ZOKARRS_TOMB, 501);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_BARROW_VI);
    recordVisit(&visited);

    constexpr std::array secondHalf = {
        RouteStep{MAP_BARROW_VI, 0, 0, 0, 501, MAP_BARROW_XI, "VI -> XI"},
        RouteStep{MAP_BARROW_XI, 0, 0, 0, 501, MAP_BARROW_I, "XI -> I"},
        RouteStep{MAP_BARROW_I, 0, 0, 0, 501, MAP_BARROW_V, "I -> V"},
        RouteStep{MAP_BARROW_V, 452, 1, 2, 501, MAP_BARROW_XII, "V -> XII"},
        RouteStep{MAP_BARROW_XII, 452, 1, 2, 501, MAP_BARROW_III, "XII -> III"},
        RouteStep{MAP_BARROW_III, 0, 0, 0, 501, MAP_BARROW_X, "III -> X"},
        RouteStep{MAP_BARROW_X, 0, 0, 0, 502, MAP_BARROW_VII, "X -> VII"},
        RouteStep{MAP_BARROW_VII, 0, 0, 0, 502, MAP_BARROW_IX, "VII -> IX"},
        RouteStep{MAP_BARROW_IX, 0, 0, 0, 502, MAP_BARROW_V, "IX -> V again"},
        RouteStep{MAP_BARROW_V, 0, 0, 0, 502, MAP_BARROW_II, "V -> II again"},
        RouteStep{MAP_BARROW_II, 451, 0, 2, 502, MAP_BARROW_XIII, "II -> XIII"},
    };
    for (const RouteStep &step : secondHalf)
        followRouteStep(game, step, &visited);

    // Return to Barrow IX and take its authored outdoor exit.
    constexpr std::array returnToExit = {
        RouteStep{MAP_BARROW_XIII, 452, 1, 2, 501, MAP_BARROW_VI, "XIII -> VI"},
        RouteStep{MAP_BARROW_VI, 0, 0, 0, 501, MAP_BARROW_XI, "VI -> XI again"},
        RouteStep{MAP_BARROW_XI, 0, 0, 0, 501, MAP_BARROW_I, "XI -> I again"},
        RouteStep{MAP_BARROW_I, 0, 0, 0, 501, MAP_BARROW_V, "I -> V again"},
        RouteStep{MAP_BARROW_V, 0, 0, 0, 501, MAP_BARROW_XII, "V -> XII again"},
        RouteStep{MAP_BARROW_XII, 452, 1, 1, 501, MAP_BARROW_V, "XII -> V"},
        RouteStep{MAP_BARROW_V, 452, 1, 1, 501, MAP_BARROW_IX, "V -> IX"},
        RouteStep{MAP_BARROW_IX, 0, 0, 0, 501, MAP_BARROW_DOWNS, "IX -> outdoors"},
    };
    for (const RouteStep &step : returnToExit)
        followRouteStep(game, step, &visited);

    EXPECT_EQ(visited, std::set<MapId>(allBarrowMaps.begin(), allBarrowMaps.end()));
}

GAME_TEST(MM7BarrowQuests, MeditationPoolPromotesMonksAndHonorsCompanionsOnce) {
    engine->config->debug.NoActors.setValue(true);
    game.startNewGame();

    Character &monk = pParty->pCharacters[0];
    Character &companion = pParty->pCharacters[1];
    monk.classType = CLASS_MONK;
    companion.classType = CLASS_KNIGHT;
    const std::array experienceBefore = {monk.experience, companion.experience,
                                         pParty->pCharacters[2].experience, pParty->pCharacters[3].experience};
    const int goldBefore = pParty->GetGold();
    ASSERT_EQ(pNPCStats->pNPCData[55].dialogue_1_evt_id, 60u);

    game.teleportTo(MAP_ZOKARRS_TOMB, Vec3f(), 0);

    // The pool refuses a party that has not accepted Bartholomew Hume's meditation quest.
    ASSERT_TRUE(interactWithBarrowEventFace(game, MAP_ZOKARRS_TOMB, 376));
    EXPECT_EQ(current_screen_type, SCREEN_GAME);
    EXPECT_EQ(monk.classType, CLASS_MONK);
    EXPECT_EQ(companion.classType, CLASS_KNIGHT);
    EXPECT_EQ(monk.experience, experienceBefore[0]);
    EXPECT_EQ(companion.experience, experienceBefore[1]);
    EXPECT_FALSE(monk._achievedAwardsBits[AWARD_PROMOTION_INITIATE]);
    EXPECT_FALSE(companion._achievedAwardsBits[AWARD_PROMOTION_INITIATE_HONORARY]);

    // Event 58 normally establishes this exact active/topic state before the party reaches the pool.
    pParty->_questBits.set(QBIT_MEDITATION_SPOT_ACTIVE);
    pNPCStats->pNPCData[38].dialogue_1_evt_id = 59;
    ASSERT_TRUE(clickBarrowEventFace(game, MAP_ZOKARRS_TOMB, 376, [] {
        return current_screen_type == SCREEN_NPC_DIALOGUE;
    }));
    ASSERT_EQ(current_screen_type, SCREEN_NPC_DIALOGUE);
    ASSERT_TRUE(selectBarrowNpcEvent(game, 55, 60));

    EXPECT_FALSE(pParty->_questBits[QBIT_MEDITATION_SPOT_ACTIVE]);
    EXPECT_EQ(monk.classType, CLASS_INITIATE);
    EXPECT_EQ(companion.classType, CLASS_KNIGHT);
    EXPECT_TRUE(monk._achievedAwardsBits[AWARD_PROMOTION_INITIATE]);
    EXPECT_FALSE(monk._achievedAwardsBits[AWARD_PROMOTION_INITIATE_HONORARY]);
    EXPECT_FALSE(companion._achievedAwardsBits[AWARD_PROMOTION_INITIATE]);
    EXPECT_TRUE(companion._achievedAwardsBits[AWARD_PROMOTION_INITIATE_HONORARY]);
    EXPECT_EQ(monk.experience - experienceBefore[0], 30000);
    EXPECT_EQ(companion.experience - experienceBefore[1], 15000);
    for (int i = 2; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].experience - experienceBefore[i], 15000);
        EXPECT_TRUE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_PROMOTION_INITIATE_HONORARY]);
    }
    EXPECT_EQ(pParty->GetGold(), goldBefore);
    EXPECT_EQ(pNPCStats->pNPCData[38].dialogue_1_evt_id, 61u);
    EXPECT_EQ(pNPCStats->pNPCData[55].dialogue_1_evt_id, 0u);

    leaveBarrowDialogue(game);
    const std::array experienceAfter = {monk.experience, companion.experience,
                                        pParty->pCharacters[2].experience, pParty->pCharacters[3].experience};
    ASSERT_TRUE(interactWithBarrowEventFace(game, MAP_ZOKARRS_TOMB, 376));
    EXPECT_EQ(current_screen_type, SCREEN_GAME);
    EXPECT_FALSE(pParty->_questBits[QBIT_MEDITATION_SPOT_ACTIVE]);
    for (int i = 0; i < 4; ++i)
        EXPECT_EQ(pParty->pCharacters[i].experience, experienceAfter[i]);
}

GAME_TEST(MM7BarrowQuests, ZokarrBonesRequireActiveQuestAndHeldSkull) {
    engine->config->debug.NoActors.setValue(true);
    game.startNewGame();

    const std::array experienceBefore = {pParty->pCharacters[0].experience, pParty->pCharacters[1].experience,
                                         pParty->pCharacters[2].experience, pParty->pCharacters[3].experience};
    const std::array awardsBefore = {pParty->pCharacters[0]._achievedAwardsBits,
                                     pParty->pCharacters[1]._achievedAwardsBits,
                                     pParty->pCharacters[2]._achievedAwardsBits,
                                     pParty->pCharacters[3]._achievedAwardsBits};
    const int goldBefore = pParty->GetGold();
    const std::array greenTopicsBefore = {pNPCStats->pNPCData[50].dialogue_1_evt_id,
                                          pNPCStats->pNPCData[50].dialogue_2_evt_id};
    const std::array dwarfTopicsBefore = {pNPCStats->pNPCData[59].dialogue_1_evt_id,
                                          pNPCStats->pNPCData[59].dialogue_2_evt_id};

    game.teleportTo(MAP_ZOKARRS_TOMB, Vec3f(), 0);
    const std::vector<FaceApproach> bonesApproaches = barrowEventFaceApproaches(377);
    ASSERT_FALSE(bonesApproaches.empty());

    // Possessing the skull without Master Green's active quest is refused and preserves the item.
    pParty->setHoldingItem(Item(ITEM_QUEST_ZOKARR_IVS_SKULL));
    ASSERT_TRUE(interactWithBarrowFaceApproaches(game, MAP_ZOKARRS_TOMB, bonesApproaches));
    EXPECT_EQ(pParty->pPickedItem.itemId, ITEM_QUEST_ZOKARR_IVS_SKULL);
    EXPECT_FALSE(pParty->_questBits[QBIT_ZOKARR_BONES_RETURNED]);

    // The active quest alone is also insufficient: the authored event requires the skull in hand.
    pParty->pPickedItem.Reset();
    pParty->_questBits.set(QBIT_ZOKARR_BONES_ACTIVE);
    ASSERT_TRUE(interactWithBarrowFaceApproaches(game, MAP_ZOKARRS_TOMB, bonesApproaches));
    EXPECT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
    EXPECT_FALSE(pParty->_questBits[QBIT_ZOKARR_BONES_RETURNED]);

    pParty->setHoldingItem(Item(ITEM_QUEST_ZOKARR_IVS_SKULL));
    pParty->_questBits.set(QBIT_ORACLE_RECOVERS_ZOKARR_IVS_SKULL);
    ASSERT_TRUE(interactWithBarrowFaceApproaches(game, MAP_ZOKARRS_TOMB, bonesApproaches));
    ASSERT_TRUE(pParty->_questBits[QBIT_ZOKARR_BONES_RETURNED]);
    EXPECT_TRUE(pParty->_questBits[QBIT_ZOKARR_BONES_ACTIVE]);
    EXPECT_TRUE(pParty->_questBits[QBIT_ZOKARR_BONES_RETURNED]);
    EXPECT_FALSE(pParty->_questBits[QBIT_ORACLE_RECOVERS_ZOKARR_IVS_SKULL]);
    EXPECT_FALSE(pParty->_questBits[QBIT_FANFARE]);
    EXPECT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);

    // A duplicate skull cannot replay the deposit or consume another item after the returned bit is set.
    pParty->setHoldingItem(Item(ITEM_QUEST_ZOKARR_IVS_SKULL));
    ASSERT_TRUE(interactWithBarrowFaceApproaches(game, MAP_ZOKARRS_TOMB, bonesApproaches));
    EXPECT_EQ(pParty->pPickedItem.itemId, ITEM_QUEST_ZOKARR_IVS_SKULL);
    EXPECT_TRUE(pParty->_questBits[QBIT_ZOKARR_BONES_RETURNED]);
    EXPECT_EQ(pParty->GetGold(), goldBefore);
    const std::array greenTopicsAfter = {pNPCStats->pNPCData[50].dialogue_1_evt_id,
                                         pNPCStats->pNPCData[50].dialogue_2_evt_id};
    const std::array dwarfTopicsAfter = {pNPCStats->pNPCData[59].dialogue_1_evt_id,
                                         pNPCStats->pNPCData[59].dialogue_2_evt_id};
    EXPECT_EQ(greenTopicsAfter, greenTopicsBefore);
    EXPECT_EQ(dwarfTopicsAfter, dwarfTopicsBefore);
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].experience, experienceBefore[i]);
        EXPECT_EQ(pParty->pCharacters[i]._achievedAwardsBits, awardsBefore[i]);
    }
}
