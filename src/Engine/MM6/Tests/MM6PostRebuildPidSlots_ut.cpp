#include <array>
#include <exception>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6PostRebuildPidSlots.h"

namespace {

mm6::DetachedIndoorWorldPlan indoorPlan() {
    mm6::DetachedIndoorWorldPlan plan;
    plan.mapName = "test.blv";
    Actor_MM7 actor{};
    actor.npcId = 4;
    actor.monsterId = 12;
    actor.monsterInfo.id = 7;
    actor.initialPosition = {10, 20, 30};
    plan.savedDelta.actors.push_back(actor);
    plan.activeSidecar.actors.push_back(actor);
    SpriteObject_MM7 sprite{};
    sprite.spriteId = 3;
    sprite.objectDescId = 5;
    sprite.initialPosition = {30, 40, 50};
    plan.savedDelta.spriteObjects.push_back(sprite);
    plan.activeSidecar.spriteObjects.push_back(sprite);
    return plan;
}

std::array<Actor, 1> rebuiltActors() {
    std::array<Actor, 1> actors;
    actors[0].id = 0;
    actors[0].npcId = 4;
    actors[0].monsterId = static_cast<MonsterId>(12);
    actors[0].monsterInfo.id = static_cast<MonsterId>(7);
    actors[0].initialPosition = Vec3f(10, 20, 30);
    return actors;
}

std::array<SpriteObject, 1> rebuiltSprites() {
    std::array<SpriteObject, 1> sprites;
    sprites[0].spriteId = static_cast<SpriteId>(3);
    sprites[0].uObjectDescID = 5;
    sprites[0].initialPosition = Vec3f(30, 40, 50);
    return sprites;
}

} // namespace

UNIT_TEST(MM6PostRebuildPidSlots, IndoorAcceptsMatchingFinalSlotsOnly) {
    auto plan = indoorPlan();
    auto actors = rebuiltActors();
    auto sprites = rebuiltSprites();
    EXPECT_NO_THROW(mm6::requirePostRebuildPidSlotAgreement(
        plan, "test.blv", actors, sprites, GameTitle::MM6));
    EXPECT_THROW(mm6::requirePostRebuildPidSlotAgreement(
        plan, "other.blv", actors, sprites, GameTitle::MM6), std::exception);
    EXPECT_THROW(mm6::requirePostRebuildPidSlotAgreement(
        plan, "test.blv", actors, sprites, GameTitle::MM7), std::exception);

    actors[0].id = 1;
    EXPECT_THROW(mm6::requirePostRebuildPidSlotAgreement(
        plan, "test.blv", actors, sprites, GameTitle::MM6), std::exception);
    actors = rebuiltActors();
    actors[0].initialPosition.x++;
    EXPECT_THROW(mm6::requirePostRebuildPidSlotAgreement(
        plan, "test.blv", actors, sprites, GameTitle::MM6), std::exception);
    actors = rebuiltActors();
    plan.activeSidecar.actors[0].npcId++;
    EXPECT_THROW(mm6::requirePostRebuildPidSlotAgreement(
        plan, "test.blv", actors, sprites, GameTitle::MM6), std::exception);
}

UNIT_TEST(MM6PostRebuildPidSlots, OutdoorRejectsSpriteReplacementAndCountChange) {
    const auto indoor = indoorPlan();
    mm6::DetachedOutdoorWorldPlan plan;
    plan.mapName = "test.odm";
    plan.savedDelta.actors = indoor.savedDelta.actors;
    plan.savedDelta.spriteObjects = indoor.savedDelta.spriteObjects;
    plan.activeSidecar.actors = indoor.activeSidecar.actors;
    plan.activeSidecar.spriteObjects = indoor.activeSidecar.spriteObjects;
    auto actors = rebuiltActors();
    auto sprites = rebuiltSprites();
    EXPECT_NO_THROW(mm6::requirePostRebuildPidSlotAgreement(
        plan, "test.odm", actors, sprites, GameTitle::MM6));
    sprites[0].uObjectDescID++;
    EXPECT_THROW(mm6::requirePostRebuildPidSlotAgreement(
        plan, "test.odm", actors, sprites, GameTitle::MM6), std::exception);
    sprites = rebuiltSprites();
    sprites[0].initialPosition.z++;
    EXPECT_THROW(mm6::requirePostRebuildPidSlotAgreement(
        plan, "test.odm", actors, sprites, GameTitle::MM6), std::exception);
    sprites = rebuiltSprites();
    plan.savedDelta.spriteObjects.push_back(plan.savedDelta.spriteObjects[0]);
    EXPECT_THROW(mm6::requirePostRebuildPidSlotAgreement(
        plan, "test.odm", actors, sprites, GameTitle::MM6), std::exception);
}

UNIT_TEST(MM6PostRebuildPidSlots, ActorLinksFollowTheMapLoadStreetNpcPolicy) {
    // A saved map link matches the rebuilt actor as it is, or as the map load leaves it: replaced with a street NPC row
    // for a street peasant, cleared for any other monster.
    for (int monster : {123, 7}) {
        auto plan = indoorPlan();
        plan.savedDelta.actors[0].monsterInfo.id = plan.activeSidecar.actors[0].monsterInfo.id = monster;
        plan.savedDelta.actors[0].npcId = plan.activeSidecar.actors[0].npcId = 1;
        auto actors = rebuiltActors();
        actors[0].monsterInfo.id = static_cast<MonsterId>(monster);
        const auto sprites = rebuiltSprites();
        const bool street = monster == 123;
        for (int link : {1, 0, 5000, 2}) {
            actors[0].npcId = link;
            const bool expected = link == 1 || (street ? link == 5000 : link == 0);
            if (expected) {
                EXPECT_NO_THROW(mm6::requirePostRebuildPidSlotAgreement(plan, "test.blv", actors, sprites, GameTitle::MM6)) << monster << ' ' << link;
            } else {
                EXPECT_THROW(mm6::requirePostRebuildPidSlotAgreement(plan, "test.blv", actors, sprites, GameTitle::MM6), std::exception)
                    << monster << ' ' << link;
            }
        }
    }
}
