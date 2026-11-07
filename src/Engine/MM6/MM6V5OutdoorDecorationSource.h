#pragma once

#include <cstdint>
#include <stdexcept>
#include <string_view>

#include "Engine/Objects/Decoration.h"
#include "Engine/Tables/DecorationTable.h"
#include "Utility/String/Ascii.h"

namespace mm6 {

// MM6 ODM stores both a decoration-table ID and its name. Row zero is an
// authored null descriptor and is intentionally skipped by decorationId().
// Require the exact same table row instead of treating an unknown name as a
// null decoration or substituting another visual template.
inline DecorationId requireMM6OutdoorSourceDecorationId(
    std::uint16_t nativeId, std::string_view name, const DecorationTable &table) {
    if (nativeId >= table.decorations.size() ||
        !ascii::noCaseEquals(name, table.decorations[nativeId].internalName) ||
        (nativeId != 0 &&
         table.decorationId(name) != static_cast<DecorationId>(nativeId)))
        throw std::runtime_error("MM6 outdoor decoration source ID and name disagree");
    return static_cast<DecorationId>(nativeId);
}

} // namespace mm6
