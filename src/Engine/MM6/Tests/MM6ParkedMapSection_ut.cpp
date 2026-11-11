#include <cstdint>
#include <exception>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6ParkedMapSection.h"

static mm6::ParkedMapRuntime parkedOutdoorMap() {
    mm6::ParkedMapRuntime map;
    map.mapName = "outd3.odm";
    map.variables.mapVars.fill(0);
    map.variables.decorVars.fill(0);
    map.variables.mapVars[3] = 9;
    map.variables.decorVars[124] = 2;

    Actor dead(0);
    dead.monsterInfo.id = static_cast<MonsterId>(121);
    dead.monsterId = static_cast<MonsterId>(121);
    dead.monsterInfo.mm6Resistances = mm6::MonsterResistances{1, 2, 3, 4, 5, 6};
    dead.aiState = Dead;
    dead.pos = Vec3f(21800.25f, -16947.5f, 0.125f);
    dead.initialPosition = Vec3f(21800, -16880, 0);
    dead.hostilityGroup = MONSTER_TYPE_INVALID;
    dead.items[2].itemId = static_cast<ItemId>(197);
    dead.items[2].goldAmount = 25;
    dead.massDistortionTime = Duration::fromTicks(77);
    dead.donebloodsplat = true;
    Actor spawned(1);
    spawned.monsterInfo.id = static_cast<MonsterId>(7);
    spawned.buffs[ACTOR_BUFF_SLOWED].expireTime = Time::fromTicks(1234);
    spawned.buffs[ACTOR_BUFF_SLOWED].power = 2;
    spawned.velocity = Vec3f(0.5f, -1, 0);
    map.actors = {dead, spawned};

    SpriteObject drop;
    drop.uObjectDescID = 3;
    drop.vPosition = Vec3f(1.5f, 2, 3);
    drop.containing_item.itemId = static_cast<ItemId>(20);
    drop.containing_item.standardEnchantment = ATTRIBUTE_MIGHT;
    drop.containing_item.standardEnchantmentStrength = 4;
    drop.containing_item.mm6RawRecord = mm6::ItemRecord28{};
    map.sprites = {drop};
    return map;
}

UNIT_TEST(MM6ParkedMapSection, RoundTripsActorsSpritesAndVariables) {
    const std::vector<mm6::ParkedMapRuntime> maps = {parkedOutdoorMap()};
    mm6::SaveContainer container;
    container.sections.push_back(mm6::encodeParkedMapSection(maps, GameTitle::MM6));
    const std::vector<mm6::ParkedMapRuntime> replay = mm6::decodeParkedMapSection(container, GameTitle::MM6);

    ASSERT_EQ(replay.size(), 1);
    EXPECT_EQ(replay[0].mapName, "outd3.odm");
    EXPECT_EQ(replay[0].variables.mapVars, maps[0].variables.mapVars);
    EXPECT_EQ(replay[0].variables.decorVars, maps[0].variables.decorVars);
    ASSERT_EQ(replay[0].actors.size(), 2);
    const Actor &dead = replay[0].actors[0];
    EXPECT_EQ(dead.id, 0);
    EXPECT_EQ(dead.aiState, Dead);
    EXPECT_EQ(dead.pos, Vec3f(21800.25f, -16947.5f, 0.125f));
    EXPECT_EQ(dead.items[2].itemId, static_cast<ItemId>(197));
    EXPECT_EQ(dead.items[2].goldAmount, 25);
    ASSERT_TRUE(dead.monsterInfo.mm6Resistances);
    EXPECT_EQ(dead.monsterInfo.mm6Resistances->magic, 6);
    EXPECT_EQ(dead.massDistortionTime, Duration::fromTicks(77));
    EXPECT_TRUE(dead.donebloodsplat);
    EXPECT_EQ(dead.hostilityGroup, MONSTER_TYPE_INVALID);
    const Actor &spawned = replay[0].actors[1];
    EXPECT_EQ(spawned.id, 1);
    EXPECT_FALSE(spawned.monsterInfo.mm6Resistances);
    EXPECT_EQ(spawned.buffs[ACTOR_BUFF_SLOWED].expireTime, Time::fromTicks(1234));
    EXPECT_EQ(spawned.velocity, Vec3f(0.5f, -1, 0));
    ASSERT_EQ(replay[0].sprites.size(), 1);
    const Item &item = replay[0].sprites[0].containing_item;
    EXPECT_EQ(replay[0].sprites[0].vPosition, Vec3f(1.5f, 2, 3));
    EXPECT_EQ(item.itemId, static_cast<ItemId>(20));
    EXPECT_EQ(item.standardEnchantment, ATTRIBUTE_MIGHT);
    EXPECT_EQ(item.standardEnchantmentStrength, 4);
    EXPECT_TRUE(item.mm6RawRecord);
    EXPECT_EQ(mm6::encodeParkedMapSection(replay, GameTitle::MM6), container.sections[0]);
}

UNIT_TEST(MM6ParkedMapSection, RejectsBadNamesSlotsVersionsAndTitles) {
    std::vector<mm6::ParkedMapRuntime> maps = {parkedOutdoorMap()};
    EXPECT_THROW((void) mm6::encodeParkedMapSection(maps, GameTitle::MM7), std::exception);

    std::vector<mm6::ParkedMapRuntime> repeated = {maps[0], maps[0]};
    EXPECT_THROW((void) mm6::encodeParkedMapSection(repeated, GameTitle::MM6), std::exception);

    std::vector<mm6::ParkedMapRuntime> badName = maps;
    badName[0].mapName = "OutD3.odm";
    EXPECT_THROW((void) mm6::encodeParkedMapSection(badName, GameTitle::MM6), std::exception);

    std::vector<mm6::ParkedMapRuntime> badSlot = maps;
    badSlot[0].actors[1].id = 5;
    EXPECT_THROW((void) mm6::encodeParkedMapSection(badSlot, GameTitle::MM6), std::exception);

    mm6::SaveContainer container;
    container.sections.push_back(mm6::encodeParkedMapSection(maps, GameTitle::MM6));
    mm6::SaveContainer future = container;
    future.sections[0].payload[0] = 2;
    EXPECT_THROW((void) mm6::decodeParkedMapSection(future, GameTitle::MM6), std::exception);
    mm6::SaveContainer truncated = container;
    truncated.sections[0].payload.pop_back();
    EXPECT_THROW((void) mm6::decodeParkedMapSection(truncated, GameTitle::MM6), std::exception);
    mm6::SaveContainer optional = container;
    optional.sections[0].required = false;
    EXPECT_THROW((void) mm6::decodeParkedMapSection(optional, GameTitle::MM6), std::exception);
    EXPECT_THROW((void) mm6::decodeParkedMapSection(mm6::SaveContainer(), GameTitle::MM6), std::exception);
}
