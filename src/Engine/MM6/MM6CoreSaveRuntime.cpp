#include "MM6CoreSaveRuntime.h"

#include <bit>
#include <cstddef>
#include <stdexcept>
#include <utility>

#include "Engine/Party.h"
#include "Engine/Objects/CombinedSkillValue.h"
#include "Engine/Spells/SpellEnums.h"

namespace mm6 {
namespace {

Skill sourceSkill(size_t index) {
    return index == 30 ? SKILL_LEARNING : static_cast<Skill>(index);
}

void requireMM6(GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 core save fields cannot apply to another title");
}

} // namespace

CharacterModifierFields captureCharacterModifiers(const Character &character, GameTitle title) {
    requireMM6(title);
    CharacterModifierFields fields;
    for (size_t stat = 0; stat < fields.statBonuses.size(); ++stat) {
        const Attribute attribute = static_cast<Attribute>(stat);
        fields.statBonuses[stat] = character._statBonuses[attribute];
        fields.pureStatPotionUsed[stat] = character._pureStatPotionUsed[attribute];
    }
    fields.ac = character.sACModifier;
    fields.level = character.sLevelModifier;
    fields.age = character.sAgeModifier;
    fields.birthYear = character.uBirthYear;
    if (fields.birthYear < 0 || fields.birthYear > 10000)
        throw std::runtime_error("MM6 character birth year exceeds supported range");
    return fields;
}

Character stageCharacterModifiers(const Character &live, const CharacterModifierFields &saved,
                                  GameTitle title) {
    requireMM6(title);
    if (saved.birthYear < 0 || saved.birthYear > 10000)
        throw std::runtime_error("MM6 character birth year exceeds supported range");
    Character staged = live;
    for (size_t stat = 0; stat < saved.statBonuses.size(); ++stat) {
        const Attribute attribute = static_cast<Attribute>(stat);
        staged._statBonuses[attribute] = saved.statBonuses[stat];
        staged._pureStatPotionUsed[attribute] = saved.pureStatPotionUsed[stat];
    }
    staged.sACModifier = saved.ac;
    staged.sLevelModifier = saved.level;
    staged.sAgeModifier = saved.age;
    staged.uBirthYear = saved.birthYear;
    return staged;
}

CharacterSpellPreferences captureCharacterSpellPreferences(const Character &character, GameTitle title) {
    requireMM6(title);
    const int spell = std::to_underlying(character.uQuickSpell);
    const int page = std::to_underlying(character.lastOpenedSpellbookPage);
    if (spell < 0 || spell > 99 || page < 0 || page > 8)
        throw std::runtime_error("MM6 character spell preferences exceed supported range");
    return {static_cast<std::uint8_t>(spell), static_cast<std::uint8_t>(page)};
}

Character stageCharacterSpellPreferences(const Character &live, const CharacterSpellPreferences &saved,
                                         GameTitle title) {
    requireMM6(title);
    if (saved.quickSpell > 99 || saved.spellbookPage > 8)
        throw std::runtime_error("MM6 character spell preferences exceed supported range");
    Character staged = live;
    staged.uQuickSpell = static_cast<SpellId>(saved.quickSpell);
    staged.lastOpenedSpellbookPage = static_cast<MagicSchool>(saved.spellbookPage);
    return staged;
}

CharacterRuntimeBonuses captureCharacterRuntimeBonuses(const Character &character, GameTitle title) {
    requireMM6(title);
    return {{{static_cast<std::uint8_t>(character._some_attack_bonus),
              static_cast<std::uint8_t>(character._melee_dmg_bonus),
              static_cast<std::uint8_t>(character._ranged_atk_bonus),
              static_cast<std::uint8_t>(character._ranged_dmg_bonus),
              static_cast<std::uint8_t>(character.uFullHealthBonus),
              static_cast<std::uint8_t>(character._health_related),
              static_cast<std::uint8_t>(character.uFullManaBonus),
              static_cast<std::uint8_t>(character._mana_related)}}};
}

Character stageCharacterRuntimeBonuses(const Character &live, const CharacterRuntimeBonuses &saved,
                                      GameTitle title) {
    requireMM6(title);
    Character staged = live;
    const auto &bytes = saved.bytes;
    staged._some_attack_bonus = std::bit_cast<char>(bytes[0]);
    staged._melee_dmg_bonus = std::bit_cast<char>(bytes[1]);
    staged._ranged_atk_bonus = std::bit_cast<char>(bytes[2]);
    staged._ranged_dmg_bonus = std::bit_cast<char>(bytes[3]);
    staged.uFullHealthBonus = std::bit_cast<char>(bytes[4]);
    staged._health_related = std::bit_cast<char>(bytes[5]);
    staged.uFullManaBonus = std::bit_cast<char>(bytes[6]);
    staged._mana_related = std::bit_cast<char>(bytes[7]);
    return staged;
}

CoreSaveFields captureCoreFields(const Party &party, GameTitle title) {
    requireMM6(title);
    CoreSaveFields fields;
    fields.party.gold = party.uNumGold;
    fields.party.food = party.uNumFoodRations;
    fields.party.bankGold = party.uNumGoldInBank;
    fields.party.playingTicks = party.playing_time.ticks();
    fields.party.position = {party.pos.x, party.pos.y, party.pos.z};
    fields.party.viewYaw = party._viewYaw;
    fields.party.viewPitch = party._viewPitch;
    fields.party.activeCharacter = party.hasActiveCharacter() ? party.activeCharacterIndex() : -1;
    fields.party.verticalVelocity = party.velocity.z;
    fields.party.savedFlightZ = party.sPartySavedFlightZ;
    fields.party.waterLavaTimerTicks = party._6FC_water_lava_timer.ticks();
    fields.party.fallStartZ = party.uFallStartZ;
    fields.party.flying = party.bFlying;
    fields.party.partyFlags = std::bit_cast<std::uint32_t>(std::to_underlying(party.uFlags));
    fields.party.partyFlags2 = std::bit_cast<std::uint32_t>(std::to_underlying(party.uFlags2));
    fields.party.fine = party.uFine;
    fields.party.deaths = party.uNumDeaths;
    fields.party.prisonTerms = party.uNumPrisonTerms;
    fields.party.bountiesCollected = party.uNumBountiesCollected;
    fields.party.reputation = party.mm6Reputation;

    for (size_t i = 0; i < fields.characters.size(); ++i) {
        const Character &source = party.pCharacters[i];
        CoreCharacterFields &target = fields.characters[i];
        if (source.uVoiceID < 0 || source.uVoiceID > 255)
            throw std::runtime_error("MM6 character voice ID exceeds core field width");
        target.name = source.name;
        target.sex = std::to_underlying(source.uSex);
        target.classId = std::to_underlying(source.classType);
        target.faceId = source.uCurrentFace;
        target.voiceId = source.uVoiceID;
        target.level = source.uLevel;
        target.experience = source.experience;
        target.health = source.health;
        target.mana = source.mana;
        target.skillPoints = source.uSkillPoints;
        for (size_t stat = 0; stat < target.baseStats.size(); ++stat)
            target.baseStats[stat] = source._stats[static_cast<Attribute>(stat)];
        for (size_t skill = 0; skill < target.skills.size(); ++skill)
            target.skills[skill] = source.pActiveSkills[sourceSkill(skill)].joined();
        for (size_t spell = 0; spell < target.spells.size(); ++spell)
            target.spells[spell] = source.bHaveSpell[static_cast<SpellId>(spell + 1)];
        target.modifiers = captureCharacterModifiers(source, title);
        target.spellPreferences = captureCharacterSpellPreferences(source, title);
        target.runtimeBonuses = captureCharacterRuntimeBonuses(source, title);
    }

    (void) encodeCoreSections(fields); // Validate before returning a capture for serialization.
    return fields;
}

Party stageCoreParty(const Party &live, std::span<const std::uint8_t> bytes, GameTitle title) {
    requireMM6(title);
    CoreSaveFields fields;
    loadCoreFieldsBeforeMutation(bytes, &fields);

    return stageCoreParty(live, fields, title);
}

Party stageCoreParty(const Party &live, const CoreSaveFields &fields, GameTitle title) {
    requireMM6(title);
    (void) encodeCoreSections(fields);
    Party staged = live;
    staged.uNumGold = fields.party.gold;
    staged.uNumFoodRations = fields.party.food;
    staged.uNumGoldInBank = fields.party.bankGold;
    staged.playing_time = Time::fromTicks(fields.party.playingTicks);
    staged.pos = Vec3f(fields.party.position[0], fields.party.position[1], fields.party.position[2]);
    staged._viewYaw = fields.party.viewYaw;
    staged._viewPitch = fields.party.viewPitch;
    staged.setActiveCharacterIndex(fields.party.activeCharacter);
    // Only vertical velocity persists; horizontal movement is derived from
    // current input. Ground face IDs belong to the reloaded map, so reset it.
    staged.velocity = Vec3f(0, 0, fields.party.verticalVelocity);
    staged.sPartySavedFlightZ = fields.party.savedFlightZ;
    staged._6FC_water_lava_timer = Time::fromTicks(fields.party.waterLavaTimerTicks);
    staged.uFallStartZ = fields.party.fallStartZ;
    staged.bFlying = fields.party.flying;
    staged.uFlags = static_cast<PartyFlags>(std::bit_cast<std::int32_t>(fields.party.partyFlags));
    staged.uFlags2 = static_cast<PARTY_FLAGS_2>(std::bit_cast<std::int32_t>(fields.party.partyFlags2));
    staged.uFine = fields.party.fine;
    staged.uNumDeaths = fields.party.deaths;
    staged.uNumPrisonTerms = fields.party.prisonTerms;
    staged.uNumBountiesCollected = fields.party.bountiesCollected;
    staged.mm6Reputation = fields.party.reputation;
    staged.floor_face_id = -1;
    staged.currentWalkingSound = SOUND_Invalid;

    for (size_t i = 0; i < fields.characters.size(); ++i) {
        const CoreCharacterFields &source = fields.characters[i];
        Character &target = staged.pCharacters[i];
        target.name = source.name;
        target.uSex = static_cast<Sex>(source.sex);
        target.classType = static_cast<Class>(source.classId);
        target.uCurrentFace = source.faceId;
        target.uVoiceID = source.voiceId;
        target.uLevel = source.level;
        target.experience = source.experience;
        target.health = source.health;
        target.mana = source.mana;
        target.uSkillPoints = source.skillPoints;
        for (size_t stat = 0; stat < source.baseStats.size(); ++stat)
            target._stats[static_cast<Attribute>(stat)] = source.baseStats[stat];
        for (size_t skill = 0; skill < source.skills.size(); ++skill)
            target.pActiveSkills[sourceSkill(skill)] = CombinedSkillValue::fromJoined(source.skills[skill]);
        for (size_t spell = 0; spell < source.spells.size(); ++spell)
            target.bHaveSpell[static_cast<SpellId>(spell + 1)] = source.spells[spell];
        target = stageCharacterModifiers(target, source.modifiers, title);
        target = stageCharacterSpellPreferences(target, source.spellPreferences, title);
        target = stageCharacterRuntimeBonuses(target, source.runtimeBonuses, title);
    }
    return staged;
}

} // namespace mm6
