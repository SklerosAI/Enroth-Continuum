#include <array>
#include <cstring>
#include <string>
#include <utility>

#include "Testing/Unit/UnitTest.h"

#include "Engine/GameTitle.h"
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
