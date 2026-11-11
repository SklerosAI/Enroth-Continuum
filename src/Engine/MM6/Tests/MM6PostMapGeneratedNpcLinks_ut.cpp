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

constexpr MonsterId streetPeasantA = static_cast<MonsterId>(121);
constexpr MonsterId streetPeasantB = static_cast<MonsterId>(134);
constexpr MonsterId mm7Peasant = static_cast<MonsterId>(137); // In MM6 this ID isn't a peasant, MM7's ID ranges say it is.

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

UNIT_TEST(MM6PostMapGeneratedNpcLinks, StreetNpcFamilies) {
    // Only the three MM6 street peasant families with a map NPC link get a street NPC, other links are cleared.
    for (int id : {121, 123, 124, 126, 133, 135})
        EXPECT_TRUE(mm6::nativeStreetNpcEligible(static_cast<MonsterId>(id))) << id;
    for (int id : {1, 40, 41, 44, 120, 127, 132, 136, 137, 145, 163})
        EXPECT_FALSE(mm6::nativeStreetNpcEligible(static_cast<MonsterId>(id))) << id;
    EXPECT_TRUE(mm6::getsStreetNpc(streetPeasantA, 1));
    EXPECT_TRUE(mm6::getsStreetNpc(streetPeasantB, -1));
    EXPECT_TRUE(mm6::getsStreetNpc(streetPeasantB, 5003));
    EXPECT_FALSE(mm6::getsStreetNpc(streetPeasantB, 0));
    EXPECT_FALSE(mm6::getsStreetNpc(mm7Peasant, 1));
    EXPECT_FALSE(mm6::clearsNativeActorNpc(streetPeasantB, 1));
    EXPECT_FALSE(mm6::clearsNativeActorNpc(mm7Peasant, 0));
    EXPECT_TRUE(mm6::clearsNativeActorNpc(mm7Peasant, 2));
    EXPECT_TRUE(mm6::clearsNativeActorNpc(mm7Peasant, -1));
}

UNIT_TEST(MM6PostMapGeneratedNpcLinks, PreMapPoolCapacityCountsOnlyStreetNpcs) {
    // Maps with more MM7-style peasants than the pool holds failed to load, MM6 only gives linked street peasants a row.
    auto npcs = std::make_unique<NPCStats>();
    npcs->uNewlNPCBufPos = 7;
    npcs->pAdditionalNPC[0].name = "Prior";
    std::deque<Actor> actors;
    for (int index = 0; index < 150; ++index)
        actors.push_back(actor(index, mm7Peasant, 0));
    for (int index = 150; index < 200; ++index)
        actors.push_back(actor(index, streetPeasantB, 0));
    EXPECT_EQ(mm6::requirePreMapGeneratedNpcCapacity(actors, *npcs, GameTitle::MM6), 0u);
    for (int index = 0; index < 100; ++index)
        actors.push_back(actor(200 + index, index % 2 ? streetPeasantA : streetPeasantB, index % 3 ? 1 : -1));
    EXPECT_EQ(mm6::requirePreMapGeneratedNpcCapacity(actors, *npcs, GameTitle::MM6), 100u);
    actors.push_back(actor(300, streetPeasantA, 2));
    EXPECT_THROW((void) mm6::requirePreMapGeneratedNpcCapacity(actors, *npcs, GameTitle::MM6), std::exception);
    EXPECT_EQ(npcs->uNewlNPCBufPos, 7);
    EXPECT_EQ(npcs->pAdditionalNPC[0].name, "Prior");
    actors.back().monsterInfo.id = mm7Peasant;
    EXPECT_EQ(mm6::requirePreMapGeneratedNpcCapacity(actors, *npcs, GameTitle::MM6), 100u);
    EXPECT_THROW((void) mm6::requirePreMapGeneratedNpcCapacity(actors, *npcs, GameTitle::MM7), std::exception);
}

UNIT_TEST(MM6PostMapGeneratedNpcLinks, AcceptsContiguousStreetNpcLinks) {
    auto npcs = std::make_unique<NPCStats>();
    generatedRow(npcs.get(), 0, streetPeasantA);
    generatedRow(npcs.get(), 1, streetPeasantB);
    npcs->uNewlNPCBufPos = 2;
    const std::vector<Actor> actors{
        actor(0, streetPeasantB, 0),
        actor(1, streetPeasantA, 5000),
        actor(2, mm7Peasant, 0),
        actor(3, streetPeasantB, 5001),
        actor(4, MONSTER_ANGEL_B, 0)};
    EXPECT_EQ(mm6::requirePostMapGeneratedNpcLinks(actors, *npcs, GameTitle::MM6), 2u);
}

UNIT_TEST(MM6PostMapGeneratedNpcLinks, RejectsUnresolvedMapLinks) {
    auto npcs = std::make_unique<NPCStats>();
    generatedRow(npcs.get(), 0, streetPeasantB);
    npcs->uNewlNPCBufPos = 1;
    std::vector<Actor> actors{actor(0, mm7Peasant, 0),
                              actor(1, streetPeasantB, 5000)};
    EXPECT_EQ(mm6::requirePostMapGeneratedNpcLinks(actors, *npcs, GameTitle::MM6), 1u);
    for (int link : {-1, 1, 5000}) {
        actors[0].npcId = link;
        EXPECT_THROW((void) mm6::requirePostMapGeneratedNpcLinks(actors, *npcs, GameTitle::MM6), std::exception) << link;
    }
    actors[0].npcId = 0;
    actors[1].npcId = 1;
    EXPECT_THROW((void) mm6::requirePostMapGeneratedNpcLinks(actors, *npcs, GameTitle::MM6), std::exception);
    actors[1].npcId = 5000;
    constexpr std::array<std::size_t, 1> cleared{0};
    EXPECT_EQ(mm6::requirePostMapGeneratedNpcLinks(actors, *npcs, GameTitle::MM6, cleared), 1u);
    constexpr std::array<std::size_t, 1> eligibleCleared{1};
    EXPECT_THROW((void) mm6::requirePostMapGeneratedNpcLinks(actors, *npcs, GameTitle::MM6, eligibleCleared), std::exception);
}

UNIT_TEST(MM6PostMapGeneratedNpcLinks, RejectsBrokenLinksAndRowsWithoutMutation) {
    auto npcs = std::make_unique<NPCStats>();
    generatedRow(npcs.get(), 0, streetPeasantB);
    npcs->uNewlNPCBufPos = 1;
    std::vector<Actor> actors{actor(0, streetPeasantB, 5000)};
    EXPECT_THROW((void) mm6::requirePostMapGeneratedNpcLinks(
        actors, *npcs, GameTitle::MM7), std::exception);
    actors[0].npcId = 5001;
    EXPECT_THROW((void) mm6::requirePostMapGeneratedNpcLinks(
        actors, *npcs, GameTitle::MM6), std::exception);
    actors[0].npcId = 5000;
    npcs->pAdditionalNPC[0].name.clear();
    EXPECT_THROW((void) mm6::requirePostMapGeneratedNpcLinks(
        actors, *npcs, GameTitle::MM6), std::exception);
    generatedRow(npcs.get(), 0, streetPeasantB);
    npcs->pAdditionalNPC[0].profession = NPC_PROFESSION_MM6_LAST;
    EXPECT_THROW((void) mm6::requirePostMapGeneratedNpcLinks(
        actors, *npcs, GameTitle::MM6), std::exception);
    EXPECT_EQ(actors[0].npcId, 5000);
    EXPECT_EQ(npcs->pAdditionalNPC[0].profession, NPC_PROFESSION_MM6_LAST);
}

UNIT_TEST(MM6PostMapGeneratedNpcLinks, RejectsUnrepresentedAndOverCapacityRows) {
    auto npcs = std::make_unique<NPCStats>();
    generatedRow(npcs.get(), 0, streetPeasantB);
    npcs->uNewlNPCBufPos = 1;
    std::vector<Actor> actors;
    EXPECT_THROW((void) mm6::requirePostMapGeneratedNpcLinks(
        actors, *npcs, GameTitle::MM6), std::exception);
    npcs->uNewlNPCBufPos = 101;
    EXPECT_THROW((void) mm6::requirePostMapGeneratedNpcLinks(
        actors, *npcs, GameTitle::MM6), std::exception);
}
