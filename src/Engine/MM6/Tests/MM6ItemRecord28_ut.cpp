#include <array>
#include <cstdint>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6ItemRecord28.h"
#include "Engine/MM6/MM6ShopStockFields.h"

UNIT_TEST(MM6ItemRecord28, PreservesEveryByteAndRawFields) {
    mm6::ItemRecord28 record;
    for (std::size_t i = 0; i < record.size(); ++i)
        record[i] = static_cast<std::uint8_t>(i * 7 + 3);
    const std::array<std::uint8_t, 28> original = record.bytes;

    EXPECT_EQ(record.itemNumberRaw(), 0x18110a03u);
    EXPECT_EQ(record.bonusRaw(), 0x342d261fu);
    EXPECT_EQ(record.bonusStrengthRaw(), 0x5049423bu);
    EXPECT_EQ(record.secondBonusRaw(), 0x6c655e57u);
    EXPECT_EQ(record.chargesRaw(), 0x88817a73u);
    EXPECT_EQ(record.conditionRaw(), 0xa49d968fu);
    EXPECT_EQ(record.bodyLocationRaw(), original[24]);
    EXPECT_EQ(record.maxChargesRaw(), original[25]);
    EXPECT_EQ(record.ownerByteRaw(), original[26]);
    EXPECT_EQ(record.paddingByteRaw(), original[27]);

    record.setWord(12, 0x44332211u);
    EXPECT_EQ(record.word(12), 0x44332211u);
    EXPECT_EQ(record.bytes[11], original[11]);
    EXPECT_EQ(record.bytes[16], original[16]);
    for (std::size_t i = 24; i < 28; ++i)
        EXPECT_EQ(record.bytes[i], original[i]);
    record.setWord(12, 0x6c655e57u);
    EXPECT_EQ(record.bytes, original);

    // SHOP v1 continues to serialize the complete raw value, including tail.
    mm6::ShopStockFields stock;
    stock.ready = true;
    stock.records[0] = record;
    mm6::SaveContainer container;
    container.sections.push_back(mm6::encodeShopStockSection(stock));
    const auto decoded = mm6::decodeShopStockSection(
        mm6::decodeSaveContainer(mm6::encodeSaveContainer(container)));
    EXPECT_EQ(decoded.records[0], record);
    EXPECT_EQ(decoded.records[0].bytes, original);
}

UNIT_TEST(MM6ItemRecord28, RejectsUnalignedOrOutOfBoundsWordOffsets) {
    mm6::ItemRecord28 record;
    EXPECT_THROW((void) record.word(1), std::out_of_range);
    EXPECT_THROW((void) record.word(28), std::out_of_range);
    EXPECT_THROW(record.setWord(25, 1), std::out_of_range);
    EXPECT_EQ(record, mm6::ItemRecord28{});
}
