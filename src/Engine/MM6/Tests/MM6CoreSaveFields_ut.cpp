#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <string>
#include <utility>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6CoreSaveFields.h"
#include "Engine/MM6/MM6CoreSaveRuntime.h"
#include "Engine/Objects/Character.h"
#include "Engine/Party.h"

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
    fields.party.verticalVelocity = -237.5f;
    fields.party.savedFlightZ = 720;
    fields.party.waterLavaTimerTicks = 987700;
    fields.party.fallStartZ = 910;
    fields.party.flying = 1;
    fields.party.partyFlags = 0x020c;
    fields.party.partyFlags2 = 2;
    fields.party.fine = 125;
    fields.party.deaths = 3;
    fields.party.prisonTerms = 2;
    fields.party.bountiesCollected = -7; // Preserve raw current-runtime event arithmetic.
    fields.party.reputation = -928;
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
        character.spellPreferences.quickSpell = 1 + i;
        character.spellPreferences.spellbookPage = i + 1;
        character.runtimeBonuses.bytes = {0x80, 0xff, 1, 2, 3, 4, 5, static_cast<std::uint8_t>(i)};
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

UNIT_TEST(MM6CoreSaveFields, CapturesSpellPreferencesAndStagesDetachedCopy) {
    Character source;
    source.uQuickSpell = SPELL_FIRE_FIREBALL;
    source.lastOpenedSpellbookPage = MAGIC_SCHOOL_WATER;
    mm6::CharacterSpellPreferences preferences =
        mm6::captureCharacterSpellPreferences(source, GameTitle::MM6);
    EXPECT_EQ(preferences.quickSpell, std::to_underlying(SPELL_FIRE_FIREBALL));
    EXPECT_EQ(preferences.spellbookPage, std::to_underlying(MAGIC_SCHOOL_WATER));

    Character prior;
    prior.uQuickSpell = SPELL_NONE;
    prior.lastOpenedSpellbookPage = MAGIC_SCHOOL_FIRE;
    Character staged = mm6::stageCharacterSpellPreferences(prior, preferences, GameTitle::MM6);
    EXPECT_EQ(staged.uQuickSpell, SPELL_FIRE_FIREBALL);
    EXPECT_EQ(staged.lastOpenedSpellbookPage, MAGIC_SCHOOL_WATER);
    EXPECT_EQ(prior.uQuickSpell, SPELL_NONE);
    EXPECT_EQ(prior.lastOpenedSpellbookPage, MAGIC_SCHOOL_FIRE);

    EXPECT_THROW((void) mm6::captureCharacterSpellPreferences(source, GameTitle::MM7), std::exception);
    EXPECT_THROW((void) mm6::stageCharacterSpellPreferences(prior, preferences, GameTitle::MM7), std::exception);
    preferences.quickSpell = 100;
    EXPECT_THROW((void) mm6::stageCharacterSpellPreferences(prior, preferences, GameTitle::MM6), std::exception);
    preferences.quickSpell = 1;
    preferences.spellbookPage = 9;
    EXPECT_THROW((void) mm6::stageCharacterSpellPreferences(prior, preferences, GameTitle::MM6), std::exception);
    EXPECT_EQ(prior.uQuickSpell, SPELL_NONE);

    source.uQuickSpell = static_cast<SpellId>(100);
    EXPECT_THROW((void) mm6::captureCharacterSpellPreferences(source, GameTitle::MM6), std::exception);
    source.uQuickSpell = SPELL_FIRE_FIREBALL;
    source.lastOpenedSpellbookPage = static_cast<MagicSchool>(9);
    EXPECT_THROW((void) mm6::captureCharacterSpellPreferences(source, GameTitle::MM6), std::exception);
}

UNIT_TEST(MM6CoreSaveFields, PreservesCurrentRuntimeBonusBytesWithoutNativeClaims) {
    Character source;
    source._some_attack_bonus = std::bit_cast<char>(std::uint8_t(0x80));
    source._melee_dmg_bonus = std::bit_cast<char>(std::uint8_t(0xff));
    source._ranged_atk_bonus = 3;
    source._ranged_dmg_bonus = 4;
    source.uFullHealthBonus = 5;
    source._health_related = 6;
    source.uFullManaBonus = 7;
    source._mana_related = 8;
    const mm6::CharacterRuntimeBonuses saved = mm6::captureCharacterRuntimeBonuses(source, GameTitle::MM6);
    EXPECT_EQ(saved.bytes, (std::array<std::uint8_t, 8>{0x80, 0xff, 3, 4, 5, 6, 7, 8}));

    Character prior;
    prior._some_attack_bonus = 9;
    Character staged = mm6::stageCharacterRuntimeBonuses(prior, saved, GameTitle::MM6);
    EXPECT_EQ(mm6::captureCharacterRuntimeBonuses(staged, GameTitle::MM6), saved);
    EXPECT_EQ(prior._some_attack_bonus, 9);
    EXPECT_THROW((void) mm6::captureCharacterRuntimeBonuses(source, GameTitle::MM7), std::exception);
    EXPECT_THROW((void) mm6::stageCharacterRuntimeBonuses(prior, saved, GameTitle::MM7), std::exception);
    EXPECT_EQ(prior._some_attack_bonus, 9);
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
    EXPECT_EQ(destination.party.verticalVelocity, -237.5f);
    EXPECT_EQ(destination.party.savedFlightZ, 720);
    EXPECT_EQ(destination.party.fallStartZ, 910);
    EXPECT_EQ(destination.party.partyFlags, 0x020c);
    EXPECT_EQ(containerFor(source).sections[0].payload.size(), 94);
    EXPECT_EQ(destination.party.fine, 125);
    EXPECT_EQ(destination.party.bountiesCollected, -7);
    EXPECT_EQ(destination.party.reputation, -928);
}

UNIT_TEST(MM6CoreSaveFields, CapturesAndStagesMovementOnDetachedParty) {
    Party source;
    for (Character &character : source.pCharacters)
        character.name = "Hero";
    source.pos = Vec3f(10, 20, 340);
    source.velocity = Vec3f(80, -90, -237.5f);
    source.sPartySavedFlightZ = 720;
    source._6FC_water_lava_timer = Time::fromTicks(987700);
    source.uFallStartZ = 910;
    source.bFlying = 1;
    source.uFlags = PARTY_FLAG_WATER_DAMAGE | PARTY_FLAG_AIRBORNE | PARTY_FLAG_BURNING;
    source.uFlags2 = PARTY_FLAGS_2_RUNNING;
    source.floor_face_id = 19;
    source.uFine = 125;
    source.uNumDeaths = 3;
    source.uNumPrisonTerms = 2;
    source.uNumBountiesCollected = -7;
    source.mm6Reputation = 800;
    mm6::CoreSaveFields fields = mm6::captureCoreFields(source, GameTitle::MM6);
    EXPECT_EQ(fields.party.verticalVelocity, -237.5f);
    EXPECT_EQ(fields.party.fallStartZ, 910);
    EXPECT_EQ(fields.party.partyFlags, 0x020c);
    EXPECT_EQ(fields.party.partyFlags2, 2);
    EXPECT_EQ(fields.party.fine, 125);
    EXPECT_EQ(fields.party.deaths, 3);
    EXPECT_EQ(fields.party.prisonTerms, 2);
    EXPECT_EQ(fields.party.bountiesCollected, -7);
    EXPECT_EQ(fields.party.reputation, 800);

    Party prior;
    prior.velocity = Vec3f(8, 9, 10);
    prior.uFallStartZ = 77;
    prior.floor_face_id = 42;
    Party staged = mm6::stageCoreParty(
        prior, mm6::encodeSaveContainer(containerFor(fields)), GameTitle::MM6);
    Party direct = mm6::stageCoreParty(prior, fields, GameTitle::MM6);
    EXPECT_EQ(mm6::captureCoreFields(direct, GameTitle::MM6),
              mm6::captureCoreFields(staged, GameTitle::MM6));
    EXPECT_EQ(staged.pos.z, 340);
    EXPECT_EQ(staged.velocity.x, 0);
    EXPECT_EQ(staged.velocity.y, 0);
    EXPECT_EQ(staged.velocity.z, -237.5f);
    EXPECT_EQ(staged.sPartySavedFlightZ, 720);
    EXPECT_EQ(staged._6FC_water_lava_timer.ticks(), 987700);
    EXPECT_EQ(staged.uFallStartZ, 910);
    EXPECT_EQ(staged.bFlying, 1);
    EXPECT_EQ(std::to_underlying(staged.uFlags), 0x020c);
    EXPECT_EQ(std::to_underlying(staged.uFlags2), 2);
    EXPECT_EQ(staged.floor_face_id, -1);
    EXPECT_EQ(staged.currentWalkingSound, SOUND_Invalid);
    EXPECT_EQ(staged.uFine, 125);
    EXPECT_EQ(staged.uNumDeaths, 3);
    EXPECT_EQ(staged.uNumPrisonTerms, 2);
    EXPECT_EQ(staged.uNumBountiesCollected, -7);
    EXPECT_EQ(staged.mm6Reputation, 800);
    EXPECT_EQ(prior.velocity.z, 10);
    EXPECT_EQ(prior.uFallStartZ, 77);
    EXPECT_EQ(prior.floor_face_id, 42);
    EXPECT_THROW((void) mm6::captureCoreFields(source, GameTitle::MM7), std::exception);
    EXPECT_THROW((void) mm6::stageCoreParty(
        prior, mm6::encodeSaveContainer(containerFor(fields)), GameTitle::MM7), std::exception);
    EXPECT_THROW((void) mm6::stageCoreParty(prior, fields, GameTitle::MM7), std::exception);
    fields.characters[0].classId = 255;
    EXPECT_THROW((void) mm6::stageCoreParty(prior, fields, GameTitle::MM6), std::exception);
    EXPECT_EQ(prior.velocity.z, 10);
}

UNIT_TEST(MM6CoreSaveFields, InvalidFieldLeavesDestinationUntouched) {
    mm6::CoreSaveFields destination = exampleFields();
    mm6::CoreSaveFields before = destination;
    mm6::SaveContainer invalid = containerFor(before);
    invalid.sections[1].payload[3 + before.characters[0].name.size() + 1] = 255; // Class ID.
    EXPECT_THROW(mm6::loadCoreFieldsBeforeMutation(mm6::encodeSaveContainer(invalid), &destination), std::exception);
    EXPECT_EQ(destination, before);

    invalid = containerFor(before);
    invalid.sections[1].payload.pop_back(); // Final character's last v4 bonus byte is missing.
    EXPECT_THROW(mm6::loadCoreFieldsBeforeMutation(mm6::encodeSaveContainer(invalid), &destination), std::exception);
    EXPECT_EQ(destination, before);

    invalid = containerFor(before);
    invalid.sections[0].payload.pop_back(); // Final PART v4 reputation byte is missing.
    EXPECT_THROW(mm6::loadCoreFieldsBeforeMutation(mm6::encodeSaveContainer(invalid), &destination),
                 std::exception);
    EXPECT_EQ(destination, before);

    invalid = containerFor(before);
    invalid.sections[0].payload[74] = 0xff; // Negative fine, valid container checksum.
    invalid.sections[0].payload[75] = 0xff;
    invalid.sections[0].payload[76] = 0xff;
    invalid.sections[0].payload[77] = 0xff;
    EXPECT_THROW(mm6::loadCoreFieldsBeforeMutation(mm6::encodeSaveContainer(invalid), &destination),
                 std::exception);
    EXPECT_EQ(destination, before);

    invalid = containerFor(before);
    invalid.sections[0].payload[42] = 0;
    invalid.sections[0].payload[43] = 0;
    invalid.sections[0].payload[44] = 0x80;
    invalid.sections[0].payload[45] = 0x7f; // Infinite vertical speed with valid CRC.
    EXPECT_THROW(mm6::loadCoreFieldsBeforeMutation(mm6::encodeSaveContainer(invalid), &destination),
                 std::exception);
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
    container.sections[1].payload[container.sections[1].payload.size() - 11] |= 0x80;
    // Unused pure-stat potion bit in final character.
    EXPECT_THROW((void) mm6::decodeCoreSections(container), std::exception);

    container = containerFor(fields);
    container.sections[1].payload[0] = 2; // Superseded CHAR v2 payload.
    EXPECT_THROW((void) mm6::decodeCoreSections(container), std::exception);
    container.sections[1].payload[0] = 3; // Superseded CHAR v3 payload.
    EXPECT_THROW((void) mm6::decodeCoreSections(container), std::exception);

    container = containerFor(fields);
    container.sections[0].payload[0] = 1; // Superseded PART v1 payload.
    EXPECT_THROW((void) mm6::decodeCoreSections(container), std::exception);
    container.sections[0].payload[0] = 2; // Superseded PART v2 payload.
    EXPECT_THROW((void) mm6::decodeCoreSections(container), std::exception);
    container.sections[0].payload[0] = 3; // Superseded PART v3 payload.
    EXPECT_THROW((void) mm6::decodeCoreSections(container), std::exception);
    container.sections[0].payload.resize(90); // Exact superseded v3 length.
    EXPECT_THROW((void) mm6::decodeCoreSections(container), std::exception);
    container.sections[0].payload.resize(74); // Exact superseded v2 length.
    EXPECT_THROW((void) mm6::decodeCoreSections(container), std::exception);
    container.sections[0].payload.resize(42); // Exact old provisional v1 length.
    EXPECT_THROW((void) mm6::decodeCoreSections(container), std::exception);

    container = containerFor(fields);
    container.sections[1].payload[container.sections[1].payload.size() - 10] = 100;
    EXPECT_THROW((void) mm6::decodeCoreSections(container), std::exception);
    container = containerFor(fields);
    container.sections[1].payload[container.sections[1].payload.size() - 9] = 9;
    EXPECT_THROW((void) mm6::decodeCoreSections(container), std::exception);

    std::vector<std::uint8_t> bytes = mm6::encodeSaveContainer(containerFor(fields));
    bytes[2] = '7';
    EXPECT_THROW((void) mm6::decodeSaveContainer(bytes), std::exception);

    fields.characters[0].skills[1] = 0x103; // Grandmaster is not an MM6 skill state.
    EXPECT_THROW((void) mm6::encodeCoreSections(fields), std::exception);
}
