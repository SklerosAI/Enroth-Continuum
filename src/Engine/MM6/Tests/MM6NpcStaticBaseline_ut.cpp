#include <cstdint>
#include <exception>
#include <string>
#include <string_view>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6NpcStaticBaseline.h"
#include "Engine/MM6/MM6RawSaveRuntime.h"
#include "Engine/MM6/MM6WorldSaveRuntime.h"

namespace {

Blob table(std::string_view text) { return Blob::copy(text.data(), text.size()); }

mm6::NpcStaticBaseline baseline() {
    return mm6::loadNpcStaticBaseline(
        table("Header\r\nHeader\r\n1\tStatic Ada\t123\t0\t0\t0\t1\t2\t1\t7\t11\t12\t13\r\n"),
        table("0\tNews\tunused\tTown news\r\n"),
        table("Header\r\nAdam\tBeth\r\n"),
        table("1\tProfession\t2\t100\tunused\tAction\tBenefit\tJoin\r\n"),
        GameTitle::MM6);
}

std::vector<std::uint8_t> indoorBase() {
    std::vector<std::uint8_t> bytes(136 + 10 * 4, 0);
    bytes[136 + 4 * 4] = 200;
    return bytes;
}

std::vector<std::uint8_t> indoorDelta() {
    return std::vector<std::uint8_t>(8 + 875 + 3 * 4 + 200 * 80 + 200 + 56, 0);
}

std::vector<std::uint8_t> outdoorBase() {
    return std::vector<std::uint8_t>(160 + 16 + 3 * 128 * 128 + 4 + 4 + 4 + 128 * 128 * 4 + 4, 0);
}

std::vector<std::uint8_t> outdoorDelta() {
    return std::vector<std::uint8_t>(8 + 2 * 88 * 11 + 3 * 4 + 200 + 56, 0);
}

mm6::RawSaveFields saved() {
    mm6::RawSaveFields fields;
    fields.core.party.playingTicks = 42;
    for (mm6::CoreCharacterFields &character : fields.core.characters)
        character.name = "Hero";
    fields.metadata.header.displayName = "NPC baseline";
    fields.metadata.header.mapFileName = "synthetic0.blv";
    fields.metadata.header.playingTicks = 42;
    fields.recovery.playingTicks = 42;
    fields.inventory.ready = true;
    fields.shops = mm6::emptyNewGameShopTitleState(GameTitle::MM6);
    fields.npcs.activeCount = 399;
    fields.npcs.npcData[1].name = "Saved Ada";
    fields.npcs.npcData[1].profession = static_cast<NpcProfession>(2);
    fields.npcs.groups[3] = 7;
    for (int i = 0; i < 52; ++i)
        fields.maps.deltas.push_back({"synthetic" + std::to_string(i) + ".dlv", indoorDelta()});
    for (int i = 0; i < 15; ++i)
        fields.maps.deltas.push_back({"outside" + std::to_string(i) + ".ddm", outdoorDelta()});
    fields.chest = {17, {}, "synthetic0.dlv", indoorDelta()};
    return fields;
}

} // namespace

UNIT_TEST(MM6NpcStaticBaseline, StagesV2SavedRowsOnFreshMm6Tables) {
    const std::string priorGlobalTopic = pNPCTopics[0].pText;
    const auto source = baseline();
    EXPECT_EQ(pNPCTopics[0].pText, priorGlobalTopic);
    EXPECT_EQ(source.stats().pOriginalNPCData[1].name, "Static Ada");
    EXPECT_EQ(source.stats().pNPCNames[SEX_MALE][0], "Adam");
    EXPECT_EQ(source.stats().pProfessions[static_cast<NpcProfession>(1)].uHirePrice, 100u);
    EXPECT_EQ(source.stats().pProfessionChance[MAP_FIRST].total, 2);
    EXPECT_EQ(source.stats().pCatchPhrases[0], "Town news");

    const auto fields = saved();
    std::vector<std::string> names;
    for (const auto &delta : fields.maps.deltas)
        names.push_back(delta.name);
    const auto raw = mm6::stageRawTitleOwners(
        mm6::encodeRawSave(fields, GameTitle::MM6), names,
        [](std::string_view name) {
            const auto bytes = name.ends_with(".blv") ? indoorBase() : outdoorBase();
            return Blob::copy(bytes.data(), bytes.size());
        }, [](std::int32_t id) { return id >= 1 && id <= 580; }, GameTitle::MM6);

    Timer timer;
    ActiveOverlayList overlays;
    const auto staged = mm6::stageRawWorldAndVisuals(source, timer, overlays, raw, 5, GameTitle::MM6);
    EXPECT_EQ(staged.world.npcs.pOriginalNPCData[1].name, "Static Ada");
    EXPECT_EQ(staged.world.npcs.pNPCData[1].name, "Saved Ada");
    EXPECT_EQ(staged.world.npcs.pGroups[3], 7);
    EXPECT_EQ(staged.world.npcs.pNPCNames[SEX_MALE][0], "Adam");
    EXPECT_EQ(staged.world.npcs.pProfessionChance[MAP_FIRST].total, 2);
    EXPECT_EQ(staged.world.mapDeltas.size(), 67);
    EXPECT_EQ(source.stats().pNPCData[1].name, "");
    EXPECT_FALSE(timer.isPaused());
    EXPECT_EQ(pNPCTopics[0].pText, priorGlobalTopic);

    EXPECT_THROW((void) mm6::stageRawWorldAndVisuals(source, timer, overlays, raw, 5,
                                                      GameTitle::MM7), std::exception);
    EXPECT_EQ(source.stats().pNPCData[1].name, "");
}

UNIT_TEST(MM6NpcStaticBaseline, RejectsForeignTitleBeforeReadingTables) {
    EXPECT_THROW((void) mm6::loadNpcStaticBaseline(Blob(), Blob(), Blob(), Blob(),
                                                   GameTitle::MM7), std::exception);
}
