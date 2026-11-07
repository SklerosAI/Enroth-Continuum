#pragma once

#include <algorithm>
#include <stdexcept>
#include <string>

#include "MM6ChestSaveFields.h"
#include "MM6CoreSaveFields.h"
#include "MM6MapSaveFields.h"
#include "MM6RecoverySaveFields.h"
#include "MM6SaveMetadata.h"

namespace mm6 {

// Shared by the provisional v1 and raw v2 manifests. These checks do not
// validate the map bytes against loaded MM6 resources; that is a later stage.
inline void validateSharedSaveCrossFields(
    const CoreSaveFields &core, const SaveMetadataFields &metadata,
    const MapSaveFields &maps, const RecoverySaveFields &recovery,
    const ChestSaveFields &chest) {
    validateMetadataAgainstCore(metadata, core);
    validateRecoveryAgainstCore(recovery, core);
    if (maps.deltas.size() != 67)
        throw std::runtime_error("MM6 save requires 67 map deltas");

    const std::string &mapName = metadata.header.mapFileName;
    if (mapName.size() < 4)
        throw std::runtime_error("invalid MM6 active map name");
    const std::string deltaName = mapName.substr(0, mapName.size() - 4) +
        (mapName.ends_with(".blv") ? ".dlv" : ".ddm");
    if (chest.activeDeltaName != deltaName)
        throw std::runtime_error("MM6 CHST sidecar name disagrees with active map");
    if (std::none_of(maps.deltas.begin(), maps.deltas.end(),
                     [&deltaName](const SavedMapDelta &delta) { return delta.name == deltaName; }))
        throw std::runtime_error("MM6 active map has no saved delta companion");
}

} // namespace mm6
