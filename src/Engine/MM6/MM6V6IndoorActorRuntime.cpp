#include "MM6V6IndoorActorRuntime.h"

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "Engine/Snapshots/MM6OutdoorDeltaSnapshot.h"
#include "Engine/Objects/MonsterEnumFunctions.h"

#include "MM6NativeActorNpcPolicy.h"
#include "MM6V6IndoorSaveFields.h"

namespace mm6 {
namespace {

constexpr std::size_t NATIVE_WIDTH = 548;
constexpr std::size_t MAX_ACTORS = 8192;

void requireSourceShape(std::string_view mapName,
                        const IndoorNativeSlotAnchor &source, GameTitle title) {
    if (title != GameTitle::MM6 || mapName != source.mapName ||
        !mapName.ends_with(".blv") || source.count > MAX_ACTORS ||
        source.records.size() != source.count * NATIVE_WIDTH)
        throw std::runtime_error("MM6 IART lacks an exact IACR indoor source");
}

void requireOccupiedNativeSlot(const Actor &runtime, const Actor_MM7 &native,
                               std::size_t slot) {
    // The map loader assigns street NPC links after restoring the native actor records. The v6 GNPC join validates
    // each such link and its row, IACR still anchors the native identity that preceded generation.
    const MonsterId nativeMonster = static_cast<MonsterId>(native.monsterInfo.id);
    const bool generatedFromNative = getsStreetNpc(nativeMonster, native.npcId) &&
        runtime.npcId >= 5000 && runtime.npcId < 5100;
    const bool clearedNative = runtime.npcId == 0 && clearsNativeActorNpc(nativeMonster, native.npcId);
    if (runtime.id != slot ||
        (runtime.npcId != native.npcId && !generatedFromNative && !clearedNative) ||
        std::to_underlying(runtime.monsterId) != native.monsterId ||
        std::to_underlying(runtime.monsterInfo.id) != native.monsterInfo.id ||
        runtime.initialPosition != native.initialPosition.toFloat())
        throw std::runtime_error("MM6 IART native actor identity differs at slot " +
                                 std::to_string(slot));
}

} // namespace

ActorRuntimeSaveFields captureIndoorActorRuntime(
    std::string_view mapName, const IndoorNativeSlotAnchor &source,
    std::span<const Actor> live, GameTitle title) {
    requireSourceShape(mapName, source, title);
    if (live.size() < source.count || live.size() > MAX_ACTORS)
        throw std::runtime_error("MM6 IART live list lacks native IACR prefix");
    ActorRuntimeSaveFields result;
    result.mapName = mapName;
    result.nativePrefixCount = source.count;
    result.actors.assign(live.begin(), live.end());
    requireIndoorActorRuntimeSourceAgreement(result, mapName, source, title);
    return result;
}

void requireIndoorActorRuntimeSourceAgreement(
    const ActorRuntimeSaveFields &fields, std::string_view mapName,
    const IndoorNativeSlotAnchor &source, GameTitle title) {
    requireSourceShape(mapName, source, title);
    if (fields.mapName != mapName || fields.nativePrefixCount != source.count ||
        fields.actors.size() < source.count || fields.actors.size() > MAX_ACTORS)
        throw std::runtime_error("MM6 IART map or native prefix count differs from IACR");
    for (std::size_t slot = 0; slot < fields.actors.size(); ++slot) {
        if (fields.actors[slot].id != slot)
            throw std::runtime_error("MM6 IART actor PID differs at slot " +
                                     std::to_string(slot));
        if (slot < source.count) {
            const auto native = projectMM6ActorRecord(
                std::span(source.records).subspan(slot * NATIVE_WIDTH, NATIVE_WIDTH));
            requireOccupiedNativeSlot(fields.actors[slot], native, slot);
        } else if (fields.actors[slot].npcId < 0) {
            throw std::runtime_error(
                "MM6 IART appended actor has no native signed NPC anchor");
        }
    }
    (void) encodeActorRuntimeSectionAs(
        fields, ActorRuntimeSectionKind::IndoorV6, title);
}

std::vector<Actor> stageIndoorActorRuntime(
    const ActorRuntimeSaveFields &fields, std::string_view mapName,
    const IndoorNativeSlotAnchor &source, GameTitle title) {
    requireIndoorActorRuntimeSourceAgreement(fields, mapName, source, title);
    return fields.actors;
}

void applyIndoorActorRuntimeToDetachedList(
    const ActorRuntimeSaveFields &fields, std::string_view mapName,
    const IndoorNativeSlotAnchor &source, std::vector<Actor> *rebuilt,
    GameTitle title) {
    if (!rebuilt)
        throw std::runtime_error("MM6 IART lacks a replay destination");
    auto desired = stageIndoorActorRuntime(fields, mapName, source, title);
    if (rebuilt->size() < source.count)
        throw std::runtime_error("MM6 IART rebuilt list lacks native prefix");
    for (std::size_t slot = 0; slot < source.count; ++slot) {
        const auto native = projectMM6ActorRecord(
            std::span(source.records).subspan(slot * NATIVE_WIDTH, NATIVE_WIDTH));
        requireOccupiedNativeSlot((*rebuilt)[slot], native, slot);
    }
    rebuilt->swap(desired);
}

} // namespace mm6
