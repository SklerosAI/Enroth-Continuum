#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "Engine/GameTitle.h"
#include "Engine/MapEnums.h"

#include "MM6SaveContainer.h"

namespace mm6 {

// Exact current-runtime screenshot dimensions. Pixels are serialized as RGBA,
// not PCX or an MM7 Games LOD index.
constexpr int BEACON_IMAGE_WIDTH = 92;
constexpr int BEACON_IMAGE_HEIGHT = 68;
constexpr int SAVE_THUMBNAIL_WIDTH = 150;
constexpr int SAVE_THUMBNAIL_HEIGHT = 112;

struct BeaconSaveRecord {
    std::int64_t expiresAtTicks = 0;
    std::string mapFileName; // Lowercase MM6 base-map filename, including .blv/.odm.
    std::array<float, 3> position{};
    std::int16_t yaw = 0;
    std::int16_t pitch = 0;
    std::uint16_t unknown = 0;
    std::vector<std::uint8_t> rgba;

    bool operator==(const BeaconSaveRecord &) const = default;
};

struct BeaconSaveFields {
    std::vector<std::uint8_t> thumbnailRgba; // Empty in an offline new-game candidate.
    std::array<std::array<std::optional<BeaconSaveRecord>, 5>, 4> beacons{};

    bool operator==(const BeaconSaveFields &) const = default;
};

struct StagedBeacon {
    std::int64_t expiresAtTicks = 0;
    MapId mapId = MAP_INVALID; // Resolved from the current MM6 map table by filename.
    std::array<float, 3> position{};
    std::int16_t yaw = 0;
    std::int16_t pitch = 0;
    std::uint16_t unknown = 0;
    std::vector<std::uint8_t> rgba; // CPU bytes only; GraphicsImage/GPU publication is later.
};

struct StagedBeaconVisuals {
    std::vector<std::uint8_t> thumbnailRgba;
    std::array<std::array<std::optional<StagedBeacon>, 5>, 4> beacons{};
};

/** Encode/decode the standalone required BEAC v1 section. */
SaveSection encodeBeaconSection(const BeaconSaveFields &fields);
BeaconSaveFields decodeBeaconSection(const SaveContainer &container);
void loadBeaconFieldsBeforeMutation(std::span<const std::uint8_t> bytes,
                                    BeaconSaveFields *destination);

} // namespace mm6
