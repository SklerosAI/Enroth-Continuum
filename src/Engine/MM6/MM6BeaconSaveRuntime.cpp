#include "MM6BeaconSaveRuntime.h"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "Engine/Objects/Character.h"
#include "Engine/Tables/MapTable.h"

namespace mm6 {
namespace {

MapId resolveMapId(std::string_view mapName, const MapTable &mapTable,
                   std::span<const std::string> expectedDeltaNames) {
    if (mapName.size() < 5 || (!mapName.ends_with(".blv") && !mapName.ends_with(".odm")))
        throw std::runtime_error("MM6 beacon map filename has an invalid extension");
    const std::string deltaName = std::string(mapName.substr(0, mapName.size() - 4)) +
        (mapName.ends_with(".blv") ? ".dlv" : ".ddm");
    if (std::count(expectedDeltaNames.begin(), expectedDeltaNames.end(), deltaName) != 1)
        throw std::runtime_error("MM6 beacon map is absent from verified delta set");

    MapId result = MAP_INVALID;
    for (MapId id : mapTable.pInfos.indices()) {
        if (mapTable.pInfos[id].fileName != mapName)
            continue;
        if (result != MAP_INVALID)
            throw std::runtime_error("ambiguous MM6 beacon map filename");
        result = id;
    }
    if (result == MAP_INVALID)
        throw std::runtime_error("MM6 beacon map is absent from current map table");
    return result;
}

} // namespace

std::vector<std::uint8_t> copyRgbaPixels(RgbaImageView image, int width, int height) {
    if (!image || image.width() != width || image.height() != height)
        throw std::runtime_error("invalid MM6 screenshot dimensions");
    std::vector<std::uint8_t> result;
    result.reserve(static_cast<std::size_t>(width * height * 4));
    for (const Color &pixel : image.pixels()) {
        result.push_back(pixel.r);
        result.push_back(pixel.g);
        result.push_back(pixel.b);
        result.push_back(pixel.a);
    }
    return result;
}

BeaconSaveRecord captureBeaconRecord(const LloydBeacon &beacon, RgbaImageView image,
                                     const MapTable &mapTable,
                                     std::span<const std::string> expectedDeltaNames,
                                     GameTitle title) {
    if (title != GameTitle::MM6 || expectedDeltaNames.size() != 67 ||
        beacon.mapId < MAP_FIRST || beacon.mapId > MAP_LAST)
        throw std::runtime_error("MM6 beacon capture requires a bounded MM6 map");
    const std::string &mapName = mapTable.pInfos[beacon.mapId].fileName;
    if (resolveMapId(mapName, mapTable, expectedDeltaNames) != beacon.mapId)
        throw std::runtime_error("MM6 beacon runtime map identity is inconsistent");
    BeaconSaveRecord record;
    record.expiresAtTicks = beacon.uBeaconTime.ticks();
    record.mapFileName = mapName;
    record.position = {beacon._partyPos.x, beacon._partyPos.y, beacon._partyPos.z};
    record.yaw = beacon._partyViewYaw;
    record.pitch = beacon._partyViewPitch;
    record.unknown = beacon.unknown;
    record.rgba = copyRgbaPixels(image, BEACON_IMAGE_WIDTH, BEACON_IMAGE_HEIGHT);
    BeaconSaveFields probe;
    probe.beacons[0][0] = record;
    (void) encodeBeaconSection(probe);
    return record;
}

StagedBeaconVisuals stageBeaconVisuals(const BeaconSaveFields &fields, const MapTable &mapTable,
                                       std::span<const std::string> expectedDeltaNames,
                                       GameTitle title) {
    if (title != GameTitle::MM6 || expectedDeltaNames.size() != 67)
        throw std::runtime_error("MM6 beacon stage requires a verified MM6 map set");
    (void) encodeBeaconSection(fields); // Validate all bounds before allocating staged images.
    StagedBeaconVisuals result;
    result.thumbnailRgba = fields.thumbnailRgba;
    for (std::size_t character = 0; character < fields.beacons.size(); ++character) {
        for (std::size_t slot = 0; slot < fields.beacons[character].size(); ++slot) {
            const auto &saved = fields.beacons[character][slot];
            if (!saved)
                continue;
            StagedBeacon staged;
            staged.expiresAtTicks = saved->expiresAtTicks;
            staged.mapId = resolveMapId(saved->mapFileName, mapTable, expectedDeltaNames);
            staged.position = saved->position;
            staged.yaw = saved->yaw;
            staged.pitch = saved->pitch;
            staged.unknown = saved->unknown;
            staged.rgba = saved->rgba;
            result.beacons[character][slot] = std::move(staged);
        }
    }
    return result;
}

} // namespace mm6
