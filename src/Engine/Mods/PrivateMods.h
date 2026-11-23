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

/** Exact, MM7-only opt-in for the separate monster loot reroll mod. */
bool monsterLootRerollEnabled(GameTitle title, const char *flag);
bool monsterLootRerollEnabled(GameTitle title);

/** Exact, MM7-only opt-in for the private Dragoon class expansion. */
bool dragoonClassEnabled(GameTitle title, const char *flag);
bool dragoonClassEnabled(GameTitle title);

/** Loads the optional Dragoon creation icon from the private MM7 mod directory. */
std::unique_ptr<GraphicsImage> loadDragoonClassIcon(PathView directory);
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
