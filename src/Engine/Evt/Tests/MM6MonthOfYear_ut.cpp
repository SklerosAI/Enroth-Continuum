#include <stdexcept>
#include <string>
#include <utility>

#include "Testing/Unit/UnitTest.h"

#include "Engine/Evt/EvtProgram.h"
#include "Engine/Evt/MM6MonthOfYear.h"

#include "Utility/Memory/Blob.h"

UNIT_TEST(MM6MonthOfYear, FirstMapShapeDecodesAsMonthSixBranch) {
    // One synthetic MM6 Compare record: event 1, step 0, raw variable 226,
    // month 6, branch to step 3. No retained game bytes are embedded here.
    std::string bytes = std::string("\x0a\x01\x00\x00\x0e\xe2\x06\x00\x00\x00\x03", 11);
    EvtProgram program = EvtProgram::load(Blob::fromString(bytes), GameTitle::MM6);
    const EvtInstruction &ir = program.instruction(1, 0);
    EXPECT_EQ(ir.gameTitle, GameTitle::MM6);
    EXPECT_EQ(ir.opcode, EVENT_Compare);
    EXPECT_EQ(std::to_underlying(ir.data.variable_descr.type), 226);
    EXPECT_EQ(ir.data.variable_descr.value, 6);
    EXPECT_EQ(ir.target_step, 3);
}

UNIT_TEST(MM6MonthOfYear, ComparesOneBasedMonthsAcrossYearBoundary) {
    EXPECT_TRUE(mm6::compareMonthOfYear(Time::fromMonths(0), 226, 1));
    EXPECT_FALSE(mm6::compareMonthOfYear(Time::fromMonths(0), 226, 6));
    EXPECT_FALSE(mm6::compareMonthOfYear(Time::fromMonths(4), 226, 6));
    EXPECT_TRUE(mm6::compareMonthOfYear(Time::fromMonths(5), 226, 6));
    EXPECT_FALSE(mm6::compareMonthOfYear(Time::fromMonths(6), 226, 6));
    EXPECT_TRUE(mm6::compareMonthOfYear(Time::fromMonths(11), 226, 12));
    EXPECT_TRUE(mm6::compareMonthOfYear(Time::fromMonths(12), 226, 1));
}

UNIT_TEST(MM6MonthOfYear, InvalidInputRejects) {
    EXPECT_THROW(mm6::compareMonthOfYear(Time::fromMonths(0), 225, 1), std::runtime_error);
    EXPECT_THROW(mm6::compareMonthOfYear(Time::fromMonths(0), 226, 0), std::runtime_error);
    EXPECT_THROW(mm6::compareMonthOfYear(Time::fromMonths(0), 226, 13), std::runtime_error);
    EXPECT_THROW(mm6::compareMonthOfYear(Time::fromTicks(-1), 226, 1), std::runtime_error);
}
