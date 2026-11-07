#include "MM6IndoorWorldPlan.h"

#include <stdexcept>
#include <string_view>
#include <utility>

#include "MM6RawSaveRuntime.h"

namespace mm6 {
namespace {

std::vector<DetachedIndoorDoorReferences> checkedDoors(
    const MM6IndoorRuntimeDelta &delta, const IndoorLocation_MM7 &base) {
    if (delta.doors.size() != base.doorCount)
        throw std::runtime_error("MM6 indoor door count disagrees with its base");
    std::vector<DetachedIndoorDoorReferences> result;
    result.reserve(delta.doors.size());
    std::size_t cursor = 0;
    for (const BLVDoor_MM7 &door : delta.doors) {
        const std::size_t needed = std::size_t(door.numVertices) +
            3 * std::size_t(door.numFaces) + std::size_t(door.numSectors) +
            3 * std::size_t(door.numOffsets);
        if (door.numOffsets < door.numVertices || cursor > delta.doorsData.size() ||
            needed > delta.doorsData.size() - cursor)
            throw std::runtime_error("MM6 indoor door arrays exceed their base shape");
        DetachedIndoorDoorReferences refs;
        for (std::size_t i = 0; i < door.numVertices; ++i) {
            const int id = delta.doorsData[cursor++];
            if (id < 0 || std::size_t(id) >= base.vertices.size())
                throw std::runtime_error("MM6 indoor door references an absent vertex");
            refs.vertices.push_back(std::size_t(id));
        }
        for (std::size_t i = 0; i < door.numFaces; ++i) {
            const int id = delta.doorsData[cursor++];
            if (id < 0 || std::size_t(id) >= base.faces.size())
                throw std::runtime_error("MM6 indoor door references an absent face");
            refs.faces.push_back(std::size_t(id));
        }
        for (std::size_t i = 0; i < door.numSectors; ++i) {
            const int id = delta.doorsData[cursor++];
            if (id < 0 || std::size_t(id) >= base.sectors.size())
                throw std::runtime_error("MM6 indoor door references an absent sector");
            refs.sectors.push_back(std::size_t(id));
        }
        cursor += 2 * std::size_t(door.numFaces) + 3 * std::size_t(door.numOffsets);
        result.push_back(std::move(refs));
    }
    if (cursor != delta.doorsData.size())
        throw std::runtime_error("MM6 indoor door arrays leave unused words");
    return result;
}

} // namespace

DetachedIndoorWorldPlan prepareDetachedRawIndoorWorldPlan(
    const StagedRawTitleOwners &raw, GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 indoor plan requires the MM6 title");
    const auto *active = std::get_if<StagedIndoorMap>(&raw.activeMap);
    if (!active || !active->mapName.ends_with(".blv") ||
        active->mapName != raw.saved.metadata.header.mapFileName ||
        raw.saved.chest.activeDeltaName !=
            active->mapName.substr(0, active->mapName.size() - 4) + ".dlv")
        throw std::runtime_error("MM6 indoor plan lacks matching active map and CHST owners");

    DetachedIndoorWorldPlan plan;
    plan.mapName = active->mapName;
    plan.base = projectMM6IndoorBase(active->base);
    // The writer rechecks logical fields against the retained MM6 wire before
    // a second decode. It rejects a mutated door or unsupported count.
    plan.savedDelta = decodeMM6IndoorRuntimeDelta(
        encodeMM6IndoorRuntimeDelta(active->delta, active->base), active->base);
    plan.activeSidecar = decodeMM6IndoorRuntimeDelta(
        Blob::view(raw.saved.chest.rawActiveDelta.data(),
                   raw.saved.chest.rawActiveDelta.size()), active->base);
    if (plan.savedDelta.actors.size() != raw.activeCounts.actors ||
        plan.savedDelta.spriteObjects.size() != raw.activeCounts.sprites ||
        plan.savedDelta.chests.size() != raw.activeCounts.chests ||
        plan.activeSidecar.actors.size() != raw.activeCounts.actors ||
        plan.activeSidecar.spriteObjects.size() != raw.activeCounts.sprites ||
        plan.activeSidecar.chests.size() != raw.activeCounts.chests)
        throw std::runtime_error("MM6 indoor plan has mismatched active record counts");
    plan.savedDoors = checkedDoors(plan.savedDelta, plan.base);
    plan.sidecarDoors = checkedDoors(plan.activeSidecar, plan.base);
    for (std::size_t i = 0; i < plan.savedDoors.size(); ++i) {
        const auto &saved = plan.savedDoors[i];
        const auto &sidecar = plan.sidecarDoors[i];
        if (saved.vertices != sidecar.vertices || saved.faces != sidecar.faces ||
            saved.sectors != sidecar.sectors ||
            plan.savedDelta.doors[i].numOffsets != plan.activeSidecar.doors[i].numOffsets)
            throw std::runtime_error("MM6 indoor DLV and CHST door topology disagree");
    }
    return plan;
}

} // namespace mm6
