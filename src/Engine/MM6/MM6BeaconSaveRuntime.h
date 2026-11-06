#pragma once

#include <array>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "Library/Image/Image.h"

#include "MM6BeaconSaveFields.h"

struct LloydBeacon;
struct MapTable;

namespace mm6 {

/** Copy a source screenshot in explicit RGBA order, after checking its dimensions. */
std::vector<std::uint8_t> copyRgbaPixels(RgbaImageView image, int width, int height);

struct PreparedBeaconPixels {
    std::optional<RgbaImage> thumbnail;
    std::array<std::array<std::optional<RgbaImage>, 5>, 4> beacons{};
};

/** Rebuild validated CPU images before any renderer or Party publication. */
PreparedBeaconPixels prepareBeaconPixels(const StagedBeaconVisuals &staged);

/** Convert one current-runtime beacon to a filename-keyed detached record. */
BeaconSaveRecord captureBeaconRecord(const LloydBeacon &beacon, RgbaImageView image,
                                     const MapTable &mapTable,
                                     std::span<const std::string> expectedDeltaNames,
                                     GameTitle title);

/** Resolve saved names to current MM6 map IDs and retain only CPU image bytes. */
StagedBeaconVisuals stageBeaconVisuals(const BeaconSaveFields &fields, const MapTable &mapTable,
                                       std::span<const std::string> expectedDeltaNames,
                                       GameTitle title);

} // namespace mm6
