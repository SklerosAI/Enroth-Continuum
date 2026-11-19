#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "Testing/Unit/MM6TestData.h"
#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6CreationRules.h"
#include "Engine/MM6/MM6Executable.h"

#include "Utility/Exception.h"
#include "Utility/Memory/Blob.h"

UNIT_TEST(MM6Executable, RejectsOtherFiles) {
    // Anything but the GOG 1.2 MM6.exe is refused with its hash, before any table is read.
    EXPECT_THROW(mm6::Executable(Blob::fromString("MZ")), Exception);

    const std::unique_ptr<MM6TestData> data = MM6TestData::open();
    if (!data)
        GTEST_SKIP() << mm6TestDataMissing;
    std::string patched(data->read("mm6.exe").str());
    patched[0x400] ^= 1;
    try {
        mm6::Executable exe(Blob::fromString(std::move(patched)).withDisplayPath("patched/MM6.exe"));
        ADD_FAILURE() << "A patched MM6.exe was accepted";
    } catch (const Exception &e) {
        EXPECT_NE(std::string(e.what()).find("'patched/MM6.exe' is not the MM6.exe of the GOG release"), std::string::npos) << e.what();
    }
}

UNIT_TEST(MM6Executable, ReadsByVirtualAddress) {
    // The room table points at room 1's movie name, and its pointer for room 0 leads past the file's data.
    const std::unique_ptr<MM6TestData> data = MM6TestData::open();
    if (!data)
        GTEST_SKIP() << mm6TestDataMissing;
    const mm6::Executable exe(data->read("mm6.exe"));
    EXPECT_EQ(exe.u8(0x4C2630), 30); // Knight family HP base.
    EXPECT_EQ(exe.string(exe.u32(0x4BE898)), "blcksrch");
    EXPECT_EQ(exe.string(exe.u32(0x4BE888)), std::nullopt);
    EXPECT_THROW((void) exe.u8(0x400000), Exception);
}

UNIT_TEST(MM6Executable, CreationArrays) {
    // Every class family starts with 73 stat points, two skills granted and nine more to choose from.
    const std::unique_ptr<MM6TestData> data = MM6TestData::open();
    if (!data)
        GTEST_SKIP() << mm6TestDataMissing;
    mm6::loadCreationRules(mm6::Executable(data->read("mm6.exe")));
    const mm6::CreationRules &rules = mm6::creationRules();
    for (int family = 0; family < mm6::kClassFamilyCount; ++family) {
        SCOPED_TRACE(family);
        EXPECT_EQ(std::ranges::fold_left(rules.startingStats[family], 0, std::plus()), 73);
        EXPECT_EQ(std::ranges::count(rules.startingSkills[family], 1), 2);
        EXPECT_EQ(std::ranges::count(rules.startingSkills[family], 2), 9);
        EXPECT_EQ(std::ranges::count_if(rules.startingSkills[family], [](int code) { return code > 3; }), 0);
    }
    EXPECT_EQ(rules.hpBase[0], 30); // Knight.
    EXPECT_EQ(rules.spFactor[0], 0);
}
