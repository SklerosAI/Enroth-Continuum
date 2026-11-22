#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6ChestRerollSection.h"

static mm6::ChestRerollLayouts twoChests() {
    mm6::ChestRerollLayouts layouts;
    mm6::ChestRerollLayout layout = {};
    layout[0] = -3;
    layout[1] = 1;
    layouts[{"d02.blv", 1}] = layout;
    layout[0] = -7;
    layouts[{"oute3.odm", 19}] = layout;
    return layouts;
}

UNIT_TEST(MM6ChestRerollSection, RoundTripsTheChests) {
    // The rerollable chests come back from a save exactly as the party left them.
    mm6::SaveContainer container;
    container.sections.push_back(mm6::encodeChestRerollSection(twoChests(), GameTitle::MM6).value());

    EXPECT_FALSE(container.sections[0].required);
    EXPECT_EQ(mm6::decodeChestRerollSection(mm6::decodeSaveContainer(mm6::encodeSaveContainer(container)), GameTitle::MM6), twoChests());
}

UNIT_TEST(MM6ChestRerollSection, NoRerollableChestIsNotSaved) {
    // A save of a game that never ran the chest reroll mod keeps the bytes it had before the mod existed.
    EXPECT_EQ(mm6::encodeChestRerollSection({}, GameTitle::MM6), std::nullopt);
    EXPECT_EQ(mm6::decodeChestRerollSection(mm6::SaveContainer(), GameTitle::MM6), mm6::ChestRerollLayouts());
}

UNIT_TEST(MM6ChestRerollSection, RefusesMalformedSections) {
    // A damaged section refuses the save rather than loading it with the wrong chests rerollable.
    const mm6::SaveSection valid = *mm6::encodeChestRerollSection(twoChests(), GameTitle::MM6);
    const std::size_t secondEntry = 3 + 1 + 7 + 1 + 140;

    auto refused = [&](auto change) {
        mm6::SaveContainer container;
        container.sections.push_back(valid);
        change(&container.sections[0]);
        EXPECT_ANY_THROW((void) mm6::decodeChestRerollSection(container, GameTitle::MM6));
    };
    refused([](mm6::SaveSection *section) { section->payload.pop_back(); });
    refused([](mm6::SaveSection *section) { section->payload.push_back(0); });
    refused([](mm6::SaveSection *section) { section->payload[0] = 2; }); // Version.
    refused([](mm6::SaveSection *section) { section->payload[1] = 0; }); // No chest.
    refused([](mm6::SaveSection *section) { section->payload[4] = 'D'; }); // Map name not canonical.
    refused([](mm6::SaveSection *section) { section->payload[11] = 20; }); // Chest index.
    refused([](mm6::SaveSection *section) { section->payload[12] = 0xf8; }); // Marker -8.
    refused([](mm6::SaveSection *section) { section->payload[12] = 0; }); // No marker left.
    refused([&](mm6::SaveSection *section) { section->payload[secondEntry + 1] = 'a'; }); // The second map now sorts before the first.
    refused([](mm6::SaveSection *section) { section->required = true; });

    mm6::SaveContainer duplicate;
    duplicate.sections = {valid, valid};
    EXPECT_ANY_THROW((void) mm6::decodeChestRerollSection(duplicate, GameTitle::MM6));

    EXPECT_ANY_THROW((void) mm6::encodeChestRerollSection(twoChests(), GameTitle::MM7));
    mm6::ChestRerollLayouts noMarker = {{{"d02.blv", 1}, {}}};
    EXPECT_ANY_THROW((void) mm6::encodeChestRerollSection(noMarker, GameTitle::MM6));
}
