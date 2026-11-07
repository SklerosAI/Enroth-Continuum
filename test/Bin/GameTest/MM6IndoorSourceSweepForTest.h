#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "Engine/Graphics/FaceEnums.h"
#include "Engine/MM6/MM6Archive.h"
#include "Engine/MM6/MM6LoadedMapSource.h"
#include "Engine/MM6/MM6NativeActorNpcPolicy.h"
#include "Engine/Snapshots/MM6IndoorSnapshot.h"

namespace mm6::testing {

struct IndoorSourceCaseForTest {
    std::string baseName;
    std::size_t ordinal = 0;
    std::uint32_t features = 0;
    bool exactDefaultDeltaRoundTrip = false;
};

struct IndoorSourceSweepForTest {
    std::vector<IndoorSourceCaseForTest> cases;
    std::set<std::uint32_t> variants;
    bool hasCodecRefusal = false;
    bool hasInitiallyMovingDoor = false;
    bool hasPrevisibleOutlines = false;
    bool hasNegativeActorNpcSource = false;
    bool hasEligibleNativeStreetNpcMarker = false;
    bool hasMultiActiveDoor = false;
    bool hasActorRecords = false;
    bool hasGroundRecords = false;
    bool hasPersistentBytes = false;
};

inline IndoorSourceSweepForTest sweepRetainedIndoorSourcesForTest(
    const LoadedMapSource &maps) {
    constexpr std::size_t kMaximumIndoorPairs = 64;
    constexpr std::size_t kMaximumSourceBytes = 64 * 1024 * 1024;
    IndoorSourceSweepForTest result;
    for (const std::string &deltaName : maps.expectedNames()) {
        if (!deltaName.ends_with(".dlv"))
            continue;
        if (result.cases.size() >= kMaximumIndoorPairs)
            throw std::runtime_error("MM6 indoor sweep exceeded pair bound");
        const std::string baseName = deltaName.substr(0, deltaName.size() - 4) + ".blv";
        const Blob baseBytes = maps.readBase(baseName);
        const Blob deltaBytes = decodeMM6ArchiveEntry(maps.readDefaultDelta(deltaName));
        if (baseBytes.size() > kMaximumSourceBytes ||
            deltaBytes.size() > kMaximumSourceBytes)
            throw std::runtime_error("MM6 indoor sweep exceeded source byte bound");
        const MM6IndoorSnapshot base = deserializeMM6IndoorBase(baseBytes);
        const MM6IndoorRuntimeDelta delta = decodeMM6IndoorRuntimeDelta(deltaBytes, base);
        if (serializeMM6Indoor(base).str() != baseBytes.str() ||
            serializeMM6Indoor(delta.wire).str() != deltaBytes.str())
            throw std::runtime_error("MM6 indoor sweep failed native wire round trip");

        std::size_t activeDoors = 0;
        bool movingDoor = false;
        for (const BLVDoor_MM7 &door : delta.doors) {
            const bool active = door.numVertices || door.numFaces ||
                door.numSectors || door.numOffsets;
            activeDoors += active;
            movingDoor |=
                door.state == static_cast<std::uint16_t>(DOOR_OPENING) ||
                door.state == static_cast<std::uint16_t>(DOOR_CLOSING);
        }
        const bool visible = std::any_of(delta.visibleOutlines.begin(),
            delta.visibleOutlines.end(), [](char value) { return value != 0; });
        const Blob &persistent = delta.wire.section(
            MM6IndoorSectionKind::PersistentBytes).bytes;
        const std::string_view persistentBytes = persistent.str();
        const bool persistentChanged = std::any_of(persistentBytes.begin(),
            persistentBytes.end(), [](char value) { return value != 0; });
        const bool negativeActorNpc = std::any_of(delta.actors.begin(),
            delta.actors.end(), [](const Actor_MM7 &actor) {
                return actor.npcId < 0;
            });
        const bool eligibleStreetNpc = std::any_of(delta.actors.begin(),
            delta.actors.end(), [](const Actor_MM7 &actor) {
                return actor.npcId != 0 && nativeStreetNpcEligible(
                    static_cast<MonsterId>(actor.monsterInfo.id));
            });

        // These flags classify engine ownership seams, not private map names,
        // dimensions or asset inventories. The ranking below is deterministic.
        const std::uint32_t features =
            (movingDoor ? 1u << 0 : 0u) |
            (visible ? 1u << 1 : 0u) |
            (activeDoors > 1 ? 1u << 2 : 0u) |
            (!delta.actors.empty() ? 1u << 3 : 0u) |
            (!delta.spriteObjects.empty() ? 1u << 4 : 0u) |
            (persistentChanged ? 1u << 5 : 0u) |
            (negativeActorNpc ? 1u << 6 : 0u);
        bool exactDelta = false;
        try {
            exactDelta = encodeMM6IndoorRuntimeDelta(delta, base).str() ==
                         deltaBytes.str();
        } catch (const std::exception &) {
            // A parsed source can still be refused by the changed-state codec.
            // Keep that distinct from a parser or wire-round-trip failure.
        }
        result.cases.push_back({baseName, result.cases.size(), features, exactDelta});
        result.variants.insert(features);
        result.hasCodecRefusal |= !exactDelta;
        result.hasInitiallyMovingDoor |= movingDoor;
        result.hasPrevisibleOutlines |= visible;
        result.hasNegativeActorNpcSource |= negativeActorNpc;
        result.hasEligibleNativeStreetNpcMarker |= eligibleStreetNpc;
        result.hasMultiActiveDoor |= activeDoors > 1;
        result.hasActorRecords |= !delta.actors.empty();
        result.hasGroundRecords |= !delta.spriteObjects.empty();
        result.hasPersistentBytes |= persistentChanged;
    }
    if (result.cases.size() < 3)
        throw std::runtime_error("MM6 indoor sweep lacks three distinct source pairs");
    return result;
}

inline const IndoorSourceCaseForTest &highestRiskAdditionalIndoorCaseForTest(
    const IndoorSourceSweepForTest &sweep) {
    const auto risk = [](std::uint32_t features) {
        return ((features & (1u << 6)) ? 128 : 0) +
               ((features & (1u << 0)) ? 64 : 0) +
               ((features & (1u << 1)) ? 32 : 0) +
               ((features & (1u << 2)) ? 16 : 0) +
               ((features & (1u << 3)) ? 8 : 0) +
               ((features & (1u << 4)) ? 4 : 0) +
               ((features & (1u << 5)) ? 2 : 0);
    };
    const std::uint32_t first = sweep.cases[0].features;
    const std::uint32_t second = sweep.cases[1].features;
    const IndoorSourceCaseForTest *selected = nullptr;
    for (const IndoorSourceCaseForTest &candidate : sweep.cases) {
        if (candidate.ordinal < 2 || !candidate.exactDefaultDeltaRoundTrip ||
            candidate.features == first || candidate.features == second)
            continue;
        if (!selected || risk(candidate.features) > risk(selected->features))
            selected = &candidate;
    }
    if (!selected)
        throw std::runtime_error("MM6 indoor sweep found no distinct exact-codec fixture");
    return *selected;
}

} // namespace mm6::testing
