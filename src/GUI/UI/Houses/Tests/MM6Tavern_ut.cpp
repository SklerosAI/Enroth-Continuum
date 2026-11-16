#include <utility>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "GUI/UI/Houses/MM6Tavern.h"

UNIT_TEST(MM6Tavern, RoomAndFoodPrices) {
    // A room costs a tenth of Val squared and food a hundredth of Val cubed, truncated after float math, then merchant
    // takes off down to a third.
    EXPECT_EQ(mm6::tavernRoomPrice(6, 0), 3);
    EXPECT_EQ(mm6::tavernFoodPrice(6, 0), 2);
    EXPECT_EQ(mm6::tavernFoodPrice(10, 0), 9);
    EXPECT_EQ(mm6::tavernRoomPrice(20, 0), 40);
    EXPECT_EQ(mm6::tavernRoomPrice(20, 10), 36);
    EXPECT_EQ(mm6::tavernRoomPrice(20, 90), 13);
}

UNIT_TEST(MM6Tavern, RoomRest) {
    // A rented room lets the party sleep until 6 am, the next day's when rented after it.
    EXPECT_EQ(mm6::tavernRoomRestMinutes(9, 0), 21 * 60);
    EXPECT_EQ(mm6::tavernRoomRestMinutes(23, 30), 6 * 60 + 30);
    EXPECT_EQ(mm6::tavernRoomRestMinutes(0, 0), 6 * 60);
    EXPECT_EQ(mm6::tavernRoomRestMinutes(4, 59), 61);
    EXPECT_EQ(mm6::tavernRoomRestMinutes(5, 0), 25 * 60);
}

UNIT_TEST(MM6Tavern, TipNews) {
    // A barkeep tells the news of the map's region, and region 1's in a region without news.
    const std::vector<int> regions = {40, 40, 39, 1, 1, 1};
    EXPECT_EQ(mm6::tavernTipNews(regions, 40), std::pair(0, 2));
    EXPECT_EQ(mm6::tavernTipNews(regions, 39), std::pair(2, 1));
    EXPECT_EQ(mm6::tavernTipNews(regions, 12), std::pair(3, 3));
    EXPECT_EQ(mm6::tavernNewsRegions("Regional News\r\n#\tMap\tTopic\tNews Text\r\n1\t40\tA\tB\r\n2\t1\tC\tD\r\n"), std::vector<int>({40, 1}));
}
