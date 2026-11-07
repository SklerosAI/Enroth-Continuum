#pragma once

#include <span>
#include <string_view>

#include "Engine/GameTitle.h"
#include "Engine/Objects/Decoration.h"

#include "MM6EventRuntimeSaveFields.h"

class Blob;
struct DecorationTable;
struct OutdoorLocation_MM7;

namespace mm6 {

// Strict first-map writer and fresh-child gate. Rebuilds the MM6 ODM's
// decoration list without mutating globals, applies only the documented
// PrepareDecorations visibility rule, and requires exact live identity/flags.
// Other maps and unsupported event flag changes refuse until they have a
// title-local save owner.
void requireFirstMapOutdoorDecorationReplay(
    std::string_view mapName, const Blob &decodedBase,
    const EventRuntimeMapEntry &events, const DecorationTable &table,
    std::span<const LevelDecoration> live, GameTitle title);

// Decoded-base overload permits source-independent unit tests. Callers of the
// disk writer use the Blob overload so the base is reread from LoadedMapSource.
void requireFirstMapOutdoorDecorationReplay(
    std::string_view mapName, const OutdoorLocation_MM7 &base,
    const EventRuntimeMapEntry &events, const DecorationTable &table,
    std::span<const LevelDecoration> live, GameTitle title);

} // namespace mm6
