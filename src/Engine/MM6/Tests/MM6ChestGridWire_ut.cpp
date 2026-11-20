#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6ChestGridWire.h"
#include "Engine/MM6/MM6MapSaveFields.h"

UNIT_TEST(MM6ChestGridWire, RoundTripsAllSignedCellsAndPreservesOtherBytes) {
    std::array<std::uint8_t, mm6::CHEST_WIRE_SIZE> source{};
    for (std::size_t i = 0; i < mm6::CHEST_GRID_OFFSET; ++i)
        source[i] = static_cast<std::uint8_t>(i % 251);
    std::array<std::int16_t, mm6::CHEST_GRID_CELLS> grid{};
    grid[0] = 1;
    grid[80] = -1;
    grid[81] = 82;
    grid[139] = -140;
    auto encoded = mm6::encodeChestGridWire(GameTitle::MM6, source, grid);
    ASSERT_TRUE(encoded);
    EXPECT_EQ((*encoded)[mm6::CHEST_GRID_OFFSET + 2 * 81], 82);
    EXPECT_EQ((*encoded)[mm6::CHEST_GRID_OFFSET + 2 * 139], 0x74);
    EXPECT_EQ((*encoded)[mm6::CHEST_GRID_OFFSET + 2 * 139 + 1], 0xff);
    EXPECT_TRUE(std::equal(source.begin(), source.begin() + mm6::CHEST_GRID_OFFSET,
                           encoded->begin()));
    auto decoded = mm6::decodeChestGridWire(GameTitle::MM6, *encoded);
    ASSERT_TRUE(decoded);
    EXPECT_EQ(*decoded, grid);

    grid[139] = std::numeric_limits<std::int16_t>::min();
    encoded = mm6::encodeChestGridWire(GameTitle::MM6, *encoded, grid);
    ASSERT_TRUE(encoded);
    EXPECT_EQ(mm6::decodeChestGridWire(GameTitle::MM6, *encoded), grid);
}

UNIT_TEST(MM6ChestGridWire, RejectsForeignAndWrongLength) {
    std::array<std::uint8_t, mm6::CHEST_WIRE_SIZE> source{};
    std::array<std::int16_t, mm6::CHEST_GRID_CELLS> grid{};
    EXPECT_FALSE(mm6::decodeChestGridWire(GameTitle::MM7, source));
    EXPECT_FALSE(mm6::encodeChestGridWire(GameTitle::MM7, source, grid));
    EXPECT_FALSE(mm6::decodeChestGridWire(GameTitle::MM6,
                                         std::span(source).first(source.size() - 1)));
    EXPECT_FALSE(mm6::encodeChestGridWire(GameTitle::MM6,
                                         std::span(source).first(source.size() - 1), grid));
    EXPECT_FALSE(mm6::decodeChestGridWire(GameTitle::MM6, {}));
}

UNIT_TEST(MM6ChestGridWire, ChangedGridSurvivesMapSectionRoundTrip) {
    std::array<std::uint8_t, mm6::CHEST_WIRE_SIZE> source{};
    source[0] = 9;
    source[mm6::CHEST_GRID_OFFSET - 1] = 0xa5;
    std::array<std::int16_t, mm6::CHEST_GRID_CELLS> grid{};
    grid[0] = 1;
    grid[94] = -95;
    grid[139] = 140;
    auto changed = mm6::encodeChestGridWire(GameTitle::MM6, source, grid);
    ASSERT_TRUE(changed);
    mm6::MapSaveFields fields{{{"test.dlv", {changed->begin(), changed->end()}}}};
    mm6::SaveContainer container{{mm6::encodeMapSection(fields)}};
    auto restored = mm6::decodeMapSection(container);
    ASSERT_EQ(restored.deltas.size(), 1);
    EXPECT_EQ(restored.deltas[0].bytes,
              std::vector<std::uint8_t>(changed->begin(), changed->end()));
    EXPECT_EQ(mm6::decodeChestGridWire(GameTitle::MM6, restored.deltas[0].bytes), grid);
    EXPECT_EQ(restored.deltas[0].bytes[0], 9);
    EXPECT_EQ(restored.deltas[0].bytes[mm6::CHEST_GRID_OFFSET - 1], 0xa5);
}

UNIT_TEST(MM6ChestGridWire, LaterPagesRoundTripThroughTheCellsPastTheFirstPage) {
    // Items on later pages take one cell each past the first page, and the first page stays as it was. The first item
    // of each page after the second is negated.
    std::array<std::int16_t, mm6::CHEST_GRID_CELLS> grid{};
    grid[0] = 3;
    grid[1] = -1;
    const std::vector<mm6::ChestPageItem> pages = {{1, 0, 0}, {1, 139, 80}, {2, 81, 40}, {3, 5, 0}, {3, 6, 9}};
    const auto encoded = mm6::encodeChestPages(grid, mm6::CHEST_PAGE_CELLS, pages);
    ASSERT_TRUE(encoded);
    EXPECT_TRUE(std::equal(grid.begin(), grid.begin() + 81, encoded->begin()));
    EXPECT_EQ(std::vector(encoded->begin() + 81, encoded->begin() + 86),
              (std::vector<std::int16_t>{1, 11340, -(1 + 81 + 140 * 40), -6, 1 + 6 + 140 * 9}));
    EXPECT_TRUE(std::all_of(encoded->begin() + 86, encoded->end(), [](std::int16_t cell) { return cell == 0; }));
    EXPECT_EQ(mm6::decodeChestPages(*encoded, mm6::CHEST_PAGE_CELLS), pages);
    EXPECT_EQ(mm6::decodeChestPages(grid, mm6::CHEST_PAGE_CELLS), std::vector<mm6::ChestPageItem>{});
}

UNIT_TEST(MM6ChestGridWire, LaterPagesRejectWhatTheCellsCantHold) {
    // A list that skips a page or goes back one, a cell past the page, more items than there are cells past the first
    // page, a gap, and a first item that starts a page.
    std::array<std::int16_t, mm6::CHEST_GRID_CELLS> grid{};
    EXPECT_FALSE(mm6::encodeChestPages(grid, mm6::CHEST_PAGE_CELLS, (std::vector<mm6::ChestPageItem>{{2, 0, 0}})));
    EXPECT_FALSE(mm6::encodeChestPages(grid, mm6::CHEST_PAGE_CELLS, (std::vector<mm6::ChestPageItem>{{1, 0, 0}, {3, 1, 0}})));
    EXPECT_FALSE(mm6::encodeChestPages(grid, mm6::CHEST_PAGE_CELLS, (std::vector<mm6::ChestPageItem>{{1, 0, 0}, {2, 1, 0}, {1, 2, 0}})));
    EXPECT_FALSE(mm6::encodeChestPages(grid, mm6::CHEST_PAGE_CELLS, (std::vector<mm6::ChestPageItem>{{1, 0, 81}})));
    EXPECT_FALSE(mm6::encodeChestPages(grid, mm6::CHEST_PAGE_CELLS, std::vector<mm6::ChestPageItem>(60, mm6::ChestPageItem{1, 0, 0})));
    EXPECT_TRUE(mm6::encodeChestPages(grid, mm6::CHEST_PAGE_CELLS, std::vector<mm6::ChestPageItem>(59, mm6::ChestPageItem{1, 0, 0})));

    grid[81] = 1 + 140 * 81;
    EXPECT_FALSE(mm6::decodeChestPages(grid, mm6::CHEST_PAGE_CELLS));
    grid[81] = 1;
    grid[83] = 1;
    EXPECT_FALSE(mm6::decodeChestPages(grid, mm6::CHEST_PAGE_CELLS));
    grid[83] = 0;
    grid[81] = -1;
    EXPECT_FALSE(mm6::decodeChestPages(grid, mm6::CHEST_PAGE_CELLS));
}
