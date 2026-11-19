#include <array>
#include <stdexcept>

#include "Testing/Unit/MM6TestData.h"
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

UNIT_TEST(MM6SpellDamageTag, RetainsAllShippedSpellSourceTags) {
    const auto data = MM6TestData::open();
    if (!data)
        GTEST_SKIP() << mm6TestDataMissing;
    SpellStats stats;
    stats.Initialize(data->iconsData("spells.txt"), GameTitle::MM6);

    std::array<int, 7> counts{};
    for (int rawId = 1; rawId <= 99; ++rawId) {
        const SpellInfo &info = stats.pInfos[static_cast<SpellId>(rawId)];
        ASSERT_TRUE(info.mm6DamageTag.has_value()) << "spell ID " << rawId;
        ++counts[static_cast<size_t>(*info.mm6DamageTag)];
    }
    EXPECT_EQ(counts, (std::array<int, 7>{50, 7, 9, 4, 3, 4, 22}));
}
