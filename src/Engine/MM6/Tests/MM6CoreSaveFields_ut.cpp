#include <cstddef>
#include <cstdint>
#include <exception>
#include <string>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6CoreSaveFields.h"
#include "Engine/MM6/MM6CoreSaveRuntime.h"
#include "Engine/Objects/Character.h"

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
        character.modifiers.statBonuses[0] = -11 - i;
        character.modifiers.statBonuses[6] = 27 + i;
        character.modifiers.ac = 3 + i;
        character.modifiers.level = -2;
        character.modifiers.age = 4;
        character.modifiers.birthYear = 1147 + i;
        character.modifiers.pureStatPotionUsed[i] = true;
    }
    return fields;
}

UNIT_TEST(MM6CoreSaveFields, CapturesCharacterModifiersAndStagesDetachedCopy) {
    Character source;
    source._statBonuses[ATTRIBUTE_MIGHT] = -13;
    source._statBonuses[ATTRIBUTE_LUCK] = 31;
    source.sACModifier = 7;
    source.sLevelModifier = -2;
    source.sAgeModifier = 4;
    source.uBirthYear = 1147;
    source._pureStatPotionUsed[ATTRIBUTE_MIGHT] = true;
    source._pureStatPotionUsed[ATTRIBUTE_LUCK] = true;

    mm6::CharacterModifierFields fields = mm6::captureCharacterModifiers(source, GameTitle::MM6);
    EXPECT_EQ(fields.statBonuses[0], -13);
    EXPECT_EQ(fields.statBonuses[6], 31);
    EXPECT_EQ(fields.birthYear, 1147);
    EXPECT_TRUE(fields.pureStatPotionUsed[0]);
    EXPECT_TRUE(fields.pureStatPotionUsed[6]);

    Character prior;
    prior._statBonuses[ATTRIBUTE_MIGHT] = 99;
    Character staged = mm6::stageCharacterModifiers(prior, fields, GameTitle::MM6);
    EXPECT_EQ(staged._statBonuses[ATTRIBUTE_MIGHT], -13);
    EXPECT_EQ(staged.sACModifier, 7);
    EXPECT_EQ(staged.sLevelModifier, -2);
    EXPECT_EQ(staged.sAgeModifier, 4);
    EXPECT_TRUE(staged._pureStatPotionUsed[ATTRIBUTE_LUCK]);
    EXPECT_EQ(prior._statBonuses[ATTRIBUTE_MIGHT], 99);

    EXPECT_THROW((void) mm6::captureCharacterModifiers(source, GameTitle::MM7), std::exception);
    EXPECT_THROW((void) mm6::stageCharacterModifiers(prior, fields, GameTitle::MM7), std::exception);
    fields.birthYear = -1;
    EXPECT_THROW((void) mm6::stageCharacterModifiers(prior, fields, GameTitle::MM6), std::exception);
    EXPECT_EQ(prior._statBonuses[ATTRIBUTE_MIGHT], 99);
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

    container = containerFor(fields);
    container.sections[1].payload.back() |= 0x80; // Unused pure-stat potion bit.
    EXPECT_THROW((void) mm6::decodeCoreSections(container), std::exception);

    container = containerFor(fields);
    container.sections[1].payload[0] = 1; // Superseded CHAR v1 payload.
    EXPECT_THROW((void) mm6::decodeCoreSections(container), std::exception);

    std::vector<std::uint8_t> bytes = mm6::encodeSaveContainer(containerFor(fields));
    bytes[2] = '7';
    EXPECT_THROW((void) mm6::decodeSaveContainer(bytes), std::exception);

    fields.characters[0].skills[1] = 0x103; // Grandmaster is not an MM6 skill state.
    EXPECT_THROW((void) mm6::encodeCoreSections(fields), std::exception);
}
