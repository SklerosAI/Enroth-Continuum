#pragma once

#include <cstddef>
#include <optional>
#include <span>

#include "Engine/GameTitle.h"
#include "Engine/Objects/Decoration.h"

namespace mm6 {

// MM6 SetSprite addresses a map sprite by its zero-based array index. Its
// MapSprite record has no MM7 uCog field, so the inherited cog lookup cannot
// resolve an MM6 event. Keep the MM7 event route separate.
inline bool applySetSprite(GameTitle title, std::span<LevelDecoration> decorations,
                           int index, bool visible, std::optional<DecorationId> replacement) {
    if (title != GameTitle::MM6 || index < 0 || static_cast<size_t>(index) >= decorations.size())
        return false;

    LevelDecoration &decoration = decorations[index];
    if (replacement)
        decoration.uDecorationDescID = *replacement;
    if (visible)
        decoration.uFlags &= ~LEVEL_DECORATION_INVISIBLE;
    else
        decoration.uFlags |= LEVEL_DECORATION_INVISIBLE;
    return true;
}

} // namespace mm6
