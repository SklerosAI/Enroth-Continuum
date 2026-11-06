#include "MM6CoreSaveRuntime.h"

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
    }

    (void) encodeCoreSections(fields); // Validate before returning a capture for serialization.
    return fields;
}

Party stageCoreParty(const Party &live, std::span<const std::uint8_t> bytes, GameTitle title) {
    requireMM6(title);
    CoreSaveFields fields;
    loadCoreFieldsBeforeMutation(bytes, &fields);

    Party staged = live;
    staged.uNumGold = fields.party.gold;
    staged.uNumFoodRations = fields.party.food;
    staged.uNumGoldInBank = fields.party.bankGold;
    staged.playing_time = Time::fromTicks(fields.party.playingTicks);
    staged.pos = Vec3f(fields.party.position[0], fields.party.position[1], fields.party.position[2]);
    staged._viewYaw = fields.party.viewYaw;
    staged._viewPitch = fields.party.viewPitch;
    staged.setActiveCharacterIndex(fields.party.activeCharacter);

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
    }
    return staged;
}

} // namespace mm6
