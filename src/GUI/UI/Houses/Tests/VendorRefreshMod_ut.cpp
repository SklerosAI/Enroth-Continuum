#include "Testing/Unit/UnitTest.h"

#include "GUI/UI/Houses/VendorRefreshMod.h"

UNIT_TEST(VendorRefreshMod, OncePerMM7VisitWithExactOptIn) {
    VendorRefreshVisit firstVisit;
    EXPECT_FALSE(firstVisit.takeMM7EntryRefresh(GameTitle::MM7, nullptr));
    EXPECT_FALSE(firstVisit.takeMM7EntryRefresh(GameTitle::MM7, "0"));
    EXPECT_FALSE(firstVisit.takeMM7EntryRefresh(GameTitle::MM7, "true"));
    EXPECT_TRUE(firstVisit.takeMM7EntryRefresh(GameTitle::MM7, "1"));
    EXPECT_FALSE(firstVisit.takeMM7EntryRefresh(GameTitle::MM7, "1"));

    VendorRefreshVisit secondVisit;
    EXPECT_TRUE(secondVisit.takeMM7EntryRefresh(GameTitle::MM7, "1"));
}

UNIT_TEST(VendorRefreshMod, NeverRefreshesMM6FromMM7Flag) {
    VendorRefreshVisit visit;
    EXPECT_FALSE(visit.takeMM7EntryRefresh(GameTitle::MM6, "1"));
    EXPECT_TRUE(visit.takeMM7EntryRefresh(GameTitle::MM7, "1"));
}
