#include <deque>
#include <memory>
#include <stdexcept>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6GeneratedNpcSaveFields.h"
#include "Engine/MM6/MM6RawSaveFields.h"
#include "Engine/Objects/MonsterEnumFunctions.h"

namespace {

Actor actor(int slot, MonsterId monster, int npcId) {
    Actor result(slot);
    result.monsterInfo.id = monster;
    result.npcId = npcId;
    return result;
}

void row(NPCStats *npcs, size_t index, MonsterId monster) {
    NPCData &npc = npcs->pAdditionalNPC[index];
    npc.name = "Generated";
    npc.portraitId = 300;
    npc.canJoin = true;
    npc.field_24 = 1;
    npc.house = HOUSE_INVALID;
    npc.sex = sexForMonsterType(monsterTypeForMonsterId(monster));
    npc.profession = NPC_PROFESSION_SMITH;
}

} // namespace

UNIT_TEST(MM6GeneratedNpcSaveFields, DetachedIdentityRoundTripsAndBindsMap) {
    constexpr MonsterId peasantB = MONSTER_PEASANT_DWARF_FEMALE_A_B;
    auto npcs = std::make_unique<NPCStats>();
    row(npcs.get(), 0, peasantB);
    npcs->uNewlNPCBufPos = 1;
    const std::deque<Actor> actors{actor(0, MONSTER_PEASANT_DWARF_FEMALE_A_A, 0),
                                   actor(1, peasantB, 5000)};
    const auto fields = mm6::captureGeneratedNpcFields(
        actors, *npcs, "oute3.odm", GameTitle::MM6);
    const mm6::SaveContainer container{{mm6::encodeGeneratedNpcSection(fields, GameTitle::MM6)}, 2};
    const auto decoded = mm6::decodeGeneratedNpcSection(
        mm6::decodeSaveContainer(mm6::encodeSaveContainer(container)), GameTitle::MM6);
    EXPECT_EQ(decoded.mapName, "oute3.odm");
    ASSERT_EQ(decoded.rows.size(), 1);
    EXPECT_EQ(decoded.rows[0].actorSlot, 1);
    EXPECT_EQ(decoded.rows[0].npc.name, "Generated");
    EXPECT_EQ(mm6::encodeGeneratedNpcSection(decoded, GameTitle::MM6),
              mm6::encodeGeneratedNpcSection(fields, GameTitle::MM6));
    EXPECT_NO_THROW(mm6::requireGeneratedNpcActorAgreement(
        decoded, actors, "oute3.odm", GameTitle::MM6));
    EXPECT_THROW(mm6::requireGeneratedNpcActorAgreement(
        decoded, actors, "other.odm", GameTitle::MM6), std::exception);
}

UNIT_TEST(MM6GeneratedNpcSaveFields, RejectsLateRowAndActorDivergence) {
    constexpr MonsterId peasantB = MONSTER_PEASANT_DWARF_FEMALE_A_B;
    auto npcs = std::make_unique<NPCStats>();
    row(npcs.get(), 0, peasantB);
    npcs->uNewlNPCBufPos = 1;
    std::deque<Actor> actors{actor(0, peasantB, 5000)};
    auto fields = mm6::captureGeneratedNpcFields(actors, *npcs, "oute3.odm", GameTitle::MM6);
    actors[0].npcId = 5001;
    EXPECT_THROW(mm6::requireGeneratedNpcActorAgreement(
        fields, actors, "oute3.odm", GameTitle::MM6), std::exception);
    EXPECT_EQ(actors[0].npcId, 5001);
    actors[0].npcId = 5000;
    fields.rows[0].npc.profession = NPC_PROFESSION_MM6_LAST;
    EXPECT_THROW((void) mm6::encodeGeneratedNpcSection(fields, GameTitle::MM6), std::exception);
    fields.rows[0].npc.profession = NPC_PROFESSION_SMITH;
    mm6::SaveSection section = mm6::encodeGeneratedNpcSection(fields, GameTitle::MM6);
    section.payload.pop_back();
    EXPECT_THROW((void) mm6::decodeGeneratedNpcSection({{section}, 2}, GameTitle::MM6),
                 std::exception);
    EXPECT_THROW((void) mm6::captureGeneratedNpcFields(
        actors, *npcs, "oute3.odm", GameTitle::MM7), std::exception);
}

UNIT_TEST(MM6GeneratedNpcSaveFields, ExactV2ManifestStillRejectsExtraIdentity) {
    constexpr MonsterId peasantB = MONSTER_PEASANT_DWARF_FEMALE_A_B;
    auto npcs = std::make_unique<NPCStats>();
    row(npcs.get(), 0, peasantB);
    npcs->uNewlNPCBufPos = 1;
    const std::deque<Actor> actors{actor(0, peasantB, 5000)};
    const auto fields = mm6::captureGeneratedNpcFields(actors, *npcs,
                                                        "oute3.odm", GameTitle::MM6);
    mm6::SaveContainer container{{mm6::encodeGeneratedNpcSection(fields, GameTitle::MM6)}, 2};
    const auto bytes = mm6::encodeSaveContainer(container);
    EXPECT_THROW((void) mm6::decodeRawSave(bytes, GameTitle::MM6), std::exception);
}
