#pragma once

#include <string>

#include "Engine/Objects/CharacterEnums.h"

#include "MM6Shop.h"

class Character;

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
