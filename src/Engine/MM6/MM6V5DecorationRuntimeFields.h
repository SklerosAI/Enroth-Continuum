#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "Engine/GameTitle.h"
#include "Engine/Objects/Decoration.h"

#include "MM6EventRuntimeSaveFields.h"
#include "MM6SaveContainer.h"

class Blob;
struct DecorationTable;
struct OutdoorLocation_MM7;

namespace mm6 {

// Full ordered decoration state, detached from native outdoor DDM. Source ODM
// position, yaw, cog, event and trigger fields remain immutable slot anchors.
struct DecorationRuntimeSaveFields {
    std::string mapName;
    std::vector<LevelDecoration> decorations;
};

DecorationRuntimeSaveFields captureDecorationRuntimeFields(
    std::string_view mapName, const Blob &decodedBase,
    const EventRuntimeMapEntry &events, const DecorationTable &table,
    std::span<const LevelDecoration> live, GameTitle title);
SaveSection encodeDecorationRuntimeSection(const DecorationRuntimeSaveFields &fields,
                                           GameTitle title);
DecorationRuntimeSaveFields decodeDecorationRuntimeSection(
    const SaveContainer &container, GameTitle title);
void requireDecorationRuntimeBaseAgreement(
    const DecorationRuntimeSaveFields &fields, std::string_view mapName,
    const Blob &decodedBase, const EventRuntimeMapEntry &events,
    const DecorationTable &table, GameTitle title);
// Decoded-base overload supports synthetic source-backed unit fixtures.
void requireDecorationRuntimeBaseAgreement(
    const DecorationRuntimeSaveFields &fields, std::string_view mapName,
    const OutdoorLocation_MM7 &base, const EventRuntimeMapEntry &events,
    const DecorationTable &table, GameTitle title);

/**
 * @param fields                        Saved DECR list.
 * @param rebuilt                       Decorations the native map load rebuilt, after `PrepareDecorations`.
 * @param mapName                       File name of the loaded map.
 * @param title                         Must be MM6.
 * @return                              Copy of the saved decoration list. Throws if the map or count differs, or if
 *                                      a slot's position, yaw, cog, event or trigger range differs from the rebuilt
 *                                      native decoration.
 */
std::vector<LevelDecoration> stageDecorationRuntimeReplay(
    const DecorationRuntimeSaveFields &fields, std::span<const LevelDecoration> rebuilt,
    std::string_view mapName, GameTitle title);

} // namespace mm6
