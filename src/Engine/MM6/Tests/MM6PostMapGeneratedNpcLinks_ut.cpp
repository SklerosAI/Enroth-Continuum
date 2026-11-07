#include <array>
#include <memory>
#include <stdexcept>
#include <deque>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6PostMapGeneratedNpcLinks.h"
#include "Engine/MM6/MM6NativeActorNpcPolicy.h"
#include "Engine/Objects/MonsterEnumFunctions.h"

namespace {

Actor actor(int index, MonsterId monster, int npcId) {
    Actor result(index);
    result.monsterInfo.id = monster;
    result.npcId = npcId;
    return result;
}

void generatedRow(NPCStats *npcs, int index, MonsterId monster) {
    NPCData &row = npcs->pAdditionalNPC[index];
    row.name = "Generated";
    row.portraitId = 300;
    row.canJoin = true;
    row.field_24 = 1;
    row.house = HOUSE_INVALID;
    row.sex = sexForMonsterType(monsterTypeForMonsterId(monster));
    row.profession = NPC_PROFESSION_SMITH;
}

} // namespace

UNIT_TEST(MM6PostMapGeneratedNpcLinks, PreMapPoolCapacityRejectsBeforeFirstWrite) {
    constexpr MonsterId peasantB = MONSTER_PEASANT_DWARF_FEMALE_A_B;
    auto npcs = std::make_unique<NPCStats>();
    npcs->uNewlNPCBufPos = 7;
    npcs->pAdditionalNPC[0].name = "Prior";
    std::deque<Actor> actors;
    for (int index = 0; index < 100; ++index)
        actors.push_back(actor(index, peasantB, 0));
    EXPECT_EQ(mm6::requirePreMapGeneratedNpcCapacity(actors, *npcs, GameTitle::MM6), 100u);
    actors.push_back(actor(100, peasantB, 0));
    EXPECT_THROW((void) mm6::requirePreMapGeneratedNpcCapacity(
        actors, *npcs, GameTitle::MM6), std::exception);
    EXPECT_EQ(npcs->uNewlNPCBufPos, 7);
    EXPECT_EQ(npcs->pAdditionalNPC[0].name, "Prior");
    actors.back().npcId = 12;
    EXPECT_EQ(mm6::requirePreMapGeneratedNpcCapacity(actors, *npcs, GameTitle::MM6), 100u);
    EXPECT_THROW((void) mm6::requirePreMapGeneratedNpcCapacity(
        actors, *npcs, GameTitle::MM7), std::exception);
}

UNIT_TEST(MM6PostMapGeneratedNpcLinks, AcceptsContiguousGeneratedActorsAndFixedRows) {
    constexpr MonsterId peasantB = MONSTER_PEASANT_DWARF_FEMALE_A_B;
    constexpr MonsterId peasantC = MONSTER_PEASANT_DWARF_FEMALE_A_C;
    auto npcs = std::make_unique<NPCStats>();
    generatedRow(npcs.get(), 0, peasantB);
    generatedRow(npcs.get(), 1, peasantC);
    npcs->uNewlNPCBufPos = 2;
    const std::vector<Actor> actors{
        actor(0, MONSTER_PEASANT_DWARF_FEMALE_A_A, 0),
        actor(1, peasantB, 5000),
        actor(2, peasantC, 5001),
        actor(3, peasantB, 12),
        actor(4, MONSTER_ANGEL_B, 0)};
    EXPECT_EQ(mm6::requirePostMapGeneratedNpcLinks(actors, *npcs, GameTitle::MM6), 2u);
    EXPECT_EQ(actors[1].npcId, 5000);
    EXPECT_EQ(npcs->pAdditionalNPC[0].name, "Generated");
}

UNIT_TEST(MM6PostMapGeneratedNpcLinks, NativeSignedMarkerClearsAfterMapLoad) {
    constexpr MonsterId peasantB = MONSTER_PEASANT_DWARF_FEMALE_A_B;
    auto npcs = std::make_unique<NPCStats>();
    generatedRow(npcs.get(), 0, peasantB);
    npcs->uNewlNPCBufPos = 1;
    std::vector<Actor> actors{actor(0, peasantB, -1),
                              actor(1, peasantB, 5000)};
    const std::deque<Actor> source{actor(0, peasantB, -1),
                                   actor(1, peasantB, 0)};
    EXPECT_EQ(mm6::requirePreMapGeneratedNpcCapacity(
        source, *npcs, GameTitle::MM6), 1u);
    EXPECT_TRUE(mm6::clearNoneligibleSignedActorNpc(peasantB, -1));
    EXPECT_FALSE(mm6::clearNoneligibleSignedActorNpc(peasantB, 0));
    EXPECT_FALSE(mm6::clearNoneligibleSignedActorNpc(static_cast<MonsterId>(40), -1));
    EXPECT_THROW((void) mm6::requirePreMapGeneratedNpcCapacity(
        std::deque<Actor>{actor(0, static_cast<MonsterId>(40), -1)},
        *npcs, GameTitle::MM6), std::exception);
    EXPECT_THROW((void) mm6::requirePreMapGeneratedNpcCapacity(
        std::deque<Actor>{actor(0, static_cast<MonsterId>(40), 1)},
        *npcs, GameTitle::MM6), std::exception);
    EXPECT_THROW((void) mm6::requirePostMapGeneratedNpcLinks(
        actors, *npcs, GameTitle::MM6), std::exception);
    actors[0].npcId = 0;
    EXPECT_THROW((void) mm6::requirePostMapGeneratedNpcLinks(
        actors, *npcs, GameTitle::MM6), std::exception);
    constexpr std::array<std::size_t, 1> cleared{0};
    EXPECT_EQ(mm6::requirePostMapGeneratedNpcLinks(
        actors, *npcs, GameTitle::MM6, cleared), 1u);
    actors[0].npcId = 5001;
    EXPECT_THROW((void) mm6::requirePostMapGeneratedNpcLinks(
        actors, *npcs, GameTitle::MM6, cleared), std::exception);
}

UNIT_TEST(MM6PostMapGeneratedNpcLinks, RejectsBrokenLinksAndRowsWithoutMutation) {
    constexpr MonsterId peasantB = MONSTER_PEASANT_DWARF_FEMALE_A_B;
    auto npcs = std::make_unique<NPCStats>();
    generatedRow(npcs.get(), 0, peasantB);
    npcs->uNewlNPCBufPos = 1;
    std::vector<Actor> actors{actor(0, peasantB, 5000)};
    EXPECT_THROW((void) mm6::requirePostMapGeneratedNpcLinks(
        actors, *npcs, GameTitle::MM7), std::exception);
    actors[0].npcId = 5001;
    EXPECT_THROW((void) mm6::requirePostMapGeneratedNpcLinks(
        actors, *npcs, GameTitle::MM6), std::exception);
    actors[0].npcId = 5000;
    npcs->pAdditionalNPC[0].name.clear();
    EXPECT_THROW((void) mm6::requirePostMapGeneratedNpcLinks(
        actors, *npcs, GameTitle::MM6), std::exception);
    generatedRow(npcs.get(), 0, peasantB);
    npcs->pAdditionalNPC[0].profession = NPC_PROFESSION_MM6_LAST;
    EXPECT_THROW((void) mm6::requirePostMapGeneratedNpcLinks(
        actors, *npcs, GameTitle::MM6), std::exception);
    EXPECT_EQ(actors[0].npcId, 5000);
    EXPECT_EQ(npcs->pAdditionalNPC[0].profession, NPC_PROFESSION_MM6_LAST);
}

UNIT_TEST(MM6PostMapGeneratedNpcLinks, RejectsUnrepresentedAndOverCapacityRows) {
    constexpr MonsterId peasantB = MONSTER_PEASANT_DWARF_FEMALE_A_B;
    auto npcs = std::make_unique<NPCStats>();
    generatedRow(npcs.get(), 0, peasantB);
    npcs->uNewlNPCBufPos = 1;
    std::vector<Actor> actors{actor(0, peasantB, 5000)};
    actors.clear();
    EXPECT_THROW((void) mm6::requirePostMapGeneratedNpcLinks(
        actors, *npcs, GameTitle::MM6), std::exception);
    npcs->uNewlNPCBufPos = 101;
    EXPECT_THROW((void) mm6::requirePostMapGeneratedNpcLinks(
        actors, *npcs, GameTitle::MM6), std::exception);
}
