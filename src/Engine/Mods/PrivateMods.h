#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <string_view>

#include "Engine/Objects/CharacterEnums.h"
#include "Utility/System/Path.h"

class GraphicsImage;

namespace mm7::mods {

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
