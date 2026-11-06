#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

#include "Engine/GameTitle.h"
#include "Engine/Graphics/Overlays.h"
#include "Engine/Timer.h"

#include "MM6SaveContainer.h"

namespace mm6 {

struct CoreSaveFields;

struct SaveHeaderFields {
    std::string displayName;
    std::string mapFileName;
    std::int64_t playingTicks = 0;

    bool operator==(const SaveHeaderFields &) const = default;
};

struct SaveTimerFields {
    std::int64_t elapsedTicks = 0; // Simulation time; frame delta/OS tick are transient.

    bool operator==(const SaveTimerFields &) const = default;
};

struct SaveUiFields {
    std::int8_t activeCharacter = -1; // Duplicates PART; full load must require agreement.
    std::uint8_t resetTransientUi = 1; // Casting/book flash/window state resets on load.

    bool operator==(const SaveUiFields &) const = default;
};

struct SaveOverlayField {
    std::int16_t tableIndex = 0;
    std::int16_t frameTime = 0;
    std::int16_t animationLength = 0;
    std::int16_t x = 0;
    std::int16_t y = 0;
    std::uint16_t packedPid = 0;
    std::int16_t projectedSize = 0;
    std::int32_t damageModifier = 0;

    bool operator==(const SaveOverlayField &) const = default;
};

struct SaveMetadataFields {
    SaveHeaderFields header;
    SaveTimerFields timer;
    SaveUiFields ui;
    std::array<SaveOverlayField, 50> overlays{};

    bool operator==(const SaveMetadataFields &) const = default;
};

struct SessionVisualFields {
    SaveTimerFields timer;
    std::array<SaveOverlayField, 50> overlays{};

    bool operator==(const SessionVisualFields &) const = default;
};

struct StagedSessionVisuals {
    Timer timer;
    ActiveOverlayList overlays;
};

/** Encode required HEAD/TIME/UIST/VISU version-one fields after validation. */
std::array<SaveSection, 4> encodeMetadataSections(const SaveMetadataFields &fields);

/** Decode all four fields into temporary state; no globals are changed. */
SaveMetadataFields decodeMetadataSections(const SaveContainer &container);

/** Reject duplicated clock and active-character fields that disagree with PART. */
void validateMetadataAgainstCore(const SaveMetadataFields &metadata, const CoreSaveFields &core);

/** Decode the full container and every metadata field before assignment. */
void loadMetadataBeforeMutation(std::span<const std::uint8_t> bytes, SaveMetadataFields *destination);

/** Capture metadata from an already initialized MM6 runtime. */
SaveMetadataFields captureMetadata(const std::string &displayName, GameTitle title);

/** Capture the actual MM6 simulation clock and overlay list without global mutation. */
SessionVisualFields captureSessionVisualFields(const Timer &timer, const ActiveOverlayList &overlays,
                                               std::size_t overlayTableCount, GameTitle title);

/** Validate every active overlay, then stage the clock and visuals on copies. */
StagedSessionVisuals stageSessionVisuals(const Timer &liveTimer, const ActiveOverlayList &liveOverlays,
                                        const SessionVisualFields &saved, std::size_t overlayTableCount,
                                        GameTitle title);

/** Validate saved map identity against the loaded MM6 map table without changing the engine. */
SaveHeaderFields stageHeader(const SaveHeaderFields &saved, GameTitle title);

/** Stage the event timer paused at saved simulation time; no OS tick is restored. */
Timer stageEventTimer(const Timer &live, const SaveTimerFields &saved, GameTitle title);

/** Stage all 50 overlays on a copy after checking active indexes against MM6 doverlay.bin. */
ActiveOverlayList stageOverlays(const ActiveOverlayList &live,
                                const std::array<SaveOverlayField, 50> &saved, GameTitle title);

} // namespace mm6
