#pragma once

#include <string>

#include "Engine/Objects/CharacterEnums.h"

#include "MM6Shop.h"

class Character;

namespace mm6 {

/**
 * @param houseId                       MM6 house id of a spell, mercenary or thieves' guild.
 * @param character                     Character who wants to study.
 * @return                              Whether the character has the award that makes it a member of the guild.
 */
bool isGuildMember(HouseId houseId, const Character &character);

/**
 * @param character                     Character who wants to learn.
 * @param skill                         Skill a guild teaches.
 * @return                              Whether the character's class can learn the skill.
 */
bool classLearnsSkill(const Character &character, Skill skill);

/**
 * @param value                         Guild's `Val` column in 2DEvents.txt.
 * @param unitPrice                     Gold the guild asks per point of `value`.
 * @param character                     Character who learns.
 * @return                              Gold the guild asks the character for a skill, at least a third of the full
 *                                      price.
 */
int guildSkillPrice(float value, int unitPrice, const Character &character);

} // namespace mm6

/**
 * MM6 spell guild. Only members are served. They buy spellbooks, and learn the guild's skills that their class can
 * learn and that they don't know yet.
 */
class GUIWindow_MM6Guild : public GUIWindow_MM6Shop {
 public:
    GUIWindow_MM6Guild(HouseId houseId, const mm6::HousePresentationState &presentation);

    virtual void Update() override;
    virtual void selectService(int index) override;

 protected:
    virtual std::string serviceLabel(int index) const override;

 private:
    /**
     * @return                          Whether the active character is a member of the guild and can act.
     */
    bool servesActiveCharacter() const;

    /**
     * @param index                     Index into the proprietor's service list.
     * @return                          Skill the service teaches the active character, or `SKILL_INVALID` if it
     *                                  teaches nothing or the character can't learn the skill or already knows it.
     */
    Skill learnableSkill(int index) const;

    /**
     * @return                          Gold the guild asks the active character for a skill.
     */
    int skillPrice() const;
};
