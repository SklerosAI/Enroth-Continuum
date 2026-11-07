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
    constexpr MonsterId peasantB = MONSTER_PEASANT_DWARF_FEMALE_A_B;
    constexpr MonsterId peasantC = MONSTER_PEASANT_DWARF_FEMALE_A_C;
    auto savedNpcs = std::make_unique<NPCStats>();
    generatedRow(savedNpcs.get(), 0, peasantB);
    generatedRow(savedNpcs.get(), 1, peasantC);
    savedNpcs->uNewlNPCBufPos = 2;
    std::deque<Actor> finalActors{
        actor(0, MONSTER_PEASANT_DWARF_FEMALE_A_A, 0),
        actor(1, peasantB, 5000), actor(2, MONSTER_ANGEL_B, 0),
        actor(3, peasantB, 12), actor(4, peasantC, 5001)};
    const auto saved = mm6::captureGeneratedNpcFields(
        finalActors, *savedNpcs, "oute3.odm", GameTitle::MM6);
    finalActors[1].npcId = 0;
    finalActors[4].npcId = 0;
    auto freshNpcs = std::make_unique<NPCStats>();
    const auto rng = rngState();
    const auto plan = mm6::stagePreEventGeneratedNpcReplay(
        saved, rng, finalActors, *freshNpcs, "oute3.odm",
        RANDOM_ENGINE_MERSENNE_TWISTER, GameTitle::MM6);
    EXPECT_EQ(plan.generatedCount, 2u);
    EXPECT_EQ(plan.generated.rows[0].npc.name, "Generated");
    EXPECT_EQ(plan.gameplayRng, rng);
    EXPECT_EQ(finalActors[1].npcId, 0);
    EXPECT_EQ(freshNpcs->uNewlNPCBufPos, 0);
}

UNIT_TEST(MM6PreEventGeneratedNpcReplay, RefusesRespawnAndWrongLoadContext) {
    constexpr MonsterId peasantB = MONSTER_PEASANT_DWARF_FEMALE_A_B;
    auto savedNpcs = std::make_unique<NPCStats>();
    generatedRow(savedNpcs.get(), 0, peasantB);
    savedNpcs->uNewlNPCBufPos = 1;
    std::deque<Actor> actors{actor(0, peasantB, 5000)};
    const auto saved = mm6::captureGeneratedNpcFields(
        actors, *savedNpcs, "oute3.odm", GameTitle::MM6);
    actors[0].npcId = 0;
    auto freshNpcs = std::make_unique<NPCStats>();
    const auto rng = rngState();
    actors.push_back(actor(1, peasantB, 0));
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
    EXPECT_EQ(actors[0].npcId, 0);
}
