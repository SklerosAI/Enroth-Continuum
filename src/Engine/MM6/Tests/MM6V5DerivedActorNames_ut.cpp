#include <array>
#include <exception>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6V5DerivedActorNames.h"

UNIT_TEST(MM6V5DerivedActorNames, ClearsOnlyExactStaticNamesInDetachedCopy) {
    MonsterStats stats;
    const MonsterId id = static_cast<MonsterId>(1);
    stats.infos[id].name = "Visible name";
    stats.infos[id].internalName = "InternalA";
    Actor actor(0);
    actor.monsterInfo.id = id;
    actor.monsterInfo.name = stats.infos[id].name;
    actor.monsterInfo.internalName = stats.infos[id].internalName;
    const std::array<Actor, 1> live{actor};
    const auto canonical = mm6::canonicalActorsWithDerivedNamesRemoved(
        live, stats, GameTitle::MM6);
    EXPECT_TRUE(canonical[0].monsterInfo.name.empty());
    EXPECT_TRUE(canonical[0].monsterInfo.internalName.empty());
    EXPECT_EQ(live[0].monsterInfo.name, "Visible name");
    EXPECT_EQ(live[0].monsterInfo.internalName, "InternalA");
    EXPECT_THROW((void) mm6::canonicalActorsWithDerivedNamesRemoved(
        live, stats, GameTitle::MM7), std::exception);
}

UNIT_TEST(MM6V5DerivedActorNames, RefusesPartialOrMutatedNames) {
    MonsterStats stats;
    const MonsterId id = static_cast<MonsterId>(1);
    stats.infos[id].name = "Visible name";
    stats.infos[id].internalName = "InternalA";
    Actor actor(0);
    actor.monsterInfo.id = id;
    actor.monsterInfo.name = "Visible name";
    const std::array<Actor, 1> partial{actor};
    EXPECT_THROW((void) mm6::canonicalActorsWithDerivedNamesRemoved(
        partial, stats, GameTitle::MM6), std::exception);
    actor.monsterInfo.internalName = "Wrong";
    const std::array<Actor, 1> mutated{actor};
    EXPECT_THROW((void) mm6::canonicalActorsWithDerivedNamesRemoved(
        mutated, stats, GameTitle::MM6), std::exception);
}
