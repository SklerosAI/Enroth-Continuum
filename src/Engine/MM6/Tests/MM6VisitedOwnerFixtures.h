#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "Engine/Engine.h"
#include "Engine/MM6/MM6ActiveActorSection.h"
#include "Engine/MM6/MM6NpcStaticBaseline.h"
#include "Engine/MM6/MM6V5SaveFields.h"
#include "Engine/MM6/MM6V6IndoorSaveFields.h"
#include "Engine/MM6/MM6VisitedIndoorRuntimeOwner.h"
#include "Engine/MM6/MM6VisitedOutdoorRuntimeOwner.h"
#include "Engine/Tables/DecorationTable.h"
#include "Engine/Tables/ItemTable.h"
#include "Utility/Memory/Blob.h"

namespace mm6::testing {

inline std::vector<std::uint8_t> fixtureIndoorBase() {
    std::vector<std::uint8_t> bytes(136 + 10 * 4, 0);
    bytes[136 + 4 * 4] = 200;
    return bytes;
}

inline std::vector<std::uint8_t> fixtureIndoorDelta() {
    std::vector<std::uint8_t> bytes(8 + 875 + 3 * 4 + 200 * 80 + 200 + 56, 0);
    bytes[4] = 5; // Native last-respawn day, matches the EVRT entry.
    return bytes;
}

inline std::vector<std::uint8_t> fixtureOutdoorBase() {
    return std::vector<std::uint8_t>(160 + 16 + 3 * 128 * 128 + 4 + 4 + 4 + 128 * 128 * 4 + 4, 0);
}

inline std::vector<std::uint8_t> fixtureOutdoorDelta() {
    return std::vector<std::uint8_t>(8 + 2 * 88 * 11 + 3 * 4 + 200 + 56, 0);
}

inline RawSaveFields fixtureRaw(std::string_view activeMap, std::string_view activeDelta,
                                const std::vector<std::uint8_t> &activeDeltaBytes) {
    RawSaveFields raw;
    raw.core.party.playingTicks = 42;
    raw.core.party.activeCharacter = 1;
    for (auto &character : raw.core.characters)
        character.name = "Hero";
    raw.inventory.ready = true;
    raw.metadata.header.displayName = "Visited fixture";
    raw.metadata.header.mapFileName = activeMap;
    raw.metadata.header.playingTicks = 42;
    raw.metadata.ui.activeCharacter = 1;
    for (int index = 0; index < 52; ++index)
        raw.maps.deltas.push_back({"d" + std::to_string(index) + ".dlv", fixtureIndoorDelta()});
    for (int index = 0; index < 15; ++index)
        raw.maps.deltas.push_back({"o" + std::to_string(index) + ".ddm", fixtureOutdoorDelta()});
    raw.chest = {0x12345678, {}, std::string(activeDelta), activeDeltaBytes};
    raw.npcs.activeCount = 399;
    raw.recovery.playingTicks = 42;
    raw.shops = emptyNewGameShopTitleState(GameTitle::MM6);
    return raw;
}

inline V6IndoorSaveFields fixtureIndoorSave() {
    V6IndoorSaveFields result;
    result.raw = fixtureRaw("d0.blv", "d0.dlv", fixtureIndoorDelta());
    result.generatedNpcs.mapName = "d0.blv";
    result.gameplayRng.type = RANDOM_ENGINE_SEQUENTIAL;
    result.gameplayRng.sequentialState = 12345;
    EventRuntimeMapEntry event;
    event.mapName = "d0.blv";
    event.lastRespawnDay = 5;
    result.eventRuntime.maps.push_back(event);
    result.actors = {"d0.blv", 0, {}};
    result.sprites = {"d0.blv", 0, {}};
    result.decorations = {"d0.blv", 0, {}};
    result.runtimeDecorations.mapName = "d0.blv";
    result.runtimeSprites.mapName = "d0.blv";
    result.runtimeActors.mapName = "d0.blv";
    return result;
}

inline V5SaveFields fixtureOutdoorSave() {
    V5SaveFields result;
    auto &v3 = result.v4.v3;
    v3.raw = fixtureRaw("o0.odm", "o0.ddm", fixtureOutdoorDelta());
    v3.activeActorPayload = encodeActiveActorSection(std::array<Actor, 0>{}, GameTitle::MM6);
    v3.generatedNpcs.mapName = "o0.odm";
    v3.gameplayRng.type = RANDOM_ENGINE_SEQUENTIAL;
    v3.gameplayRng.sequentialState = 12345;
    EventRuntimeMapEntry event;
    event.mapName = "o0.odm";
    event.lastRespawnDay = 5;
    result.v4.eventRuntime.maps.push_back(event);
    result.sprites.mapName = "o0.odm";
    result.actors.mapName = "o0.odm";
    result.decorations.mapName = "o0.odm";
    return result;
}

inline NpcStaticBaseline fixtureNpcBaseline() {
    const auto table = [](std::string_view value) { return Blob::copy(value.data(), value.size()); };
    return loadNpcStaticBaseline(
        table("Header\r\nHeader\r\n1\tStatic Ada\t123\t0\t0\t0\t1\t2\t1\t7\t11\t12\t13\r\n"),
        table("0\tNews\tunused\tTown news\r\n"),
        table("Header\r\nAdam\tBeth\r\n"),
        table("1\tProfession\t2\t100\tunused\tAction\tBenefit\tJoin\r\n"),
        GameTitle::MM6);
}

inline std::vector<IndoorFaceState> fixtureNoFaces(std::string_view) {
    return {};
}

struct FixtureItemTableScope {
    ItemTable table;
    ItemTable *previous = pItemTable;

    FixtureItemTableScope() { pItemTable = &table; }
    ~FixtureItemTableScope() { pItemTable = previous; }
};

inline StagedV6IndoorSaveFields fixtureStagedIndoorSave(const V6IndoorSaveFields &fields,
                                                        const DecorationTable &decorations) {
    std::vector<std::string> names;
    for (const auto &delta : fields.raw.maps.deltas)
        names.push_back(delta.name);
    const auto readBase = [](std::string_view name) {
        const auto bytes = name.ends_with(".blv") ? fixtureIndoorBase() : fixtureOutdoorBase();
        return Blob::copy(bytes.data(), bytes.size());
    };
    FixtureItemTableScope items;
    return stageV6IndoorSave(encodeV6IndoorSave(fields, GameTitle::MM6), fixtureNpcBaseline(), names, readBase,
                             [](std::int32_t id) { return id >= 1 && id <= 580; },
                             items.table, decorations, 5, 145, fixtureNoFaces, GameTitle::MM6);
}

inline VisitedOutdoorRuntimeOwner fixtureVisitedOutdoor(std::string_view mapName) {
    std::string deltaName(mapName);
    deltaName.replace(deltaName.size() - 4, 4, ".ddm");
    VisitedOutdoorRuntimeOwner owner;
    owner.mapName = mapName;
    owner.nativeDelta = {deltaName, fixtureOutdoorDelta()};
    owner.actorPayload = encodeActiveActorSection(std::array<Actor, 0>{}, GameTitle::MM6);
    owner.generatedNpcs.mapName = mapName;
    owner.chest = {0x12345678, {}, deltaName, fixtureOutdoorDelta()};
    owner.event.mapName = mapName;
    owner.event.lastRespawnDay = 5;
    owner.actors.mapName = mapName;
    owner.sprites.mapName = mapName;
    owner.decorations.mapName = mapName;
    return owner;
}

inline VisitedIndoorRuntimeOwner renamedVisitedIndoor(VisitedIndoorRuntimeOwner owner, std::string_view mapName) {
    std::string deltaName(mapName);
    deltaName.replace(deltaName.size() - 4, 4, ".dlv");
    owner.mapName = mapName;
    owner.nativeDelta.name = deltaName;
    owner.chest.activeDeltaName = deltaName;
    owner.generatedNpcs.mapName = mapName;
    owner.event.mapName = mapName;
    if (owner.faceOverrides)
        owner.faceOverrides->mapName = mapName;
    owner.nativeActors.mapName = mapName;
    owner.nativeSprites.mapName = mapName;
    owner.nativeDecorations.mapName = mapName;
    owner.decorations.mapName = mapName;
    owner.sprites.mapName = mapName;
    owner.actors.mapName = mapName;
    return owner;
}

} // namespace mm6::testing
