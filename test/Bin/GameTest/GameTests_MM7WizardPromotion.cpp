#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <numbers>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Data/AwardEnums.h"
#include "Engine/Data/HouseEnums.h"
#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Evt/EvtContext.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Evt/Processor.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/CharacterEnums.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Objects/Decoration.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Objects/NPC.h"
#include "Engine/Party.h"
#include "Engine/PartyEnums.h"
#include "Engine/Tables/ChestTable.h"
#include "Engine/Tables/NPCTable.h"

#include "GUI/UI/UIChest.h"
#include "GUI/UI/NPCTopics.h"
#include "GUI/UI/UIDialogue.h"

#include "Utility/Exception.h"

namespace {

struct GolemActorState {
    ActorAttributes attributes;
    AIState aiState;

    bool operator==(const GolemActorState &) const = default;
};

struct GolemPartSource {
    MapId map;
    ItemId item;
    int chestId;
    int eventId;
    QuestBit oracleBit;
    QuestBit placedBit;
};

constexpr std::array commonGolemParts = {
    GolemPartSource{MAP_BARROW_DOWNS, ITEM_QUEST_GOLEM_CHEST, 0, 220,
                    QBIT_ORACLE_RECOVERS_GOLEM_CHEST, QBIT_GOLEM_CHEST_PLACED},
    GolemPartSource{MAP_DEYJA, ITEM_QUEST_GOLEM_LEFT_LEG, 0, 170,
                    QBIT_ORACLE_RECOVERS_GOLEM_LEFT_LEG, QBIT_GOLEM_LEFT_LEG_PLACED},
    GolemPartSource{MAP_DEYJA, ITEM_QUEST_GOLEM_RIGHT_LEG, 19, 169,
                    QBIT_ORACLE_RECOVERS_GOLEM_RIGHT_LEG, QBIT_GOLEM_RIGHT_LEG_PLACED},
    GolemPartSource{MAP_AVLEE, ITEM_QUEST_GOLEM_LEFT_ARM, 0, 170,
                    QBIT_ORACLE_RECOVERS_GOLEM_LEFT_ARM, QBIT_GOLEM_LEFT_ARM_PLACED},
    GolemPartSource{MAP_TATALIA, ITEM_QUEST_GOLEM_RIGHT_ARM, 0, 170,
                    QBIT_ORACLE_RECOVERS_GOLEM_RIGHT_ARM, QBIT_GOLEM_RIGHT_ARM_PLACED},
};

constexpr GolemPartSource goodGolemHead = {
    MAP_BRACADA_DESERT, ITEM_QUEST_GOLEM_HEAD, 19, 219,
    QBIT_ORACLE_RECOVERS_GOLEM_HEAD, QBIT_GOLEM_HEAD_PLACED,
};

constexpr GolemPartSource dentedGolemHead = {
    MAP_BRACADA_DESERT, ITEM_QUEST_ABBEY_NORMAL_GOLEM_HEAD, 0, 220,
    QBIT_ORACLE_RECOVERS_ABBEY_NORMAL_GOLEM_HEAD, QBIT_ABBEY_NORMAL_GOLEM_HEAD_PLACED,
};

std::vector<GolemActorState> golemActorStates() {
    std::vector<GolemActorState> result;
    for (const Actor &actor : pActors)
        if (actor.group == 6)
            result.push_back({actor.attributes, actor.aiState});
    return result;
}

bool eventSpeaksNpc(int eventId, int npcId) {
    return engine->_localEventMap.hasEvent(eventId) &&
           std::ranges::any_of(engine->_localEventMap.function(eventId), [npcId](const EvtInstruction &instruction) {
               return instruction.opcode == EVENT_SpeakNPC && instruction.data.npc_descr.npc_id == npcId;
           });
}

bool eventOpensChest(int eventId, int chestId) {
    return engine->_localEventMap.hasEvent(eventId) &&
           std::ranges::any_of(engine->_localEventMap.function(eventId), [chestId](const EvtInstruction &instruction) {
               return instruction.opcode == EVENT_OpenChest && instruction.data.chest_id == chestId;
           });
}

bool selectNpcEvent(EngineController &game, NPCData *npc, unsigned int eventId) {
    std::array eventIds = {npc->dialogue_1_evt_id, npc->dialogue_2_evt_id, npc->dialogue_3_evt_id,
                           npc->dialogue_4_evt_id, npc->dialogue_5_evt_id, npc->dialogue_6_evt_id};
    auto event = std::ranges::find(eventIds, eventId);
    if (event == eventIds.end())
        return false;
    DialogueId line = static_cast<DialogueId>(std::to_underlying(DIALOGUE_SCRIPTED_LINE_1) +
                                               (event - eventIds.begin()));
    std::vector<DialogueId> topics = prepareScriptedNPCDialogueTopics(npc);
    auto option = std::ranges::find(topics, line);
    if (option == topics.end())
        return false;
    const char *prefix = current_screen_type == SCREEN_NPC_DIALOGUE ?
                         "NpcDialogue_Option" : "HouseNpcDialogue_Option";
    game.pressGuiButton(fmt::format("{}{}", prefix, option - topics.begin()));
    game.tick();
    return true;
}

bool npcEventVisible(NPCData *npc, unsigned int eventId) {
    std::array eventIds = {npc->dialogue_1_evt_id, npc->dialogue_2_evt_id, npc->dialogue_3_evt_id,
                           npc->dialogue_4_evt_id, npc->dialogue_5_evt_id, npc->dialogue_6_evt_id};
    auto event = std::ranges::find(eventIds, eventId);
    if (event == eventIds.end())
        return false;
    DialogueId line = static_cast<DialogueId>(std::to_underlying(DIALOGUE_SCRIPTED_LINE_1) +
                                               (event - eventIds.begin()));
    std::vector<DialogueId> topics = prepareScriptedNPCDialogueTopics(npc);
    return std::ranges::find(topics, line) != topics.end();
}

void leaveDialogue(EngineController &game) {
    for (int attempt = 0; attempt < 5 && current_screen_type != SCREEN_GAME; ++attempt) {
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
    }
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
}

void openThomasGrey(EngineController &game) {
    game.teleportTo(MAP_SCHOOL_OF_SORCERY, Vec3f(), 0);
    auto actor = std::ranges::find(pActors, 48, &Actor::npcId);
    if (actor != pActors.end()) {
        for (const auto &[offset, yaw] : std::array{
                 std::pair{Vec3f(200, 0, 0), 180}, std::pair{Vec3f(-200, 0, 0), 0},
                 std::pair{Vec3f(0, 200, 0), 270}, std::pair{Vec3f(0, -200, 0), 90}}) {
            game.teleportTo(MAP_SCHOOL_OF_SORCERY, actor->pos + offset, yaw);
            try {
                game.pointMouseAtActor(actor->id);
            } catch (const Exception &) {
                continue;
            }
            game.pressAndReleaseButton(BUTTON_LEFT);
            game.tick();
            if (current_screen_type == SCREEN_NPC_DIALOGUE)
                break;
        }
    } else {
        auto decoration = std::ranges::find_if(pLevelDecorations, [](const LevelDecoration &candidate) {
            return candidate.uEventID != 0 && eventSpeaksNpc(candidate.uEventID, 48);
        });
        if (decoration != pLevelDecorations.end()) {
            const int decorationId = decoration - pLevelDecorations.begin();
            game.teleportTo(MAP_SCHOOL_OF_SORCERY, decoration->vPosition + Vec3f(300, 0, 0), 180);
            game.pointMouseAtDecoration(decorationId);
        } else {
            auto face = std::ranges::find_if(pIndoor->faces, [](const BLVFace &candidate) {
                return candidate.Clickable() && eventSpeaksNpc(candidate.eventId, 48);
            });
            ASSERT_NE(face, pIndoor->faces.end());
            const int faceId = face - pIndoor->faces.begin();
            Vec3f position = face->boundingBox.center() + 150 * face->facePlane.normal;
            position.z = face->boundingBox.z1;
            int yaw = std::lround(std::atan2(-face->facePlane.normal.y, -face->facePlane.normal.x) *
                                  180 / std::numbers::pi);
            game.teleportTo(MAP_SCHOOL_OF_SORCERY, position, yaw);
            game.pointMouseAtFace(faceId);
        }
        game.pressAndReleaseButton(BUTTON_LEFT);
        game.tick();
    }
    ASSERT_EQ(current_screen_type, SCREEN_NPC_DIALOGUE);
    ASSERT_EQ(getNPCData(speakingNpcId)->name, pNPCStats->pNPCData[48].name);
}

void openGolemDialogue(EngineController &game) {
    pParty->CountHirelings();
    game.pressGuiButton("Game_Hireling2");
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_NPC_DIALOGUE);
    ASSERT_EQ(getNPCData(speakingNpcId)->name, pNPCStats->pNPCData[56].name);
}

bool clickOutdoorEventSource(EngineController &game, MapId map, int eventId, const std::function<bool()> &done) {
    for (int decorationId = 0; decorationId < std::ssize(pLevelDecorations); ++decorationId) {
        const LevelDecoration &decoration = pLevelDecorations[decorationId];
        if (decoration.uEventID != eventId)
            continue;
        for (const auto &[offset, yaw] : std::array{
                 std::pair{Vec3f(300, 0, 0), 180}, std::pair{Vec3f(-300, 0, 0), 0},
                 std::pair{Vec3f(0, 300, 0), 270}, std::pair{Vec3f(0, -300, 0), 90}}) {
            game.teleportTo(map, decoration.vPosition + offset, yaw);
            try {
                game.pointMouseAtDecoration(decorationId);
            } catch (const Exception &) {
                continue;
            }
            for (int attempt = 0; attempt < 2 && !done(); ++attempt) {
                game.pressAndReleaseButton(BUTTON_LEFT);
                game.tick();
            }
            if (done())
                return true;
        }
    }

    for (int faceId = 0; faceId < std::ssize(pOutdoor->faces); ++faceId) {
        const BLVFace &face = pOutdoor->faces[faceId];
        if (face.eventId != eventId || !face.Clickable())
            continue;
        for (float distance : {150.0f, 300.0f, 500.0f}) {
            for (const auto &[direction, yaw] : std::array{
                     std::pair{Vec3f(1, 0, 0), 180}, std::pair{Vec3f(-1, 0, 0), 0},
                     std::pair{Vec3f(0, 1, 0), 270}, std::pair{Vec3f(0, -1, 0), 90}}) {
                Vec3f position = face.boundingBox.center() + distance * direction;
                bool onWater = false;
                int floorFace = -1;
                position.z = ODM_GetFloorLevel(position, &onWater, &floorFace);
                game.teleportTo(map, position, yaw, -30);
                try {
                    game.pointMouseAtFace(faceId);
                } catch (const Exception &) {
                    continue;
                }
                for (int attempt = 0; attempt < 2 && !done(); ++attempt) {
                    game.pressAndReleaseButton(BUTTON_LEFT);
                    game.tick();
                }
                if (done())
                    return true;
            }
        }
    }
    return false;
}

bool storeHeldItemInParty();

void acquireGolemPart(EngineController &game, const GolemPartSource &source) {
    SCOPED_TRACE(fmt::format("map={}, item={}, chest={}, event={}", std::to_underlying(source.map),
                             std::to_underlying(source.item), source.chestId, source.eventId));
    game.teleportTo(source.map, Vec3f(), 0);
    ASSERT_LT(source.chestId, std::ssize(vChests));
    InventoryEntry entry = vChests[source.chestId].inventory.find(source.item);
    ASSERT_TRUE(entry);
    ASSERT_TRUE(eventOpensChest(source.eventId, source.chestId));
    const bool opened = clickOutdoorEventSource(game, source.map, source.eventId, [] {
        return current_screen_type == SCREEN_CHEST;
    });
    ASSERT_TRUE(opened);
    ASSERT_EQ(current_screen_type, SCREEN_CHEST);
    ASSERT_EQ(pGUIWindow_CurrentChest->chestId(), source.chestId);
    entry = vChests[source.chestId].inventory.find(source.item);
    ASSERT_TRUE(entry);
    ASSERT_EQ(entry.zone(), INVENTORY_ZONE_GRID);
    const Pointi itemPosition = chestTable[vChests[source.chestId].chestTypeId].inventoryOffset +
                                entry.geometry().topLeft() * 32 + Pointi(16, 16);
    game.pressAndReleaseButton(BUTTON_LEFT, itemPosition);
    game.tick();
    ASSERT_EQ(pParty->pPickedItem.itemId, source.item);
    ASSERT_TRUE(storeHeldItemInParty());
    ASSERT_TRUE(pParty->hasItem(source.item))
        << "held=" << std::to_underlying(pParty->pPickedItem.itemId)
        << ", still_in_chest=" << static_cast<bool>(vChests[source.chestId].inventory.find(source.item));
    EXPECT_TRUE(pParty->_questBits[source.oracleBit]);
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
}

bool holdGolemPart(ItemId item) {
    for (Character &character : pParty->pCharacters) {
        InventoryEntry entry = character.inventory.find(item);
        if (!entry)
            continue;
        pParty->setHoldingItem(character.inventory.take(entry));
        return true;
    }
    return false;
}

bool storeHeldItemInParty() {
    if (pParty->pPickedItem.itemId == ITEM_NULL)
        return true;
    for (Character &character : pParty->pCharacters) {
        if (character.inventory.add(pParty->pPickedItem)) {
            pParty->takeHoldingItem();
            return true;
        }
    }
    return false;
}

void installGolemPart(EngineController &game, const GolemPartSource &source) {
    SCOPED_TRACE(fmt::format("installing item {} from map {}", std::to_underlying(source.item),
                             std::to_underlying(source.map)));
    ASSERT_TRUE(holdGolemPart(source.item));
    openGolemDialogue(game);
    ASSERT_TRUE(selectNpcEvent(game, getNPCData(speakingNpcId), 104));
    EXPECT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
    EXPECT_TRUE(pParty->_questBits[source.placedBit]);
    EXPECT_FALSE(pParty->_questBits[source.oracleBit]);
    leaveDialogue(game);
}

void runWizardPromotion(EngineController &game, bool dentedHead) {
    engine->config->debug.NoActors.setValue(true);
    engine->config->debug.NoDamage.setValue(true);
    game.startNewGame();

    NPCData &thomas = pNPCStats->pNPCData[48];
    NPCData &golem = pNPCStats->pNPCData[56];
    pParty->pCharacters[0].classType = CLASS_SORCERER;
    for (int i = 1; i < 4; ++i)
        pParty->pCharacters[i].classType = CLASS_KNIGHT;
    const std::array experienceBefore = {pParty->pCharacters[0].experience, pParty->pCharacters[1].experience,
                                         pParty->pCharacters[2].experience, pParty->pCharacters[3].experience};
    const std::array classBefore = {pParty->pCharacters[0].classType, pParty->pCharacters[1].classType,
                                    pParty->pCharacters[2].classType, pParty->pCharacters[3].classType};
    const std::array awardsBefore = {pParty->pCharacters[0]._achievedAwardsBits,
                                     pParty->pCharacters[1]._achievedAwardsBits,
                                     pParty->pCharacters[2]._achievedAwardsBits,
                                     pParty->pCharacters[3]._achievedAwardsBits};
    const int goldBefore = pParty->GetGold();
    ASSERT_EQ(thomas.dialogue_1_evt_id, 92u);
    ASSERT_EQ(thomas.house, HOUSE_521);

    openThomasGrey(game);
    const int reputationBefore = pParty->GetPartyReputation();
    ASSERT_TRUE(selectNpcEvent(game, &thomas, 92));
    ASSERT_TRUE(pParty->_questBits[QBIT_GOLEM_ACTIVE]);
    EXPECT_EQ(thomas.dialogue_1_evt_id, 93u);
    EXPECT_TRUE(golem.flags & NPC_HIRED);
    EXPECT_EQ(golem.dialogue_1_evt_id, 104u);
    EXPECT_EQ(golem.dialogue_2_evt_id, 105u);
    EXPECT_EQ(pParty->GetGold(), goldBefore);
    EXPECT_EQ(pParty->GetPartyReputation(), reputationBefore);
    leaveDialogue(game);

    // Thomas exposes the return topic immediately, but refuses before the golem is fully assembled.
    openThomasGrey(game);
    ASSERT_TRUE(selectNpcEvent(game, &thomas, 93));
    EXPECT_TRUE(pParty->_questBits[QBIT_GOLEM_ACTIVE]);
    EXPECT_FALSE(pParty->_questBits[QBIT_GOLEM_ANIMATED]);
    EXPECT_EQ(thomas.dialogue_1_evt_id, 93u);
    EXPECT_EQ(pParty->GetGold(), goldBefore);
    EXPECT_EQ(pParty->GetPartyReputation(), reputationBefore);
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].classType, classBefore[i]);
        EXPECT_EQ(pParty->pCharacters[i].experience, experienceBefore[i]);
        EXPECT_EQ(pParty->pCharacters[i]._achievedAwardsBits, awardsBefore[i]);
    }
    leaveDialogue(game);

    for (const GolemPartSource &source : commonGolemParts)
        acquireGolemPart(game, source);
    const GolemPartSource &head = dentedHead ? dentedGolemHead : goodGolemHead;
    acquireGolemPart(game, head);

    for (const GolemPartSource &source : commonGolemParts)
        installGolemPart(game, source);
    installGolemPart(game, head);
    EXPECT_FALSE(pParty->_questBits[QBIT_GOLEM_ASSEMBLED]);
    EXPECT_FALSE(pParty->_questBits[QBIT_ABBEY_NORMAL_GOLEM_ASSEMBLED]);
    EXPECT_EQ(golem.dialogue_2_evt_id, 105u);

    // Installing the sixth part is not enough: the authored Golem Parts topic requires one extra confirmation.
    openGolemDialogue(game);
    ASSERT_TRUE(selectNpcEvent(game, getNPCData(speakingNpcId), 104));
    EXPECT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
    EXPECT_EQ(golem.dialogue_2_evt_id, 0u);
    EXPECT_EQ(pParty->_questBits[QBIT_GOLEM_ASSEMBLED], !dentedHead);
    EXPECT_EQ(pParty->_questBits[QBIT_ABBEY_NORMAL_GOLEM_ASSEMBLED], dentedHead);
    leaveDialogue(game);

    openThomasGrey(game);
    ASSERT_TRUE(selectNpcEvent(game, &thomas, 93));
    EXPECT_FALSE(pParty->_questBits[QBIT_GOLEM_ACTIVE]);
    EXPECT_TRUE(pParty->_questBits[QBIT_GOLEM_ANIMATED]);
    EXPECT_EQ(thomas.dialogue_1_evt_id, 94u);
    EXPECT_EQ(golem.greetingIndex, 84);
    EXPECT_EQ(pParty->GetGold(), goldBefore);
    EXPECT_EQ(pParty->GetPartyReputation() - reputationBefore, 0);
    EXPECT_EQ(pParty->pCharacters[0].classType, CLASS_WIZARD);
    auto expectedAwards = awardsBefore[0];
    expectedAwards.set(AWARD_PROMOTION_WIZARD);
    EXPECT_EQ(pParty->pCharacters[0]._achievedAwardsBits, expectedAwards);
    EXPECT_EQ(pParty->pCharacters[0].experience - experienceBefore[0], 30000);
    for (int i = 1; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].classType, classBefore[i]);
        expectedAwards = awardsBefore[i];
        expectedAwards.set(AWARD_PROMOTION_WIZARD_HONORARY);
        EXPECT_EQ(pParty->pCharacters[i]._achievedAwardsBits, expectedAwards);
        EXPECT_EQ(pParty->pCharacters[i].experience - experienceBefore[i], 15000);
    }
    for (const GolemPartSource &source : commonGolemParts)
        EXPECT_TRUE(pParty->_questBits[source.placedBit]);
    EXPECT_TRUE(pParty->_questBits[head.placedBit]);
    EXPECT_TRUE(pParty->_questBits[dentedHead ? QBIT_ABBEY_NORMAL_GOLEM_ASSEMBLED : QBIT_GOLEM_ASSEMBLED]);
    EXPECT_FALSE(pParty->_questBits[QBIT_ORACLE_RECOVERS_GOLEM_HEAD]);
    EXPECT_FALSE(pParty->_questBits[QBIT_ORACLE_RECOVERS_ABBEY_NORMAL_GOLEM_HEAD]);

    leaveDialogue(game);
    const std::array experienceAfter = {pParty->pCharacters[0].experience, pParty->pCharacters[1].experience,
                                        pParty->pCharacters[2].experience, pParty->pCharacters[3].experience};
    const std::array awardsAfter = {pParty->pCharacters[0]._achievedAwardsBits,
                                    pParty->pCharacters[1]._achievedAwardsBits,
                                    pParty->pCharacters[2]._achievedAwardsBits,
                                    pParty->pCharacters[3]._achievedAwardsBits};
    openThomasGrey(game);
    EXPECT_FALSE(npcEventVisible(&thomas, 93));
    EXPECT_FALSE(selectNpcEvent(game, &thomas, 93));
    EXPECT_EQ(pParty->GetGold(), goldBefore);
    EXPECT_EQ(pParty->GetPartyReputation(), reputationBefore);
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].experience, experienceAfter[i]);
        EXPECT_EQ(pParty->pCharacters[i]._achievedAwardsBits, awardsAfter[i]);
    }
    leaveDialogue(game);

    // The animated good-head golem is enabled as a friendly guard; the dented-head version is hostile.
    engine->config->debug.NoActors.setValue(false);
    game.teleportTo(MAP_CASTLE_HARMONDALE, Vec3f(), 0);
    auto actor = std::ranges::find(pActors, 6, &Actor::group);
    ASSERT_NE(actor, pActors.end());
    EXPECT_FALSE(actor->attributes & ACTOR_UNKNOW11);
    EXPECT_EQ(static_cast<bool>(actor->attributes & ACTOR_AGGRESSOR), dentedHead);
    EXPECT_NE(actor->aiState, Disabled);
    EXPECT_FALSE(golem.flags & NPC_HIRED);
}

} // namespace

GAME_TEST(MM7WizardPromotion, CastleGolemEventWaitsForAnimation) {
    game.startNewGame();
    game.teleportTo(MAP_CASTLE_HARMONDALE, Vec3f(), 0);
    ASSERT_TRUE(engine->_localEventMap.hasEvent(376));
    ASSERT_FALSE(golemActorStates().empty());

    pNPCStats->pNPCData[56].flags |= NPC_HIRED;
    pParty->CountHirelings();
    pParty->_questBits.set(QBIT_GOLEM_ASSEMBLED);
    ASSERT_FALSE(pParty->_questBits[QBIT_GOLEM_ANIMATED]);
    const std::vector<GolemActorState> before = golemActorStates();

    runEvent(EvtContext(EVT_SOURCE_MAP, 376), 1);

    EXPECT_EQ(golemActorStates(), before);
    EXPECT_TRUE(pNPCStats->pNPCData[56].flags & NPC_HIRED);
}

GAME_TEST(MM7WizardPromotion, CastleGolemGuardIgnoresPatchedEventShape) {
    game.startNewGame();
    game.teleportTo(MAP_CASTLE_HARMONDALE, Vec3f(), 0);
    ASSERT_FALSE(pParty->_questBits[QBIT_GOLEM_ANIMATED]);

    engine->_localEventMap.clear();
    EvtInstruction trigger(EVENT_OnMapReload);
    trigger.step = 0;
    engine->_localEventMap.add(376, trigger);
    EvtInstruction marker(EVENT_Set);
    marker.step = 1;
    marker.data.variable_descr.type = VAR_MapPersistentVariable_0;
    marker.data.variable_descr.value = 1;
    engine->_localEventMap.add(376, marker);
    EvtInstruction animationGate(EVENT_Compare);
    animationGate.step = 2;
    animationGate.target_step = 4;
    animationGate.data.variable_descr.type = VAR_QBits_QuestsDone;
    animationGate.data.variable_descr.value = std::to_underlying(QBIT_GOLEM_ANIMATED);
    engine->_localEventMap.add(376, animationGate);
    EvtInstruction stop(EVENT_Exit);
    stop.step = 3;
    engine->_localEventMap.add(376, stop);
    engine->_persistentVariables.mapVars[0] = 0;

    runEvent(EvtContext(EVT_SOURCE_MAP, 376), 1);

    EXPECT_EQ(engine->_persistentVariables.mapVars[0], 1);
}

GAME_TEST(MM7WizardPromotion, GoodHeadPromotesSorcererAndCreatesFriendlyGolem) {
    runWizardPromotion(game, false);
}

GAME_TEST(MM7WizardPromotion, DentedHeadPromotesSorcererAndCreatesHostileGolem) {
    runWizardPromotion(game, true);
}
