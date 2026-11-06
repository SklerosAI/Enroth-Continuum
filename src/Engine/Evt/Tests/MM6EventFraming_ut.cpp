#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <string>

#include "Testing/Unit/UnitTest.h"

#include "Engine/Evt/EvtProgram.h"
#include "Engine/Evt/MM6EventRules.h"

#include "Utility/Memory/Blob.h"

namespace {

void appendU32(std::string *bytes, uint32_t value) {
    for (int i = 0; i < 4; ++i)
        bytes->push_back(static_cast<char>(value >> (8 * i)));
}

std::string record(uint16_t id, EvtOpcode opcode, std::string payload) {
    std::string result;
    result.push_back(static_cast<char>(4 + payload.size())); // Size excludes the length byte.
    result.push_back(static_cast<char>(id));
    result.push_back(static_cast<char>(id >> 8));
    result.push_back(0); // Step.
    result.push_back(static_cast<char>(opcode));
    result += payload;
    return result;
}

} // namespace

UNIT_TEST(MM6EventFraming, ActiveTitleSpecificRecords) {
    std::string source;

    std::string texture;
    appendU32(&texture, 11);
    appendU32(&texture, 22);
    texture += std::string("stone1\0", 7);
    source += record(101, EVENT_ShowMovie, texture);

    std::string monsters = std::string("\x03\x02\x01", 3);
    appendU32(&monsters, 100);
    appendU32(&monsters, 200);
    appendU32(&monsters, 300);
    source += record(102, EVENT_SummonMonsters, monsters);

    std::string facet;
    appendU32(&facet, 2);
    appendU32(&facet, 3);
    appendU32(&facet, 4);
    facet.push_back(1);
    source += record(103, EVENT_ToggleActorFlag, facet);

    std::string question;
    appendU32(&question, 5);
    appendU32(&question, 6);
    appendU32(&question, 7);
    question.push_back(8);
    source += record(104, EVENT_InputString, question);

    source += record(105, EVENT_MM6_Opcode20, std::string("\x01\x02\x03\x04\x05", 5));
    source += record(106, EVENT_MM6_Opcode27, std::string("\x01\x02", 2));
    source += record(107, EVENT_MM6_Opcode28, std::string("\x01", 1));

    EvtProgram program = EvtProgram::load(Blob::fromString(source), GameTitle::MM6);
    const EvtInstruction &textureIr = program.instruction(101, 0);
    EXPECT_EQ(textureIr.gameTitle, GameTitle::MM6);
    EXPECT_EQ(textureIr.data.mm6_outdoor_texture_descr.model, 11);
    EXPECT_EQ(textureIr.data.mm6_outdoor_texture_descr.facet, 22);
    EXPECT_EQ(textureIr.str, "stone1");
    EXPECT_EQ(program.instruction(102, 0).data.monster_descr.z, 300);
    EXPECT_EQ(program.instruction(103, 0).data.mm6_outdoor_facet_descr.bit, 4);
    EXPECT_EQ(program.instruction(104, 0).target_step, 8);
    EXPECT_EQ(program.instruction(105, 0).data.mm6_unknown_descr.length, 5);
    EXPECT_EQ(program.instruction(106, 0).data.mm6_unknown_descr.length, 2);
    EXPECT_EQ(program.instruction(107, 0).data.mm6_unknown_descr.length, 1);
}

UNIT_TEST(MM6EventFraming, RejectsTruncatedOrForeignRecords) {
    std::string mm6Only = record(1, EVENT_MM6_Opcode20, std::string("\x01\x02\x03\x04\x05", 5));
    EXPECT_THROW(EvtProgram::load(Blob::fromString(mm6Only), GameTitle::MM7), std::exception);

    mm6Only.pop_back();
    EXPECT_THROW(EvtProgram::load(Blob::fromString(mm6Only), GameTitle::MM6), std::exception);

    std::string movie = record(2, EVENT_ShowMovie, std::string("\x01\x00" "intro\0", 8));
    EvtProgram mm7 = EvtProgram::load(Blob::fromString(movie), GameTitle::MM7);
    EXPECT_EQ(mm7.instruction(2, 0).str, "intro");
}

UNIT_TEST(MM6EventFraming, AllActiveRetainedResources) {
    const char *directory = std::getenv("MM6_RETAINED_EVT_DIR");
    const char *listPath = std::getenv("MM6_ACTIVE_EVT_LIST");
    if (!directory || !listPath)
        GTEST_SKIP() << "Private retained MM6 event corpus was not supplied";

    std::ifstream list(listPath);
    ASSERT_TRUE(list.good());
    size_t checked = 0;
    std::string name;
    while (std::getline(list, name)) {
        if (name.empty())
            continue;
        try {
            std::filesystem::path path = std::filesystem::path(directory) / name;
            (void) EvtProgram::load(Blob::fromFile(Path(path.string())), GameTitle::MM6);
            ++checked;
        } catch (const std::exception &e) {
            ADD_FAILURE() << "Failed to parse retained EVT resource: " << e.what();
        }
    }
    EXPECT_EQ(checked, 68); // 67 listed map scripts plus global.evt; extras are separately classified.
}

UNIT_TEST(MM6EventFraming, ZeroMasterySkillCheckMatchesTrainedSkill) {
    EXPECT_FALSE(mm6CheckSkillSatisfied(CombinedSkillValue::none(), MASTERY_NONE, 8));
    EXPECT_FALSE(mm6CheckSkillSatisfied(CombinedSkillValue::novice(7), MASTERY_NONE, 8));
    EXPECT_TRUE(mm6CheckSkillSatisfied(CombinedSkillValue::novice(8), MASTERY_NONE, 8));
    EXPECT_TRUE(mm6CheckSkillSatisfied(CombinedSkillValue(8, MASTERY_EXPERT), MASTERY_NONE, 8));
    EXPECT_FALSE(mm6CheckSkillSatisfied(CombinedSkillValue(8, MASTERY_EXPERT), MASTERY_MASTER, 8));
}
