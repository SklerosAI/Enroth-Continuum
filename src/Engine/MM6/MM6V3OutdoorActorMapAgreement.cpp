#include "MM6V3OutdoorActorMapAgreement.h"

#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "MM6ActiveActorSection.h"

namespace mm6 {
namespace {

bool sameNativeActorAnchor(const Actor_MM7 &left, const Actor_MM7 &right) {
    return left.monsterInfo.id == right.monsterInfo.id &&
           left.monsterId == right.monsterId &&
           left.initialPosition == right.initialPosition;
}

bool sameNativeActorAnchor(const Actor_MM7 &native, const Actor &actor, size_t slot) {
    return actor.id == slot &&
           native.monsterInfo.id == std::to_underlying(actor.monsterInfo.id) &&
           native.monsterId == std::to_underlying(actor.monsterId) &&
           native.initialPosition.toFloat() == actor.initialPosition;
}

} // namespace

V3OutdoorActorMapAgreement stageV3OutdoorActorMapAgreement(
    std::span<const std::uint8_t> bytes,
    std::span<const std::string> expectedNames,
    const MapBaseReader &readBase,
    const GroundItemVerifier &verifyGroundItem,
    std::size_t runtimeActorCap,
    GameTitle title) {
    if (title != GameTitle::MM6 || runtimeActorCap == 0)
        throw std::runtime_error("MM6 v3 outdoor actor map needs title and actor cap");
    V3SaveFields saved = decodeV3Save(bytes, title);
    ValidatedRawMapSave validated = decodeRawSaveWithActiveMapCounts(
        encodeRawSave(saved.raw, title), expectedNames, readBase, verifyGroundItem, title);
    if (!std::holds_alternative<StagedOutdoorMap>(validated.activeMap))
        throw std::runtime_error("MM6 v3 actor map is not outdoor");
    StagedOutdoorMap active = std::get<StagedOutdoorMap>(std::move(validated.activeMap));
    const std::string &mapName = saved.raw.metadata.header.mapFileName;
    if (active.mapName != mapName || saved.generatedNpcs.mapName != mapName ||
        saved.raw.chest.activeDeltaName != mapName.substr(0, mapName.size() - 4) + ".ddm")
        throw std::runtime_error("MM6 v3 actor map differs from HEAD or CHST");

    MM6OutdoorDeltaSnapshot sidecar = decodeMM6OutdoorDelta(Blob::view(
        saved.raw.chest.rawActiveDelta.data(), saved.raw.chest.rawActiveDelta.size()));
    const auto &delta = active.delta;
    // OutdoorLocation::Load receives playing days + 1 and, while loading a
    // save, uses 0x1BAF800 days as its respawn interval. Check the same day
    // relationship without the overflow-prone Time::toSeconds multiplication.
    constexpr std::int64_t saveLoadRespawnInterval = 0x1BAF800;
    const std::int64_t ticks = saved.raw.core.party.playingTicks;
    if (ticks > std::numeric_limits<std::int64_t>::max() / 30)
        throw std::runtime_error("MM6 v3 Party clock cannot select an outdoor respawn day");
    const std::int64_t playingDay = (ticks * 30 / 128) / 86400 + 1;
    if (playingDay > std::numeric_limits<int>::max() ||
        delta.lastRespawnDay > playingDay ||
        playingDay - delta.lastRespawnDay >= saveLoadRespawnInterval)
        throw std::runtime_error("MM6 v3 active DDM would respawn on save load");
    if (delta.lastRespawnDay == 0 ||
        delta.lastRespawnDay > std::numeric_limits<int>::max() ||
        sidecar.lastRespawnDay != delta.lastRespawnDay ||
        sidecar.respawnCount != delta.respawnCount ||
        sidecar.actors.size() != delta.actors.size() ||
        validated.active.actors != delta.actors.size())
        throw std::runtime_error("MM6 v3 active DDM lacks a matching no-respawn day or actor shape");

    std::vector<Actor> actors = decodeActiveActorSection(saved.activeActorPayload, title);
    if (delta.actors.size() > actors.size() || actors.size() > runtimeActorCap)
        throw std::runtime_error("MM6 v3 ACTR actor count differs from native DDM or runtime cap");
    for (size_t slot = 0; slot < delta.actors.size(); ++slot)
        if (!sameNativeActorAnchor(delta.actors[slot], sidecar.actors[slot]) ||
            !sameNativeActorAnchor(delta.actors[slot], actors[slot], slot))
            throw std::runtime_error("MM6 v3 native actor identity differs from ACTR or CHST");
    for (size_t slot = delta.actors.size(); slot < actors.size(); ++slot)
        if (actors[slot].id != slot)
            throw std::runtime_error("MM6 v3 ACTR spawned actor slot is discontinuous");

    return {std::move(saved), std::move(active), std::move(sidecar), std::move(actors)};
}

} // namespace mm6
