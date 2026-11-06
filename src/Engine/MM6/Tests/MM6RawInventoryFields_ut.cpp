#include <cstdint>
#include <exception>
#include <memory>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6RawInventoryFields.h"

UNIT_TEST(MM6RawInventoryFields, PreservesAllPhysicalRecordsAndIndexWords) {
    auto source = std::make_unique<mm6::RawInventoryFields>();
    source->ready = true;
    for (std::size_t character = 0; character < source->characters.size(); ++character) {
        auto &inventory = source->characters[character];
        for (std::size_t slot = 0; slot < inventory.items.size(); ++slot)
            for (std::size_t byte = 0; byte < inventory.items[slot].size(); ++byte)
                inventory.items[slot][byte] = static_cast<std::uint8_t>(character + slot + 3 * byte);
        for (std::size_t cell = 0; cell < inventory.gridWords.size(); ++cell)
            inventory.gridWords[cell] = 0x80000000u | (character << 16) | cell;
        for (std::size_t slot = 0; slot < inventory.equipmentWords.size(); ++slot)
            inventory.equipmentWords[slot] = 0x40000000u | (character << 8) | slot;
    }
    source->characters[3].equipmentWords[15] = 138; // Preserve the physical index without interpreting it.
    const auto captured = mm6::captureRawInventory(*source, GameTitle::MM6);
    const mm6::SaveSection section = mm6::encodeRawInventorySection(captured);
    EXPECT_TRUE(section.required);
    EXPECT_EQ(section.payload.size(), 17732);
    const auto bytes = mm6::encodeSaveContainer({{section}});
    const auto decoded = mm6::decodeRawInventorySection(mm6::decodeSaveContainer(bytes));
    EXPECT_EQ(decoded, *source);
    EXPECT_EQ(decoded.characters[3].items[137].bytes,
              source->characters[3].items[137].bytes);
    EXPECT_EQ(decoded.characters[3].equipmentWords[15], 138);
}

UNIT_TEST(MM6RawInventoryFields, RejectsMalformedOrForeignBeforeMutation) {
    auto source = std::make_unique<mm6::RawInventoryFields>();
    source->ready = true;
    source->characters[3].items[137][27] = 0xa5;
    mm6::SaveContainer container{{mm6::encodeRawInventorySection(*source)}};
    const auto valid = mm6::encodeSaveContainer(container);
    auto destination = std::make_unique<mm6::RawInventoryFields>();
    destination->ready = true;
    destination->characters[0].items[0][0] = 73;
    const auto prior = *destination;

    EXPECT_THROW(mm6::loadRawInventoryBeforeMutation(valid, destination.get(), GameTitle::MM7),
                 std::exception);
    EXPECT_EQ(*destination, prior);
    EXPECT_THROW((void) mm6::captureRawInventory(*source, GameTitle::MM7), std::exception);
    source->ready = false;
    EXPECT_THROW((void) mm6::encodeRawInventorySection(*source), std::exception);

    container.sections[0].payload.pop_back();
    EXPECT_THROW(mm6::loadRawInventoryBeforeMutation(mm6::encodeSaveContainer(container),
                                                     destination.get(), GameTitle::MM6), std::exception);
    EXPECT_EQ(*destination, prior);
    container.sections[0].payload.push_back(0);
    container.sections[0].payload[0] = 2;
    EXPECT_THROW(mm6::loadRawInventoryBeforeMutation(mm6::encodeSaveContainer(container),
                                                     destination.get(), GameTitle::MM6), std::exception);
    EXPECT_EQ(*destination, prior);
    container.sections[0].payload[0] = 1;
    container.sections[0].required = false;
    EXPECT_THROW(mm6::loadRawInventoryBeforeMutation(mm6::encodeSaveContainer(container),
                                                     destination.get(), GameTitle::MM6), std::exception);
    EXPECT_EQ(*destination, prior);

    std::vector<std::uint8_t> corrupt = valid;
    corrupt.back() ^= 1;
    EXPECT_THROW(mm6::loadRawInventoryBeforeMutation(corrupt, destination.get(), GameTitle::MM6),
                 std::exception);
    EXPECT_EQ(*destination, prior);
}
