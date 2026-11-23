#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <string_view>

#include "Engine/GameTitle.h"
#include "Engine/Objects/CharacterEnums.h"
#include "Utility/System/Path.h"

class GraphicsImage;

namespace mm7::mods {

/** Exact, MM7-only opt-in for the private all-artifacts engine behavior. */
bool allArtifactsStorageEnabled(GameTitle title, const char *flag);
bool allArtifactsStorageEnabled(GameTitle title);

/** Exact, MM7-only opt-in for rerolling unclaimed random chest loot. */
bool chestRerollEnabled(GameTitle title, const char *flag);
bool chestRerollEnabled(GameTitle title);

/** Exact, MM7-only opt-in for Fortune's Turn, including chest reroll. */
bool monsterLootRerollEnabled(GameTitle title, const char *flag);
bool monsterLootRerollEnabled(GameTitle title);
void armMonsterLootNewGameDragon(GameTitle title);
void cancelMonsterLootNewGameDragon();
bool consumeMonsterLootNewGameDragon();

enum class PortraitMood {
    Normal, Talk, Smile, Sad, Hurt, Pain, Afraid, Cast, Sleep, Unconscious,
    Weak, Cursed, Drunk, Insane, Sick, Paralyzed, Stone, Zombie, Dead, Eradicated,
    Count
};

std::string_view portraitMoodName(PortraitMood mood);
PortraitMood portraitMood(PortraitId portrait, Condition condition);

/** Private MM7 portraits, loaded once from an explicitly supplied local directory. */
class PrivatePortraits {
 public:
    PrivatePortraits() = default;
    ~PrivatePortraits();

    void load(PathView directory);
    GraphicsImage *get(int partySlot, PortraitId portrait, Condition condition) const;
    void releaseRenderIds();

 private:
    static constexpr size_t moodCount = static_cast<size_t>(PortraitMood::Count);
    std::array<std::array<std::unique_ptr<GraphicsImage>, moodCount>, 4> portraits;
};

}  // namespace mm7::mods

namespace mm6::mods {

/**
 * The MM6 All Artifacts mod lifts the cap on random artifacts and keeps an artifact eligible until the party collects
 * it. Atlas sets `ENROTH_MM6_ALL_ARTIFACTS` for an MM6 launch from its own preference.
 *
 * @param title                         Running game title.
 * @param flag                          Value of `ENROTH_MM6_ALL_ARTIFACTS`, or `nullptr` if it isn't set.
 * @return                              Whether the mod is on, which takes MM6 and the exact string `1`.
 */
bool allArtifactsEnabled(GameTitle title, const char *flag);

/**
 * @param title                         Running game title.
 * @return                              Whether `ENROTH_MM6_ALL_ARTIFACTS` turns the All Artifacts mod on.
 */
bool allArtifactsEnabled(GameTitle title);

/**
 * Fortune's Turn for MM6 shows random monster loot, clicked random event items and the monthly town-hall bounty in a
 * popup that can reroll them. Atlas sets `ENROTH_MM6_MONSTER_LOOT_REROLL` for an MM6 launch from its own preference.
 *
 * @param title                         Running game title.
 * @param flag                          Value of `ENROTH_MM6_MONSTER_LOOT_REROLL`, or `nullptr` if it isn't set.
 * @return                              Whether the mod is on, which takes MM6 and the exact string `1`.
 */
bool monsterLootRerollEnabled(GameTitle title, const char *flag);

/**
 * @param title                         Running game title.
 * @return                              Whether `ENROTH_MM6_MONSTER_LOOT_REROLL` turns Fortune's Turn on.
 */
bool monsterLootRerollEnabled(GameTitle title);

/**
 * The MM6 chest reroll mod adds a Reroll button to a chest that still holds the random loot of its first roll. Atlas
 * sets `ENROTH_MM6_CHEST_REROLL` for an MM6 launch from its own preference.
 *
 * @param title                         Running game title.
 * @param flag                          Value of `ENROTH_MM6_CHEST_REROLL`, or `nullptr` if it isn't set.
 * @return                              Whether the mod is on, which takes MM6 and the exact string `1`.
 */
bool chestRerollEnabled(GameTitle title, const char *flag);

/**
 * @param title                         Running game title.
 * @return                              Whether Fortune's Turn or the legacy `ENROTH_MM6_CHEST_REROLL` flag enables chest reroll.
 */
bool chestRerollEnabled(GameTitle title);

}  // namespace mm6::mods
