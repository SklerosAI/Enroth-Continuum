#include "MM6Training.h"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <string>

#include "Engine/Engine.h"
#include "Engine/Localization.h"
#include "Engine/Objects/Character.h"
#include "Engine/Party.h"

#include "GUI/UI/UIHouseEnums.h"
#include "GUI/UI/UIStatusBar.h"
#include "GUI/UI/Houses/MM6House.h"
#include "GUI/UI/Houses/MM6ShopPrices.h"

static constexpr int HOUSE_FIRST_MM6_TRAINING_HALL = 79;
static constexpr std::array<int, 10> trainingHallMaxLevels = {15, 60, 40, 65535, 20, 100, 200, 30, 80, 50};

/**
 * @param hour                          Hour of the day.
 * @return                              Hours to the next 5 am, 24 at 5 am itself.
 */
static int hoursUntil5am(int hour) {
    return hour < 5 ? 5 - hour : 29 - hour;
}

int mm6::trainingMaxLevel(int houseId) {
    if (houseId < HOUSE_FIRST_MM6_TRAINING_HALL || houseId >= HOUSE_FIRST_MM6_TRAINING_HALL + static_cast<int>(trainingHallMaxLevels.size()))
        throw std::runtime_error("MM6 training hall id is outside 79-88");
    return trainingHallMaxLevels[houseId - HOUSE_FIRST_MM6_TRAINING_HALL];
}

int64_t mm6::trainingExperience(int level) {
    return 1000ll * level * (level + 1) / 2;
}

int mm6::trainingPrice(const Character &character, float value, int merchant) {
    const int tier = std::to_underlying(character.classType) % 3 + 1; // MM6 class ids run in promotion triples.
    const int base = static_cast<int>(tier * static_cast<double>(value) * character.uLevel);
    return std::max((100 - merchant) * base / 100, base / 3);
}

std::string mm6::trainingLabel(int houseId, const Character &character, float value) {
    if (character.uLevel >= trainingMaxLevel(houseId))
        return fmt::format("{}\n \n{}", localization->str(LSTR_WITH_YOUR_SKILLS_YOU_SHOULD_BE_WORKING), localization->str(LSTR_SORRY_BUT_WE_ARE_UNABLE_TO_TRAIN_YOU));
    const int64_t required = trainingExperience(character.uLevel);
    if (static_cast<int64_t>(character.experience) < required)
        return localization->format(LSTR_YOU_NEED_D_MORE_EXPERIENCE_TO_TRAIN_TO, static_cast<int>(required - character.experience), character.uLevel + 1);
    return localization->format(LSTR_TRAIN_TO_LEVEL_D_FOR_D_GOLD, character.uLevel + 1, trainingPrice(character, value, mm6::merchantPercent(character)));
}

void mm6::trainActiveCharacter(int houseId, float value, bool *timePassed) {
    if (!pParty->hasActiveCharacter())
        return;
    Character &character = pParty->activeCharacter();
    const HouseId house = static_cast<HouseId>(houseId);
    if (character.uLevel >= trainingMaxLevel(houseId) || static_cast<int64_t>(character.experience) < trainingExperience(character.uLevel)) {
        mm6PlayHouseSound(house, HOUSE_SOUND_TRAINING_CANT_TRAIN);
        return;
    }
    const int price = trainingPrice(character, value, mm6::merchantPercent(character));
    if (pParty->GetGold() < price) {
        engine->_statusBar->setEvent(LSTR_YOU_DONT_HAVE_ENOUGH_GOLD);
        mm6PlayHouseSound(house, HOUSE_SOUND_TRAINING_NOT_ENOUGH_GOLD);
        return;
    }

    pParty->TakeGold(price);
    mm6PlayHouseSound(house, HOUSE_SOUND_TRAINING_TRAIN);
    character.uLevel++;
    character.uSkillPoints += character.uLevel / 10 + 5;
    character.health = character.GetMaxHealth();
    character.mana = character.GetMaxMana();
    if (!*timePassed) {
        const CivilTime now = pParty->GetPlayingTime().toCivilTime();
        restAndHeal(Duration::fromMinutes((hoursUntil5am(now.hour) + 172) * 60 - now.minute)); // Seconds are kept.
        *timePassed = true;
    }
    character.playReaction(SPEECH_LEVEL_UP);
    engine->_statusBar->setEvent(LSTR_S_IS_NOW_LEVEL_LU_AND_HAS_EARNED_LU, character.name, character.uLevel, character.uLevel / 10 + 5);
}
