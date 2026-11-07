#include <stdexcept>

#include "Testing/Unit/UnitTest.h"

#include "Engine/Evt/MM6HirelingEvent.h"
#include "Engine/Party.h"
#include "Engine/Tables/NPCTable.h"

UNIT_TEST(MM6HirelingEvent, HireAndDismissOneTableNpc) {
    // MM6 variable 214 hires and dismisses one NPC from the table.
    NPCStats stats;
    stats.pNPCData[12].flags = NPC_GREETED_FIRST;
    EXPECT_FALSE(mm6::isEventNpcHired(stats, 12));

    mm6::setEventNpcHired(stats, 12, true);
    EXPECT_TRUE(mm6::isEventNpcHired(stats, 12));
    EXPECT_TRUE(static_cast<bool>(stats.pNPCData[12].flags & NPC_GREETED_FIRST));
    EXPECT_FALSE(mm6::isEventNpcHired(stats, 11));

    mm6::setEventNpcHired(stats, 12, false);
    EXPECT_FALSE(mm6::isEventNpcHired(stats, 12));
    EXPECT_TRUE(static_cast<bool>(stats.pNPCData[12].flags & NPC_GREETED_FIRST));

    EXPECT_THROW(mm6::setEventNpcHired(stats, 0, true), std::runtime_error);
    EXPECT_THROW(mm6::isEventNpcHired(stats, static_cast<int>(stats.pNPCData.size())), std::runtime_error);
}

UNIT_TEST(MM6HirelingEvent, DismissProfessionClearsTableNpcsAndHirelingSlots) {
    // MM6 variable 223 dismisses everyone with the profession, both table NPCs and hired hirelings.
    NPCStats stats;
    stats.uNumNewNPCs = 20;
    stats.pNPCData[3].profession = static_cast<NpcProfession>(63);
    stats.pNPCData[3].flags = NPC_HIRED;
    stats.pNPCData[4].profession = static_cast<NpcProfession>(64);
    stats.pNPCData[4].flags = NPC_HIRED;
    stats.pNPCData[25].profession = static_cast<NpcProfession>(63);
    stats.pNPCData[25].flags = NPC_HIRED;

    Party party;
    party.pHirelings[0].name = "first";
    party.pHirelings[0].profession = static_cast<NpcProfession>(63);
    party.pHirelings[1].name = "second";
    party.pHirelings[1].profession = static_cast<NpcProfession>(64);

    mm6::dismissEventProfession(stats, party, 63);
    EXPECT_FALSE(static_cast<bool>(stats.pNPCData[3].flags & NPC_HIRED));
    EXPECT_TRUE(static_cast<bool>(stats.pNPCData[4].flags & NPC_HIRED));
    EXPECT_TRUE(static_cast<bool>(stats.pNPCData[25].flags & NPC_HIRED)); // Past the table's live NPC count.
    EXPECT_TRUE(party.pHirelings[0].name.empty());
    EXPECT_EQ(party.pHirelings[0].profession, NPC_PROFESSION_NONE);
    EXPECT_EQ(party.pHirelings[1].name, "second");

    EXPECT_THROW(mm6::dismissEventProfession(stats, party, 0), std::runtime_error);
    EXPECT_THROW(mm6::dismissEventProfession(stats, party, 78), std::runtime_error);
}
