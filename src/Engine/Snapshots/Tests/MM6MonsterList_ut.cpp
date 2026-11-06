#include <array>
#include <cstring>
#include <string>
#include <utility>

#include "Testing/Unit/UnitTest.h"

#include "Engine/GameTitle.h"
#include "Engine/MM6/MM6MonsterPreferences.h"
#include "Engine/Objects/Monsters.h"
#include "Engine/Snapshots/EntitySnapshots.h"
#include "Engine/Snapshots/TableSerialization.h"

#include "Utility/Exception.h"
#include "Utility/Memory/Blob.h"

UNIT_TEST(MM6MonsterList, RetainsLastDescriptor) {
    std::array<MonsterDesc_MM6, 173> records{};
    records.front().monsterHeight = 101;
    records.back().monsterHeight = 173;

    std::string data(4 + sizeof(records), '\0');
    data[0] = static_cast<char>(records.size());
    std::memcpy(data.data() + 4, records.data(), sizeof(records));

    MonsterList list;
    deserialize(Blob::fromString(std::move(data)), &list, GameTitle::MM6);

    EXPECT_EQ(list.monsters[MONSTER_ANGEL_A].monsterHeight, 101);
    EXPECT_EQ(list.monsters[static_cast<MonsterId>(173)].monsterHeight, 173);
}

UNIT_TEST(MM6MonsterList, RejectsTruncatedDescriptorTable) {
    std::array<MonsterDesc_MM6, 172> records{};
    std::string data(4 + sizeof(records), '\0');
    data[0] = static_cast<char>(173);
    std::memcpy(data.data() + 4, records.data(), sizeof(records));
    MonsterList list;

    EXPECT_THROW(deserialize(Blob::fromString(std::move(data)), &list, GameTitle::MM6), Exception);
}

UNIT_TEST(MM6MonsterList, ClassAttackPreferencesUseThreeTierFamilies) {
    for (int family = 0; family < mm6::kClassFamilyCount; ++family) {
        MonsterAttackPreference preference = ATTACK_PREFERENCE_KNIGHT;
        switch (family) {
            case 0: preference = ATTACK_PREFERENCE_KNIGHT; break;
            case 1: preference = ATTACK_PREFERENCE_CLERIC; break;
            case 2: preference = ATTACK_PREFERENCE_SORCERER; break;
            case 3: preference = ATTACK_PREFERENCE_PALADIN; break;
            case 4: preference = ATTACK_PREFERENCE_ARCHER; break;
            case 5: preference = ATTACK_PREFERENCE_DRUID; break;
        }
        const int base = family * mm6::kClassTiers;
        EXPECT_TRUE(mm6::matchesClassAttackPreference(base, preference, false));
        EXPECT_TRUE(mm6::matchesClassAttackPreference(base + 1, preference, true));
        EXPECT_TRUE(mm6::matchesClassAttackPreference(base + 2, preference, true));
        EXPECT_FALSE(mm6::matchesClassAttackPreference(base + 1, preference, false));
    }
    EXPECT_FALSE(mm6::matchesClassAttackPreference(0, ATTACK_PREFERENCE_MONK, true));
    EXPECT_FALSE(mm6::matchesClassAttackPreference(0, ATTACK_PREFERENCE_RANGER, true));
    EXPECT_FALSE(mm6::matchesClassAttackPreference(18, ATTACK_PREFERENCE_KNIGHT, true));
}

UNIT_TEST(MM6MonsterList, MalePreferenceTokenIsTitleScoped) {
    EXPECT_EQ(mm6::mAttackPreference(GameTitle::MM6), ATTACK_PREFERENCE_MALE);
    EXPECT_EQ(mm6::mAttackPreference(GameTitle::MM7), ATTACK_PREFERENCE_MONK);
}
