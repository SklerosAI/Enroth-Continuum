#include <cstddef>
#include <cstdint>
#include <exception>
#include <string>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6CoreSaveFields.h"

namespace {

mm6::CoreSaveFields exampleFields() {
    mm6::CoreSaveFields fields;
    fields.party.gold = 1000;
    fields.party.food = 12;
    fields.party.bankGold = 37;
    fields.party.playingTicks = 987654;
    fields.party.position = {123.5f, -40.0f, 256.25f};
    fields.party.viewYaw = 512;
    fields.party.viewPitch = -21;
    fields.party.activeCharacter = 2;
    for (size_t i = 0; i < fields.characters.size(); ++i) {
        auto &character = fields.characters[i];
        character.name = "Hero" + std::to_string(i);
        character.classId = i * 3;
        character.faceId = i;
        character.voiceId = i;
        character.level = i + 1;
        character.experience = 10000 + i;
        character.health = 24 - i;
        character.mana = 12 + i;
        character.skillPoints = i;
        character.baseStats[0] = 13 + i;
        character.skills[0] = 0x82; // Master 2.
        character.spells[0] = true;
        character.spells[98] = true;
    }
    return fields;
}

mm6::SaveContainer containerFor(const mm6::CoreSaveFields &fields) {
    auto sections = mm6::encodeCoreSections(fields);
    mm6::SaveContainer container;
    container.sections.assign(sections.begin(), sections.end());
    return container;
}

} // namespace

UNIT_TEST(MM6CoreSaveFields, NonzeroPartyAndFourCharacterRoundTrip) {
    mm6::CoreSaveFields source = exampleFields();
    std::vector<std::uint8_t> bytes = mm6::encodeSaveContainer(containerFor(source));
    EXPECT_EQ(mm6::decodeCoreSections(mm6::decodeSaveContainer(bytes)), source);

    mm6::CoreSaveFields destination;
    mm6::loadCoreFieldsBeforeMutation(bytes, &destination);
    EXPECT_EQ(destination, source);
}

UNIT_TEST(MM6CoreSaveFields, InvalidFieldLeavesDestinationUntouched) {
    mm6::CoreSaveFields destination = exampleFields();
    mm6::CoreSaveFields before = destination;
    mm6::SaveContainer invalid = containerFor(before);
    invalid.sections[1].payload[3 + before.characters[0].name.size() + 1] = 255; // Class ID.
    EXPECT_THROW(mm6::loadCoreFieldsBeforeMutation(mm6::encodeSaveContainer(invalid), &destination), std::exception);
    EXPECT_EQ(destination, before);

    invalid = containerFor(before);
    invalid.sections[0].payload[1] = 255; // Negative gold with a valid container checksum.
    invalid.sections[0].payload[2] = 255;
    invalid.sections[0].payload[3] = 255;
    invalid.sections[0].payload[4] = 255;
    EXPECT_THROW(mm6::loadCoreFieldsBeforeMutation(mm6::encodeSaveContainer(invalid), &destination), std::exception);
    EXPECT_EQ(destination, before);
}

UNIT_TEST(MM6CoreSaveFields, RejectsMissingMalformedAndForeignSections) {
    mm6::CoreSaveFields fields = exampleFields();
    mm6::SaveContainer container = containerFor(fields);
    container.sections.pop_back();
    EXPECT_THROW((void) mm6::decodeCoreSections(container), std::exception);

    container = containerFor(fields);
    container.sections[1].payload.pop_back();
    EXPECT_THROW((void) mm6::decodeCoreSections(container), std::exception);

    container = containerFor(fields);
    container.sections[0].required = false;
    EXPECT_THROW((void) mm6::decodeCoreSections(container), std::exception);

    std::vector<std::uint8_t> bytes = mm6::encodeSaveContainer(containerFor(fields));
    bytes[2] = '7';
    EXPECT_THROW((void) mm6::decodeSaveContainer(bytes), std::exception);

    fields.characters[0].skills[1] = 0x103; // Grandmaster is not an MM6 skill state.
    EXPECT_THROW((void) mm6::encodeCoreSections(fields), std::exception);
}
