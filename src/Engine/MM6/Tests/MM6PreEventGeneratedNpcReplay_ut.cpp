#include <deque>
#include <memory>
#include <stdexcept>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6PreEventGeneratedNpcReplay.h"
#include "Engine/Objects/MonsterEnumFunctions.h"
#include "Library/Random/MersenneTwisterRandomEngine.h"

namespace {

Actor actor(int slot, MonsterId monster, int npcId) {
    Actor result(slot);
    result.monsterInfo.id = monster;
    result.npcId = npcId;
    return result;
}

void generatedRow(NPCStats *npcs, size_t index, MonsterId monster) {
    NPCData &row = npcs->pAdditionalNPC[index];
    row.name = "Generated";
    row.portraitId = 300;
    row.canJoin = true;
    row.field_24 = 1;
    row.house = HOUSE_INVALID;
    row.sex = sexForMonsterType(monsterTypeForMonsterId(monster));
    row.profession = NPC_PROFESSION_SMITH;
}

constexpr MonsterId streetPeasantA = static_cast<MonsterId>(121);
constexpr MonsterId streetPeasantB = static_cast<MonsterId>(134);
constexpr MonsterId mm7Peasant = static_cast<MonsterId>(137); // In MM6 this ID isn't a peasant, MM7's ID ranges say it is.

GameplayRandomState rngState() {
    MersenneTwisterRandomEngine rng;
    rng.seed(37);
    (void) rng.random(100);
    GameplayRandomState result;
    result.mtState = rng.stateText();
    return result;
}

} // namespace

UNIT_TEST(MM6PreEventGeneratedNpcReplay, StagesExactEligibleAnchorsWithoutMutation) {
    auto savedNpcs = std::make_unique<NPCStats>();
    generatedRow(savedNpcs.get(), 0, streetPeasantA);
    generatedRow(savedNpcs.get(), 1, streetPeasantB);
    savedNpcs->uNewlNPCBufPos = 2;
    std::deque<Actor> finalActors{
        actor(0, streetPeasantB, 0),
        actor(1, streetPeasantA, 5000), actor(2, MONSTER_ANGEL_B, 0),
        actor(3, mm7Peasant, 0), actor(4, streetPeasantB, 5001)};
    const auto saved = mm6::captureGeneratedNpcFields(
        finalActors, *savedNpcs, "oute3.odm", GameTitle::MM6);
    finalActors[1].npcId = 1;
    finalActors[4].npcId = -1;
    auto freshNpcs = std::make_unique<NPCStats>();
    const auto rng = rngState();
    const auto plan = mm6::stagePreEventGeneratedNpcReplay(
        saved, rng, finalActors, *freshNpcs, "oute3.odm",
        RANDOM_ENGINE_MERSENNE_TWISTER, GameTitle::MM6);
    EXPECT_EQ(plan.generatedCount, 2u);
    EXPECT_EQ(plan.generated.rows[0].npc.name, "Generated");
    EXPECT_EQ(plan.gameplayRng, rng);
    EXPECT_EQ(finalActors[1].npcId, 1);
    EXPECT_EQ(freshNpcs->uNewlNPCBufPos, 0);
}

UNIT_TEST(MM6PreEventGeneratedNpcReplay, KeepsClearedActorOutsideGeneratedRows) {
    auto savedNpcs = std::make_unique<NPCStats>();
    generatedRow(savedNpcs.get(), 0, streetPeasantB);
    savedNpcs->uNewlNPCBufPos = 1;
    std::deque<Actor> finalActors{actor(0, mm7Peasant, 0),
                                  actor(1, streetPeasantB, 5000)};
    const auto saved = mm6::captureGeneratedNpcFields(
        finalActors, *savedNpcs, "oute3.odm", GameTitle::MM6);
    finalActors[0].npcId = -1;
    finalActors[1].npcId = 2;
    auto freshNpcs = std::make_unique<NPCStats>();
    const auto plan = mm6::stagePreEventGeneratedNpcReplay(
        saved, rngState(), finalActors, *freshNpcs, "oute3.odm",
        RANDOM_ENGINE_MERSENNE_TWISTER, GameTitle::MM6);
    EXPECT_EQ(plan.generatedCount, 1u);
    EXPECT_EQ(finalActors[0].npcId, -1);
    EXPECT_EQ(finalActors[1].npcId, 2);
}

UNIT_TEST(MM6PreEventGeneratedNpcReplay, RefusesRespawnAndWrongLoadContext) {
    auto savedNpcs = std::make_unique<NPCStats>();
    generatedRow(savedNpcs.get(), 0, streetPeasantB);
    savedNpcs->uNewlNPCBufPos = 1;
    std::deque<Actor> actors{actor(0, streetPeasantB, 5000)};
    const auto saved = mm6::captureGeneratedNpcFields(
        actors, *savedNpcs, "oute3.odm", GameTitle::MM6);
    actors[0].npcId = 1;
    auto freshNpcs = std::make_unique<NPCStats>();
    const auto rng = rngState();
    actors.push_back(actor(1, streetPeasantB, 1));
    EXPECT_THROW((void) mm6::stagePreEventGeneratedNpcReplay(
        saved, rng, actors, *freshNpcs, "oute3.odm",
        RANDOM_ENGINE_MERSENNE_TWISTER, GameTitle::MM6), std::exception);
    actors.pop_back();
    EXPECT_THROW((void) mm6::stagePreEventGeneratedNpcReplay(
        saved, rng, actors, *freshNpcs, "wrong.odm",
        RANDOM_ENGINE_MERSENNE_TWISTER, GameTitle::MM6), std::exception);
    EXPECT_THROW((void) mm6::stagePreEventGeneratedNpcReplay(
        saved, rng, actors, *freshNpcs, "oute3.odm",
        RANDOM_ENGINE_SEQUENTIAL, GameTitle::MM6), std::exception);
    freshNpcs->uNewlNPCBufPos = 1;
    EXPECT_THROW((void) mm6::stagePreEventGeneratedNpcReplay(
        saved, rng, actors, *freshNpcs, "oute3.odm",
        RANDOM_ENGINE_MERSENNE_TWISTER, GameTitle::MM6), std::exception);
    EXPECT_EQ(actors[0].npcId, 1);
}
