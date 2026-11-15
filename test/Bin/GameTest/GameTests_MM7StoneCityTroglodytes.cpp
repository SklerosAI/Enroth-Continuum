#include <algorithm>
#include <array>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Core/Time/Time.h"

#include "Engine/Data/AwardEnums.h"
#include "Engine/Data/HouseEnums.h"
#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/MM7/MM7AtomicSaveFile.h"
#include "Engine/MM7/MM7SaveValidation.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/MonsterEnumFunctions.h"
#include "Engine/Objects/Monsters.h"
#include "Engine/Party.h"
#include "Engine/PartyEnums.h"
#include "Engine/Resources/EngineFileSystem.h"
#include "Engine/Resources/LOD.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/NPCTable.h"

#include "GUI/GUIWindow.h"
#include "GUI/UI/NPCTopics.h"
#include "GUI/UI/UIHouses.h"
#include "GUI/UI/UISaveLoad.h"

#include "Utility/Exception.h"

#include "MM7CombatHarness.h"
#include "MM7GameplayHarness.h"

namespace {

constexpr std::string_view checkpointSlot = "saves/save000.mm7";
constexpr std::string_view checkpointFileName = "save000.mm7";
constexpr std::array<uint64_t, 4> checkpointExperienceBaseline = {9000, 9100, 9200, 9300};
constexpr int checkpointGoldBaseline = 10000;

int troglodyteQueenExperiencePerCharacter() {
    return pMonsterStats->infos[MONSTER_TROGLODYTE_C].exp / 4;
}

bool eventChangesQuestBit(const EvtInstruction &instruction, EvtOpcode opcode, QuestBit bit) {
    return instruction.opcode == opcode && instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
           instruction.data.variable_descr.value == std::to_underlying(bit);
}

unsigned int findNpcEvent(const NPCData *npc, auto predicate) {
    const std::array eventIds = {npc->dialogue_1_evt_id, npc->dialogue_2_evt_id, npc->dialogue_3_evt_id,
                                 npc->dialogue_4_evt_id, npc->dialogue_5_evt_id, npc->dialogue_6_evt_id};
    auto event = std::ranges::find_if(eventIds, [predicate](unsigned int eventId) {
        return eventId != 0 && engine->_globalEventMap.hasEvent(eventId) &&
               std::ranges::any_of(engine->_globalEventMap.function(eventId), predicate);
    });
    EXPECT_NE(event, eventIds.end());
    return event == eventIds.end() ? 0 : *event;
}

bool selectNpcEvent(EngineController &game, NPCData *npc, unsigned int eventId) {
    const std::array eventIds = {npc->dialogue_1_evt_id, npc->dialogue_2_evt_id, npc->dialogue_3_evt_id,
                                 npc->dialogue_4_evt_id, npc->dialogue_5_evt_id, npc->dialogue_6_evt_id};
    auto event = std::ranges::find(eventIds, eventId);
    if (event == eventIds.end())
        return false;
    DialogueId line = static_cast<DialogueId>(std::to_underlying(DIALOGUE_SCRIPTED_LINE_1) +
                                               (event - eventIds.begin()));
    const std::vector<DialogueId> topics = prepareScriptedNPCDialogueTopics(npc);
    auto option = std::ranges::find(topics, line);
    if (option == topics.end())
        return false;
    game.pressGuiButton(fmt::format("HouseNpcDialogue_Option{}", option - topics.begin()));
    game.tick();
    return true;
}

NPCData *findSpark() {
    auto npc = std::ranges::find(pNPCStats->pNPCData, HOUSE_STONE_CITY_SPARKS_HOUSE, &NPCData::house);
    EXPECT_NE(npc, pNPCStats->pNPCData.end());
    return npc == pNPCStats->pNPCData.end() ? nullptr : &*npc;
}

void openSparkDialogue(EngineController &game, NPCData *spark) {
    // Setup teleport: the house itself is entered with the authored viewport interaction.
    game.teleportTo(MAP_STONE_CITY, Vec3f(3904, -3334, -5), 270);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick(2);
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_NE(window_SpeakInHouse, nullptr);
    ASSERT_EQ(window_SpeakInHouse->houseId(), HOUSE_STONE_CITY_SPARKS_HOUSE);
    auto entry = std::ranges::find(houseNpcs, spark, &HouseNpcDesc::npc);
    ASSERT_NE(entry, houseNpcs.end());
    game.pressGuiButton(fmt::format("House_Npc{}", entry - houseNpcs.begin()));
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
}

void leaveSparkDialogue(EngineController &game) {
    for (int attempt = 0; attempt < 4 && current_screen_type != SCREEN_GAME; ++attempt) {
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
    }
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
}

bool isTroglodyte(const Actor &actor) {
    return monsterTypeForMonsterId(actor.monsterInfo.id) == MONSTER_TYPE_TROGLODYTE;
}

bool isAlive(const Actor &actor) {
    return actor.aiState != Dead && actor.aiState != Dying && actor.aiState != Removed && actor.aiState != Disabled;
}

float horizontalDistance(const Vec3f &left, const Vec3f &right) {
    return (left - right).xy().length();
}

int placeForMeasuredApproach(EngineController &game) {
    for (Actor &actor : pActors) {
        if (actor.monsterInfo.id != MONSTER_TROGLODYTE_C || !isAlive(actor))
            continue;
        for (const auto &[offset, yaw] : std::array{
                 std::pair{Vec3f(0, -400, 0), 90}, std::pair{Vec3f(-400, 0, 0), 0},
                 std::pair{Vec3f(0, 400, 0), 270}, std::pair{Vec3f(400, 0, 0), 180}}) {
            Vec3f position = actor.pos + offset;
            int sectorId = pIndoor->GetSector(position);
            int floorFaceId = -1;
            float floor = sectorId ? BLV_GetFloorLevel(position + Vec3f(0, 0, pParty->radius),
                                                       sectorId, &floorFaceId) : -30000;
            if (floor == -30000 || floorFaceId == -1)
                continue;
            position.z = floor;
            for (int pitch : {-20, 0, 20}) {
                game.teleportTo(MAP_STONE_CITY, position, yaw, pitch);
                try {
                    game.pointMouseAtActor(actor.id);
                    return actor.id;
                } catch (const Exception &) {
                }
            }
        }
    }
    return -1;
}

void toughenPartyForTroglodyte() {
    for (Character &character : pParty->pCharacters) {
        character._stats[ATTRIBUTE_SPEED] = 200;
        character._stats[ATTRIBUTE_ACCURACY] = 200;
        character._stats[ATTRIBUTE_ENDURANCE] = 200;
        character.uLevel = 200;
        character._melee_dmg_bonus = 100;
        character.sResBodyBase = 1000;
        character.sResPhysicalBase = 1000;
        character.health = 100000;
    }
}

void killOneTroglodyteWithMovementAndAttackInput(EngineController &game, int *targetId) {
    toughenPartyForTroglodyte();
    *targetId = placeForMeasuredApproach(game);
    ASSERT_GE(*targetId, 0);
    Actor &target = pActors[*targetId];
    ASSERT_EQ(target.monsterInfo.id, MONSTER_TROGLODYTE_C);
    ASSERT_TRUE(isAlive(target));
    target.hp = 1; // Combat fixture: the one authored target is still killed through attack input below.
    target.moveSpeed = 1;
    target.buffs[ACTOR_BUFF_PARALYZED].Apply(
        pParty->GetPlayingTime() + Duration::fromDays(1), MASTERY_GRANDMASTER, 0, 0, 0);
    target.aiState = Paralyzed;
    target.attributes &= ~ACTOR_FLEEING;
    game.tick(5); // Let the authored placement settle on its floor before measuring immobilization.
    ASSERT_TRUE(target.buffs[ACTOR_BUFF_PARALYZED].Active());
    const Vec3f targetPosition = target.pos;

    // Setup teleporting ends here. This measured approach and the fight below use input only.
    const float initialDistance = horizontalDistance(pParty->pos, target.pos);
    mm7::testing::MM7MovementResult movement = mm7::testing::driveMovementInput(game, {
        .key = PlatformKey::KEY_UP,
        .heldTicks = 2,
        .minimumHorizontalDistance = 15,
    });
    const float movedDistance = horizontalDistance(pParty->pos, target.pos);
    const Vec3f releasedPosition = pParty->pos;
    game.tick(3);
    const float afterReleaseDrift = horizontalDistance(releasedPosition, pParty->pos);
    EXPECT_EQ(target.pos, targetPosition);

    mm7::testing::CombatResult combat = mm7::testing::fightActorWithAttackInput(
        game, *targetId, [&] {
            try {
                game.pointMouseAtActor(*targetId);
                return true;
            } catch (const Exception &) {
                return false;
            }
        }, 20, 1, 2);
    const std::string diagnostic = fmt::format(
        "distance={:.1f}->{:.1f}, release_drift={:.1f}; movement=[{}]; combat=[{}]",
        initialDistance, movedDistance, afterReleaseDrift, movement.diagnostic(),
        mm7::testing::describeCombatResult(combat));

    EXPECT_TRUE(movement.pressPosted) << diagnostic;
    EXPECT_TRUE(movement.releasePosted) << diagnostic;
    EXPECT_TRUE(movement.releaseProcessed) << diagnostic;
    EXPECT_LT(movedDistance, initialDistance) << diagnostic;
    EXPECT_LT(movement.postReleaseHorizontalDistance(), 1) << diagnostic;
    EXPECT_LT(afterReleaseDrift, 1) << diagnostic;
    EXPECT_EQ(movement.startMap, MAP_STONE_CITY) << diagnostic;
    EXPECT_EQ(movement.endMap, MAP_STONE_CITY) << diagnostic;
    EXPECT_TRUE(combat.targetAcquired) << diagnostic;
    EXPECT_FALSE(combat.targetingFailed) << diagnostic;
    EXPECT_TRUE(combat.observedHpDecrease) << diagnostic;
    EXPECT_TRUE(combat.reachedTerminalDeath) << diagnostic;
    EXPECT_TRUE(combat.reachedDeadState) << diagnostic;
    EXPECT_EQ(combat.initialMap, MAP_STONE_CITY) << diagnostic;
    EXPECT_EQ(combat.finalMap, MAP_STONE_CITY) << diagnostic;
}

int fixtureRemainingTroglodytesExceptOne(int realTargetId) {
    int survivorId = -1;
    for (Actor &actor : pActors) {
        if (!isTroglodyte(actor) || actor.id == realTargetId || !isAlive(actor))
            continue;
        if (survivorId == -1) {
            survivorId = actor.id;
            continue;
        }
        Actor::Die(actor.id); // Fixture only the repetitive remainder after one authored input-driven kill.
    }
    return survivorId;
}

int aliveTroglodytes() {
    return std::ranges::count_if(pActors, [](const Actor &actor) {
        return isTroglodyte(actor) && isAlive(actor);
    });
}

void runStoneCityTroglodyteRoute(EngineController &game, bool useCheckpointBaseline = false) {
    engine->config->debug.NoDamage.setValue(true);
    game.startNewGame();
    if (useCheckpointBaseline) {
        pParty->SetGold(checkpointGoldBaseline);
        for (int i = 0; i < 4; ++i)
            pParty->pCharacters[i].experience = checkpointExperienceBaseline[i];
    }
    NPCData *spark = findSpark();
    ASSERT_NE(spark, nullptr);
    const unsigned int offerEvent = findNpcEvent(spark, [](const EvtInstruction &instruction) {
        return eventChangesQuestBit(instruction, EVENT_Set, QBIT_STONE_CITY_TROGLODYTES_ACTIVE) ||
               eventChangesQuestBit(instruction, EVENT_Add, QBIT_STONE_CITY_TROGLODYTES_ACTIVE);
    });
    ASSERT_NE(offerEvent, 0u);

    openSparkDialogue(game, spark);
    ASSERT_TRUE(selectNpcEvent(game, spark, offerEvent));
    ASSERT_TRUE(pParty->_questBits[QBIT_STONE_CITY_TROGLODYTES_ACTIVE]);
    const unsigned int returnEvent = findNpcEvent(spark, [](const EvtInstruction &instruction) {
        return eventChangesQuestBit(instruction, EVENT_Subtract, QBIT_STONE_CITY_TROGLODYTES_ACTIVE);
    });
    ASSERT_NE(returnEvent, 0u);
    leaveSparkDialogue(game);

    int realTargetId = -1;
    killOneTroglodyteWithMovementAndAttackInput(game, &realTargetId);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    ASSERT_FALSE(::testing::Test::HasFailure());
    const int survivorId = fixtureRemainingTroglodytesExceptOne(realTargetId);
    ASSERT_GE(survivorId, 0);
    ASSERT_EQ(aliveTroglodytes(), 1);

    const int goldBeforeReward = pParty->GetGold();
    const int reputationBeforeReward = pParty->GetPartyReputation();
    std::array<uint64_t, 4> experienceBeforeReward;
    for (int i = 0; i < 4; ++i)
        experienceBeforeReward[i] = pParty->pCharacters[i].experience;

    openSparkDialogue(game, spark);
    ASSERT_TRUE(selectNpcEvent(game, spark, returnEvent));
    EXPECT_TRUE(pParty->_questBits[QBIT_STONE_CITY_TROGLODYTES_ACTIVE]);
    EXPECT_EQ(pParty->GetGold(), goldBeforeReward);
    EXPECT_EQ(pParty->GetPartyReputation(), reputationBeforeReward);
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].experience, experienceBeforeReward[i]);
        EXPECT_FALSE(pParty->pCharacters[i]._achievedAwardsBits[
            AWARD_BARROW_DOWNS_KILLED_TROGLODYTES_IN_LOWER_CITY]);
    }
    leaveSparkDialogue(game);

    Actor::Die(survivorId); // Final repetition fixture after the authored survivor-refusal branch.
    ASSERT_EQ(aliveTroglodytes(), 0);
    openSparkDialogue(game, spark);
    ASSERT_TRUE(selectNpcEvent(game, spark, returnEvent));
    EXPECT_FALSE(pParty->_questBits[QBIT_STONE_CITY_TROGLODYTES_ACTIVE]);
    EXPECT_EQ(pParty->GetGold(), goldBeforeReward + 2500);
    EXPECT_EQ(pParty->GetPartyReputation(), reputationBeforeReward - 10);
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].experience, experienceBeforeReward[i] + 5000);
        EXPECT_TRUE(pParty->pCharacters[i]._achievedAwardsBits[
            AWARD_BARROW_DOWNS_KILLED_TROGLODYTES_IN_LOWER_CITY]);
    }
    leaveSparkDialogue(game);
}

void assertDisposableHeadlessProfile() {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM7);
    const auto profileRoot = mm7::startupNativeProfileRoot();
    ASSERT_TRUE(profileRoot);
    ASSERT_EQ(*profileRoot, std::filesystem::canonical(std::filesystem::path(ufs->displayPath(""))));
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
}

}  // namespace

GAME_TEST(MM7StoneCityTroglodytes, MovementCombatRefusalAndReward) {
    runStoneCityTroglodyteRoute(game);
}

GAME_TEST(MM7SideQuestCheckpoint, DISABLED_WriteStoneCityTroglodytes) {
    assertDisposableHeadlessProfile();
    ASSERT_FALSE(ufs->exists(checkpointSlot));
    runStoneCityTroglodyteRoute(game, true);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    ASSERT_FALSE(::testing::Test::HasFailure());
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_STONE_CITY);
    game.runGameRoutine([] {
        saveGame(false, false, checkpointSlot, "Stone City troglodytes checkpoint");
    });

    ASSERT_TRUE(ufs->exists(checkpointSlot));
    const Blob saved = ufs->read(checkpointSlot);
    ASSERT_GT(saved.size(), 1024u);
    ASSERT_TRUE(mm7::preflightNativeSaveStructure(
        std::span<const std::uint8_t>(static_cast<const std::uint8_t *>(saved.data()), saved.size()),
        GameTitle::MM7, *pGames_LOD));
}

GAME_TEST(MM7SideQuestCheckpoint, DISABLED_ReloadStoneCityTroglodytes) {
    assertDisposableHeadlessProfile();
    ASSERT_TRUE(ufs->exists(checkpointSlot));
    const Blob saved = Blob::copy(ufs->read(checkpointSlot));
    ASSERT_GT(saved.size(), 1024u);

    game.pressGuiButton("MainMenu_LoadGame");
    game.tick(3);
    ASSERT_EQ(current_screen_type, SCREEN_LOADGAME);
    const auto &slots = saveLoadMenu()->slots();
    const auto found = std::ranges::find(slots, checkpointFileName, &SavegameSlot::fileName);
    ASSERT_NE(found, slots.end());
    const int index = static_cast<int>(found - slots.begin());
    ASSERT_LT(index, 7);
    game.pressGuiButton(fmt::format("LoadMenu_Slot{}", index));
    game.tick(2);
    ASSERT_EQ(saveLoadMenu()->selectedSlot().fileName, checkpointFileName);
    game.pressGuiButton("LoadMenu_Load");
    game.skipLoadingScreen();
    game.tick(2);

    ASSERT_EQ(current_screen_type, SCREEN_GAME);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_STONE_CITY);
    ASSERT_EQ(engine->_lastLoadedSaveFileName, checkpointFileName);
    EXPECT_FALSE(pParty->_questBits[QBIT_STONE_CITY_TROGLODYTES_ACTIVE]);
    EXPECT_EQ(pParty->GetGold(), checkpointGoldBaseline + 2500);
    EXPECT_EQ(pParty->GetPartyReputation(), -10);
    EXPECT_EQ(aliveTroglodytes(), 0);
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].experience,
                  checkpointExperienceBaseline[i] + troglodyteQueenExperiencePerCharacter() + 5000);
        EXPECT_TRUE(pParty->pCharacters[i]._achievedAwardsBits[
            AWARD_BARROW_DOWNS_KILLED_TROGLODYTES_IN_LOWER_CITY]);
    }
    EXPECT_TRUE(ufs->read(checkpointSlot).str() == saved.str());
}
