#include <deque>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6V3PreEventReplayForTest.h"
#include "Engine/MM6/MM6ActiveActorSection.h"
#include "Engine/Objects/MonsterEnumFunctions.h"
#include "Library/Random/MersenneTwisterRandomEngine.h"

namespace {

constexpr MonsterId PEASANT = static_cast<MonsterId>(122); // An MM6 street peasant.

Actor actor(int slot, int npcId) {
    Actor result(slot);
    result.monsterInfo.id = PEASANT;
    result.monsterInfo.attackPreferences = MonsterAttackPreferences{};
    result.monsterInfo.mm6Resistances = mm6::MonsterResistances{1, 2, 3, 4, 5, 6};
    result.npcId = npcId;
    return result;
}

GameplayRandomState randomState() {
    MersenneTwisterRandomEngine random;
    random.seed(81);
    GameplayRandomState state;
    state.mtState = random.stateText();
    return state;
}

mm6::GeneratedNpcSaveFields generatedFields() {
    auto npcs = std::make_unique<NPCStats>();
    NPCData &row = npcs->pAdditionalNPC[0];
    row.name = "Generated";
    row.portraitId = 300;
    row.canJoin = true;
    row.field_24 = 1;
    row.house = HOUSE_INVALID;
    row.sex = sexForMonsterType(monsterTypeForMonsterId(PEASANT));
    row.profession = NPC_PROFESSION_SMITH;
    npcs->uNewlNPCBufPos = 1;
    return mm6::captureGeneratedNpcFields(
        std::deque<Actor>{actor(0, 5000)}, *npcs, "oute3.odm", GameTitle::MM6);
}

} // namespace

UNIT_TEST(MM6V3PreEventReplayForTest, StagesCompleteActorListWithoutMutation) {
    const auto saved = generatedFields();
    const auto rng = randomState();
    auto freshNpcs = std::make_unique<NPCStats>();
    std::deque<Actor> rebuilt{actor(0, 0)};
    std::vector<Actor> active{actor(0, 5000)};
    active[0].hp = 41;
    active.push_back(Actor(1));
    active[1].monsterInfo.id = MONSTER_ANGEL_B;
    active[1].hp = 29;
    auto plan = mm6::stagePreEventActorReplay(
        active, saved, rng, rebuilt, *freshNpcs, "oute3.odm", 1, 10,
        RANDOM_ENGINE_MERSENNE_TWISTER, GameTitle::MM6);
    EXPECT_EQ(rebuilt[0].npcId, 0);
    EXPECT_EQ(freshNpcs->uNewlNPCBufPos, 0);
    EXPECT_EQ(plan.actors[0].hp, 41);
    EXPECT_EQ(plan.actors.size(), 2u);

    mm6::publishPreEventActorReplay(
        std::move(plan), &rebuilt, freshNpcs.get(), GameTitle::MM6);
    EXPECT_EQ(rebuilt[0].npcId, 5000);
    EXPECT_EQ(rebuilt[0].hp, 41);
    EXPECT_EQ(rebuilt.size(), 2u);
    EXPECT_EQ(rebuilt[1].hp, 29);
    EXPECT_EQ(freshNpcs->uNewlNPCBufPos, 1);
    EXPECT_EQ(freshNpcs->pAdditionalNPC[0].name, "Generated");
}

UNIT_TEST(MM6V3PreEventReplayForTest, RejectsMissingStateAndPreservesOwners) {
    auto freshNpcs = std::make_unique<NPCStats>();
    std::deque<Actor> rebuilt{actor(0, 0)};
    std::vector<Actor> active{actor(0, 5000)};
    const auto saved = generatedFields();
    const auto rng = randomState();
    EXPECT_THROW((void) mm6::stagePreEventActorReplay(
        active, {}, rng, rebuilt, *freshNpcs, "oute3.odm", 1, 10,
        RANDOM_ENGINE_MERSENNE_TWISTER, GameTitle::MM6), std::exception);
    EXPECT_THROW((void) mm6::stagePreEventActorReplay(
        active, saved, rng, rebuilt, *freshNpcs, "oute3.odm", 2, 10,
        RANDOM_ENGINE_MERSENNE_TWISTER, GameTitle::MM6), std::exception);
    EXPECT_THROW((void) mm6::stagePreEventActorReplay(
        active, saved, rng, rebuilt, *freshNpcs, "oute3.odm", 1, 10,
        RANDOM_ENGINE_SEQUENTIAL, GameTitle::MM6), std::exception);
    active[0].initialPosition.x = 1;
    EXPECT_THROW((void) mm6::stagePreEventActorReplay(
        active, saved, rng, rebuilt, *freshNpcs, "oute3.odm", 1, 10,
        RANDOM_ENGINE_MERSENNE_TWISTER, GameTitle::MM6), std::exception);
    EXPECT_EQ(rebuilt[0].npcId, 0);
    EXPECT_EQ(freshNpcs->uNewlNPCBufPos, 0);

    active[0].initialPosition.x = 0;
    auto plan = mm6::stagePreEventActorReplay(
        active, saved, rng, rebuilt, *freshNpcs, "oute3.odm", 1, 10,
        RANDOM_ENGINE_MERSENNE_TWISTER, GameTitle::MM6);
    EXPECT_THROW(mm6::publishPreEventActorReplay(
        std::move(plan), &rebuilt, freshNpcs.get(), GameTitle::MM7),
        std::exception);
    EXPECT_EQ(rebuilt[0].npcId, 0);
    EXPECT_EQ(freshNpcs->uNewlNPCBufPos, 0);
}

UNIT_TEST(MM6V3PreEventReplayForTest, ScopedReplayRestoresRngOnlyAfterNpcBypass) {
    EXPECT_FALSE(mm6::testing::consumeV3GeneratedNpcBypassForTest(GameTitle::MM6));
    EXPECT_NO_THROW(mm6::testing::requireV3ReplayBeforeMapLoadForTest(GameTitle::MM6));
    auto staged = std::make_unique<mm6::V3OutdoorActorMapAgreement>();
    staged->activeMap.mapName = "oute3.odm";
    staged->activeActors.push_back(actor(0, 5000));
    staged->saved.raw.metadata.header.mapFileName = "oute3.odm";
    staged->saved.generatedNpcs = generatedFields();
    staged->saved.gameplayRng = randomState();
    staged->saved.raw.chest.lcgState = 42;
    staged->saved.raw.chest.rawActiveDelta = {1};
    staged->activeMap.delta.actors.emplace_back();
    staged->activeSidecar.actors.emplace_back();
    staged->saved.activeActorPayload = mm6::encodeActiveActorSection(
        staged->activeActors, GameTitle::MM6);
    const GameplayRandomState expectedRng = staged->saved.gameplayRng;
    auto freshNpcs = std::make_unique<NPCStats>();
    std::deque<Actor> rebuilt{actor(0, 0)};
    mm6::ChestTitleState chest;
    chest.ready = true;
    chest.lcgState = 42;
    const std::vector<std::uint8_t> delta{1};
    bool eventStarted = false;
    bool rngRestored = false;
    {
        auto scope = std::make_unique<mm6::testing::ScopedV3PreEventReplayForTest>(
            GameTitle::MM6, std::move(*staged));
        EXPECT_THROW(mm6::testing::consumeV3GeneratedNpcBypassForTest(GameTitle::MM6),
                     std::exception);
        EXPECT_THROW(mm6::testing::requireV3ReplayBeforeMapLoadForTest(GameTitle::MM6),
                     std::exception);
        EXPECT_THROW(mm6::testing::runArmedV3PreEventReplayForTest(
            mm6::testing::PreEventCheckpointResult::Unarmed,
            rebuilt, &rebuilt, freshNpcs.get(), RANDOM_ENGINE_MERSENNE_TWISTER,
            "oute3.odm",
            10, 0, false, true, true, &chest, delta, GameTitle::MM6), std::exception);
        EXPECT_FALSE(scope->published());
        EXPECT_THROW(mm6::testing::runArmedV3PreEventReplayForTest(
            mm6::testing::PreEventCheckpointResult::Continue,
            rebuilt, &rebuilt, freshNpcs.get(), RANDOM_ENGINE_MERSENNE_TWISTER,
            "oute3.odm", 10, 0, false, true, false, &chest, delta,
            GameTitle::MM6), std::exception);
        EXPECT_EQ(rebuilt[0].npcId, 0);
        EXPECT_TRUE(mm6::testing::runArmedV3PreEventReplayForTest(
            mm6::testing::PreEventCheckpointResult::Continue,
            rebuilt, &rebuilt, freshNpcs.get(), RANDOM_ENGINE_MERSENNE_TWISTER,
            "oute3.odm", 10, 0, false, true, true, &chest, delta,
            GameTitle::MM6));
        EXPECT_TRUE(scope->published());
        EXPECT_EQ(rebuilt[0].npcId, 5000);
        EXPECT_FALSE(scope->rngPublished());
        EXPECT_THROW(mm6::testing::requireV3ReplayBeforeMapLoadForTest(GameTitle::MM6),
                     std::exception);
        EXPECT_TRUE(mm6::testing::consumeV3GeneratedNpcBypassForTest(GameTitle::MM6));
        EXPECT_THROW(mm6::testing::consumeV3GeneratedNpcBypassForTest(GameTitle::MM6),
                     std::exception);
        EXPECT_THROW(mm6::testing::restoreV3RngBeforeMapLoadForTest(
            [&](const GameplayRandomState &) { rngRestored = true; },
            &chest, {}, GameTitle::MM6), std::exception);
        EXPECT_FALSE(rngRestored);
        mm6::testing::restoreV3RngBeforeMapLoadForTest(
            [&](const GameplayRandomState &state) {
                EXPECT_FALSE(eventStarted);
                EXPECT_EQ(state, expectedRng);
                rngRestored = true;
            }, &chest, delta, GameTitle::MM6);
        EXPECT_TRUE(rngRestored);
        EXPECT_TRUE(scope->rngPublished());
        EXPECT_NO_THROW(mm6::testing::requireV3ReplayBeforeMapLoadForTest(GameTitle::MM6));
        eventStarted = true;
        EXPECT_TRUE(eventStarted);
    }
    EXPECT_EQ(rebuilt[0].npcId, 5000);
    EXPECT_FALSE(mm6::testing::consumeV3GeneratedNpcBypassForTest(GameTitle::MM6));
}
