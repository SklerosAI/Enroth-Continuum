#include <cstdint>
#include <exception>
#include <memory>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6ShopStockFields.h"
#include "Engine/MM6/MM6ShopStockIdentity.h"

UNIT_TEST(MM6ShopStockFields, PhysicalAliasAndTailRoundTrip) {
    auto live = std::make_unique<mm6::ShopStockFields>();
    live->ready = true;
    const std::size_t shop = mm6::shopStockRecordIndex(mm6::ShopStockBank::Second, 47, 5, GameTitle::MM6);
    const std::size_t guild = mm6::guildStockRecordIndex(119, 5, GameTitle::MM6);
    ASSERT_EQ(shop, guild);
    for (std::size_t i = 0; i < 28; ++i)
        live->records[shop][i] = static_cast<std::uint8_t>(i + 1);
    live->records.back()[27] = 0xa5; // Preserve initialized physical rows beyond active guild keys.

    const mm6::ShopStockFields captured = mm6::captureShopStock(*live, GameTitle::MM6);
    mm6::SaveContainer container;
    container.sections.push_back(mm6::encodeShopStockSection(captured));
    EXPECT_EQ(container.sections[0].payload.size(), 1 + 120 * 12 * 28);
    const std::vector<std::uint8_t> bytes = mm6::encodeSaveContainer(container);
    const mm6::ShopStockFields decoded = mm6::decodeShopStockSection(mm6::decodeSaveContainer(bytes));
    EXPECT_EQ(decoded, *live);
    EXPECT_EQ(decoded.records[guild][0], 1);
    EXPECT_EQ(decoded.records.back()[27], 0xa5);
}

UNIT_TEST(MM6ShopStockFields, RejectsMalformedOrForeignBeforeMutation) {
    auto live = std::make_unique<mm6::ShopStockFields>();
    live->ready = true;
    live->records[0][0] = 71;
    mm6::SaveContainer container;
    container.sections.push_back(mm6::encodeShopStockSection(*live));
    const std::vector<std::uint8_t> valid = mm6::encodeSaveContainer(container);
    auto prior = std::make_unique<mm6::ShopStockFields>();
    prior->ready = true;
    prior->records[0][0] = 9;

    EXPECT_THROW(mm6::loadShopStockBeforeMutation(valid, prior.get(), GameTitle::MM7), std::exception);
    EXPECT_EQ(prior->records[0][0], 9);
    EXPECT_THROW((void) mm6::captureShopStock(*live, GameTitle::MM7), std::exception);
    live->ready = false;
    EXPECT_THROW((void) mm6::captureShopStock(*live, GameTitle::MM6), std::exception);
    EXPECT_THROW((void) mm6::encodeShopStockSection(*live), std::exception);

    container.sections[0].payload.pop_back(); // Valid container checksum, short final record.
    EXPECT_THROW(mm6::loadShopStockBeforeMutation(mm6::encodeSaveContainer(container),
                                                 prior.get(), GameTitle::MM6), std::exception);
    EXPECT_EQ(prior->records[0][0], 9);
    container.sections[0].payload.push_back(0);
    container.sections[0].payload[0] = 2; // Unsupported SHOP section version.
    EXPECT_THROW(mm6::loadShopStockBeforeMutation(mm6::encodeSaveContainer(container),
                                                 prior.get(), GameTitle::MM6), std::exception);
    EXPECT_EQ(prior->records[0][0], 9);
    container.sections[0].payload[0] = 1;
    container.sections[0].required = false;
    EXPECT_THROW(mm6::loadShopStockBeforeMutation(mm6::encodeSaveContainer(container),
                                                 prior.get(), GameTitle::MM6), std::exception);
    EXPECT_EQ(prior->records[0][0], 9);
}
