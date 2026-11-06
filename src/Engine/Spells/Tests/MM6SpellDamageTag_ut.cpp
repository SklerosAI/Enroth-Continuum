#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <utility>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6SpellDamageTag.h"
#include "Engine/Spells/Spells.h"

#include "Utility/Memory/Blob.h"

UNIT_TEST(MM6SpellDamageTag, DistinguishesAllSourceLabels) {
    using mm6::SpellDamageTag;
    EXPECT_EQ(mm6::parseSpellDamageTag("none"), SpellDamageTag::None);
    EXPECT_EQ(mm6::parseSpellDamageTag("Phys"), SpellDamageTag::Physical);
    EXPECT_EQ(mm6::parseSpellDamageTag("Fire"), SpellDamageTag::Fire);
    EXPECT_EQ(mm6::parseSpellDamageTag("Elec"), SpellDamageTag::Electricity);
    EXPECT_EQ(mm6::parseSpellDamageTag("Cold"), SpellDamageTag::Cold);
    EXPECT_EQ(mm6::parseSpellDamageTag("Pois"), SpellDamageTag::Poison);
    EXPECT_EQ(mm6::parseSpellDamageTag("Magic"), SpellDamageTag::Magic);
    EXPECT_THROW(mm6::parseSpellDamageTag("unknown"), std::runtime_error);
}

UNIT_TEST(MM6SpellDamageTag, RetainsAllF3MatchedSpellSourceTags) {
    const char *research = std::getenv("ENROTH_MM6_F3_RESEARCH_DIR");
    if (!research)
        GTEST_SKIP() << "Set ignored MM6 F3 research directory";
    const std::filesystem::path path = std::filesystem::path(research) / "icons/spells.txt";
    std::ifstream stream(path, std::ios::binary);
    ASSERT_TRUE(stream.is_open());
    std::string data(std::istreambuf_iterator<char>{stream}, {});
    SpellStats stats;
    stats.Initialize(Blob::fromString(std::move(data)), GameTitle::MM6);

    std::array<int, 7> counts{};
    for (int rawId = 1; rawId <= 99; ++rawId) {
        const SpellInfo &info = stats.pInfos[static_cast<SpellId>(rawId)];
        ASSERT_TRUE(info.mm6DamageTag.has_value()) << "spell ID " << rawId;
        ++counts[static_cast<size_t>(*info.mm6DamageTag)];
    }
    EXPECT_EQ(counts, (std::array<int, 7>{50, 7, 9, 4, 3, 4, 22}));
}
