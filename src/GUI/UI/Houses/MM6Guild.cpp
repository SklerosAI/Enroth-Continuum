#include "MM6Guild.h"

#include <algorithm>
#include <array>
#include <ranges>
#include <string>
#include <utility>

#include "Engine/Engine.h"
#include "Engine/Localization.h"
#include "Engine/MM6/MM6CreationRules.h"
#include "Engine/Objects/Character.h"
#include "Engine/Party.h"
#include "Engine/Tables/NPCTable.h"

#include "GUI/UI/UIStatusBar.h"

#include "MM6ShopPrices.h"

/**
 * @param houseId                       MM6 house id.
 * @param character                     Character who wants to study.
 * @return                              Whether the character has the award that makes it a member of the spell guild.
 */
static bool isGuildMember(int houseId, const Character &character) {
    static constexpr int kFirstGuild = 119;
    static constexpr std::array<int, 11> kAwards = {74, 72, 75, 73, 78, 77, 76, 79, 80, 64, 65}; // Fire to Self, two houses each.
    const int guild = (houseId - kFirstGuild) / 2;
    return houseId >= kFirstGuild && guild < static_cast<int>(kAwards.size()) && character._achievedAwardsBits[static_cast<AwardId>(kAwards[guild])];
}

GUIWindow_MM6Guild::GUIWindow_MM6Guild(HouseId houseId, const mm6::HousePresentationState &presentation)
    : GUIWindow_MM6Shop(houseId, presentation) {}

void GUIWindow_MM6Guild::Update() {
    GUIWindow_MM6Shop::Update();
    if (screen() || !isProprietorSelected() || !pParty->hasActiveCharacter())
        return;

    const Character &character = pParty->activeCharacter();
    std::string refusal;
    if (!isGuildMember(std::to_underlying(houseId()), character)) {
        refusal = pNPCTopics[171].pText; // Text 172 of npctext.txt.
    } else if (!character.CanAct()) {
        refusal = localization->format(LSTR_S_IS_IN_NO_CONDITION_TO_S, character.name, localization->str(LSTR_DO_ANYTHING));
    }
    if (!refusal.empty()) {
        drawPanelText(refusal, (212 - panelTextHeight(refusal)) / 2 + 101, colorTable.PaleCanary);
        return;
    }
    const auto learnable = [this](int index) { return learnableSkill(index) != SKILL_INVALID; };
    if (std::ranges::any_of(std::views::iota(0, static_cast<int>(_services.size())), learnable))
        drawPanelText(localization->format(LSTR_SKILL_COST_LU, skillPrice()), 146, colorTable.White);
}

void GUIWindow_MM6Guild::selectService(int index) {
    if (screen() || !isProprietorSelected() || !servesActiveCharacter() || index < 0 || index >= static_cast<int>(_services.size()))
        return;
    if (_services[index] != mm6::HouseServiceOption::LearnSkill) {
        GUIWindow_MM6Shop::selectService(index);
        return;
    }

    const Skill skill = learnableSkill(index);
    if (skill == SKILL_INVALID)
        return;
    const int price = skillPrice();
    if (pParty->GetGold() < price) {
        engine->_statusBar->setEvent(LSTR_YOU_DONT_HAVE_ENOUGH_GOLD);
        mm6PlayHouseSound(houseId(), HOUSE_SOUND_GENERAL_NOT_ENOUGH_GOLD);
        return;
    }
    pParty->TakeGold(price);
    pParty->activeCharacter().pActiveSkills[skill] = CombinedSkillValue::novice();
}

std::string GUIWindow_MM6Guild::serviceLabel(int index) const {
    if (!servesActiveCharacter())
        return {};
    if (_services[index] != mm6::HouseServiceOption::LearnSkill)
        return GUIWindow_MM6Shop::serviceLabel(index);
    const Skill skill = learnableSkill(index);
    return skill == SKILL_INVALID ? std::string() : localization->skillName(skill);
}

bool GUIWindow_MM6Guild::servesActiveCharacter() const {
    if (!pParty->hasActiveCharacter())
        return false;
    const Character &character = pParty->activeCharacter();
    return isGuildMember(std::to_underlying(houseId()), character) && character.CanAct();
}

Skill GUIWindow_MM6Guild::learnableSkill(int index) const {
    if (_services[index] != mm6::HouseServiceOption::LearnSkill || !pParty->hasActiveCharacter())
        return SKILL_INVALID;
    const Skill skill = mm6::guildSkills(_house.type)[index - 1]; // The skills follow Buy Spells.
    const Character &character = pParty->activeCharacter();
    const int sourceSkill = skill == SKILL_LEARNING ? 30 : std::to_underlying(skill); // MM6 numbers Learning 30.
    const bool classLearns = mm6::creationRules().startingSkills[mm6::classFamily(std::to_underlying(character.classType))][sourceSkill] != 0;
    return classLearns && !character.pActiveSkills[skill] ? skill : SKILL_INVALID;
}

int GUIWindow_MM6Guild::skillPrice() const {
    const int full = static_cast<int>(500 * _house.value.value_or(0));
    return std::max((100 - mm6::merchantPercent(pParty->activeCharacter())) * full / 100, full / 3);
}
