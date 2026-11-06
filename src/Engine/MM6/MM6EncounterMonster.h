#pragma once

#include <string>
#include <string_view>
#include <utility>

#include "Engine/GameTitle.h"
#include "Engine/Objects/Monsters.h"

#include "Utility/String/Ascii.h"
#include "Utility/String/Transformations.h"

namespace mm6 {

// MM6 map encounter cells can name either an internal family or the display
// name of one of its variants. Its monster tables use compact internal names
// such as FamilyA, while MM7's encounter path appends " A" and looks that up.
// Return invalid on missing or ambiguous families rather than substituting an
// unrelated monster.
inline MonsterId resolveEncounterMonster(GameTitle title, const MonsterStats &stats,
                                         std::string_view selectedName) {
    if (title != GameTitle::MM6)
        return MONSTER_INVALID;

    selectedName = trim(selectedName);
    if (selectedName.empty() || selectedName == "0")
        return MONSTER_INVALID;

    int rank = 0;
    std::string_view familyName = selectedName;
    if (selectedName.size() >= 2 && selectedName[selectedName.size() - 2] == ' ' &&
        selectedName.back() >= 'A' && selectedName.back() <= 'C') {
        rank = selectedName.back() - 'A';
        familyName.remove_suffix(2);
    }

    auto compact = [](std::string_view name) {
        std::string result;
        result.reserve(name.size());
        for (char c : name)
            if (c != ' ' && c != '\t')
                result.push_back(c);
        return result;
    };

    // A complete internal name can be supplied directly. Otherwise attach
    // the selected rank to the compact family stem.
    const std::string direct = compact(selectedName);
    const std::string candidate = compact(familyName) + static_cast<char>('A' + rank);
    MonsterId internalMatch = MONSTER_INVALID;
    for (MonsterId id : stats.infos.indices()) {
        const MonsterInfo &info = stats.infos[id];
        if (info.name.empty())
            continue;
        if (ascii::noCaseEquals(compact(info.internalName), direct) ||
            ascii::noCaseEquals(compact(info.internalName), candidate)) {
            if (internalMatch != MONSTER_INVALID && internalMatch != id)
                return MONSTER_INVALID;
            internalMatch = id;
        }
    }
    if (internalMatch != MONSTER_INVALID)
        return internalMatch;

    MonsterId family = MONSTER_INVALID;
    for (MonsterId id : stats.infos.indices()) {
        if (!ascii::noCaseEquals(compact(stats.infos[id].name), compact(familyName)))
            continue;
        const int first = std::to_underlying(id) - (std::to_underlying(id) - 1) % 3;
        const MonsterId firstId = static_cast<MonsterId>(first);
        if (family != MONSTER_INVALID && family != firstId)
            return MONSTER_INVALID;
        family = firstId;
    }
    if (family == MONSTER_INVALID)
        return MONSTER_INVALID;

    const MonsterId result = static_cast<MonsterId>(std::to_underlying(family) + rank);
    return stats.infos[result].name.empty() ? MONSTER_INVALID : result;
}

} // namespace mm6
