#include "MM6V4OutdoorDecorationReplay.h"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

#include "Engine/MM6/MM6Decorations.h"
#include "Engine/Snapshots/CompositeSnapshots.h"
#include "Engine/Snapshots/MM6OutdoorSnapshot.h"
#include "Engine/Tables/DecorationTable.h"

namespace mm6 {

void requireFirstMapOutdoorDecorationReplay(
    std::string_view mapName, const Blob &decodedBase,
    const EventRuntimeMapEntry &events, const DecorationTable &table,
    std::span<const LevelDecoration> live, GameTitle title) {
    if (title != GameTitle::MM6 || mapName != "oute3.odm" ||
        events.mapName != mapName || decodedBase.empty())
        throw std::runtime_error("MM6 first-map decoration replay lacks a bound base");

    OutdoorLocation_MM7 base;
    deserializeMM6Outdoor(decodedBase, &base);
    requireFirstMapOutdoorDecorationReplay(mapName, base, events, table, live, title);
}

void requireFirstMapOutdoorDecorationReplay(
    std::string_view mapName, const OutdoorLocation_MM7 &base,
    const EventRuntimeMapEntry &events, const DecorationTable &table,
    std::span<const LevelDecoration> live, GameTitle title) {
    if (title != GameTitle::MM6 || mapName != "oute3.odm" ||
        events.mapName != mapName)
        throw std::runtime_error("MM6 first-map decoration replay lacks a bound base");
    if (base.decorations.size() != live.size() ||
        base.decorations.size() != base.decorationNames.size())
        throw std::runtime_error("MM6 first-map decoration count differs from base");

    std::vector<LevelDecoration> expected(live.size());
    for (std::size_t index = 0; index < expected.size(); ++index) {
        reconstruct(base.decorations[index], &expected[index]);
        const auto &name = base.decorationNames[index];
        const auto end = std::find(name.begin(), name.end(), '\0');
        expected[index].uDecorationDescID = table.decorationId(
            std::string(name.begin(), end));
        if (expected[index].uDecorationDescID == DECORATION_NULL)
            throw std::runtime_error("MM6 first-map decoration name lacks a template");
    }
    if (captureEventRuntimeDecorationBindings(expected, title) != events.bindings)
        throw std::runtime_error("MM6 first-map decoration binding differs from base");

    std::size_t variable = 0;
    for (LevelDecoration &decoration : expected) {
        if (!decoration.uEventID && isInteractiveDecoration(decoration.uDecorationDescID) && variable < 124) {
            decoration.eventVarId = static_cast<int16_t>(variable);
            if (!events.decorVars[variable])
                decoration.uFlags |= LEVEL_DECORATION_INVISIBLE;
            ++variable;
        }
    }
    for (std::size_t index = 0; index < expected.size(); ++index) {
        const LevelDecoration &one = expected[index];
        const LevelDecoration &two = live[index];
        const auto reject = [index](std::string_view field) {
            throw std::runtime_error("MM6 first-map decoration " +
                                     std::string(field) + " changed at slot " +
                                     std::to_string(index));
        };
        if (one.uDecorationDescID != two.uDecorationDescID) reject("descriptor");
        if (one.uFlags != two.uFlags) reject("flags");
        if (one.vPosition != two.vPosition) reject("position");
        if (one._yawAngle != two._yawAngle) reject("yaw");
        if (one.uCog != two.uCog) reject("cog");
        if (one.uEventID != two.uEventID) reject("event id");
        if (one.uTriggerRange != two.uTriggerRange) reject("trigger range");
        if (one.eventVarId != two.eventVarId) reject("event var id");
    }
}

} // namespace mm6
