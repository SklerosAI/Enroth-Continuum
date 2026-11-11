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

constexpr MonsterId streetPeasantA = static_cast<MonsterId>(121);
constexpr MonsterId streetPeasantB = static_cast<MonsterId>(134);
constexpr MonsterId mm7Peasant = static_cast<MonsterId>(137); // In MM6 this ID isn't a peasant, MM7's ID ranges say it is.

} // namespace

UNIT_TEST(MM6GeneratedNpcSaveFields, DetachedIdentityRoundTripsAndBindsMap) {
    auto npcs = std::make_unique<NPCStats>();
    row(npcs.get(), 0, streetPeasantB);
    npcs->uNewlNPCBufPos = 1;
    const std::deque<Actor> actors{actor(0, streetPeasantA, 0),
                                   actor(1, streetPeasantB, 5000)};
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

UNIT_TEST(MM6GeneratedNpcSaveFields, SignedFixedActorDoesNotConsumeGeneratedRow) {
    auto npcs = std::make_unique<NPCStats>();
    row(npcs.get(), 0, streetPeasantB);
    npcs->uNewlNPCBufPos = 1;
    std::deque<Actor> actors{actor(0, MONSTER_ANGEL_B, -1),
                             actor(1, streetPeasantB, 5000)};
    const auto fields = mm6::captureGeneratedNpcFields(
        actors, *npcs, "oute3.odm", GameTitle::MM6);
    ASSERT_EQ(fields.rows.size(), 1u);
    EXPECT_EQ(fields.rows.front().actorSlot, 1u);
    EXPECT_NO_THROW(mm6::requireGeneratedNpcActorAgreement(
        fields, actors, "oute3.odm", GameTitle::MM6));
    EXPECT_EQ(actors.front().npcId, -1);
    actors.front().id = 2;
    EXPECT_THROW((void) mm6::captureGeneratedNpcFields(
        actors, *npcs, "oute3.odm", GameTitle::MM6), std::exception);
}

UNIT_TEST(MM6GeneratedNpcSaveFields, RejectsLateRowAndActorDivergence) {
    auto npcs = std::make_unique<NPCStats>();
    row(npcs.get(), 0, streetPeasantB);
    npcs->uNewlNPCBufPos = 1;
    std::deque<Actor> actors{actor(0, streetPeasantB, 5000)};
    auto fields = mm6::captureGeneratedNpcFields(actors, *npcs, "oute3.odm", GameTitle::MM6);
    actors[0].npcId = 5001;
    EXPECT_THROW(mm6::requireGeneratedNpcActorAgreement(
        fields, actors, "oute3.odm", GameTitle::MM6), std::exception);
    EXPECT_EQ(actors[0].npcId, 5001);
    actors[0].npcId = 5000;
    fields.rows[0].npc.profession = NPC_PROFESSION_MM6_LAST;
    EXPECT_THROW((void) mm6::encodeGeneratedNpcSection(fields, GameTitle::MM6), std::exception);
    fields.rows[0].npc.profession = NPC_PROFESSION_SMITH;
    fields.rows[0].monster = mm7Peasant;
    EXPECT_THROW((void) mm6::encodeGeneratedNpcSection(fields, GameTitle::MM6), std::exception);
    fields.rows[0].monster = streetPeasantB;
    mm6::SaveSection section = mm6::encodeGeneratedNpcSection(fields, GameTitle::MM6);
    section.payload.pop_back();
    EXPECT_THROW((void) mm6::decodeGeneratedNpcSection({{section}, 2}, GameTitle::MM6),
                 std::exception);
    EXPECT_THROW((void) mm6::captureGeneratedNpcFields(
        actors, *npcs, "oute3.odm", GameTitle::MM7), std::exception);
}

UNIT_TEST(MM6GeneratedNpcSaveFields, ExactV2ManifestStillRejectsExtraIdentity) {
    auto npcs = std::make_unique<NPCStats>();
    row(npcs.get(), 0, streetPeasantB);
    npcs->uNewlNPCBufPos = 1;
    const std::deque<Actor> actors{actor(0, streetPeasantB, 5000)};
    const auto fields = mm6::captureGeneratedNpcFields(actors, *npcs,
                                                        "oute3.odm", GameTitle::MM6);
    mm6::SaveContainer container{{mm6::encodeGeneratedNpcSection(fields, GameTitle::MM6)}, 2};
    const auto bytes = mm6::encodeSaveContainer(container);
    EXPECT_THROW((void) mm6::decodeRawSave(bytes, GameTitle::MM6), std::exception);
}
