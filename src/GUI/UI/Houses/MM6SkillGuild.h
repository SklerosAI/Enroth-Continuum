#pragma once

#include <string>

#include "Engine/Objects/CharacterEnums.h"

#include "MM6House.h"

/**
 * MM6 mercenary or thieves' guild. Only members are served. They learn the guild's skills that their class can learn
 * and that they don't know yet.
 */
class GUIWindow_MM6SkillGuild : public GUIWindow_MM6House {
 public:
    GUIWindow_MM6SkillGuild(HouseId houseId, const mm6::HousePresentationState &presentation);

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
     * @return                          Skill the service teaches the active character, or `SKILL_INVALID` if the
     *                                  character's class can't learn it or the character already knows it.
     */
    Skill learnableSkill(int index) const;

    /**
     * @return                          Gold the guild asks the active character for a skill.
     */
    int skillPrice() const;
};
