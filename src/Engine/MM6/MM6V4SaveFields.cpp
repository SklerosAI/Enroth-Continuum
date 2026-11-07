#include "MM6V4SaveFields.h"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mm6 {
namespace {

constexpr std::array<char, 4> EVRT_TAG = {'E', 'V', 'R', 'T'};

void requireMM6(GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 v4 save requires MM6 title");
}

void requireEventMapsInSave(const V4SaveFields &fields) {
    const std::string_view activeName = fields.v3.raw.metadata.header.mapFileName;
    const auto &entries = fields.eventRuntime.maps;
    if (std::none_of(entries.begin(), entries.end(), [activeName](const EventRuntimeMapEntry &entry) {
            return entry.mapName == activeName;
        }))
        throw std::runtime_error("MM6 v4 EVRT lacks active HEAD map");
    for (const EventRuntimeMapEntry &entry : entries) {
        const bool outdoor = entry.mapName.ends_with(".odm");
        const std::string delta = entry.mapName.substr(0, entry.mapName.size() - 4) +
            (outdoor ? ".ddm" : ".dlv");
        if (std::none_of(fields.v3.raw.maps.deltas.begin(), fields.v3.raw.maps.deltas.end(),
                         [&delta](const SavedMapDelta &saved) { return saved.name == delta; }))
            throw std::runtime_error("MM6 v4 EVRT names a map absent from MAPS");
    }
}

void requireExactManifest(const SaveContainer &container) {
    if (container.version != 4 || container.sections.size() != 23)
        throw std::runtime_error("MM6 v4 save requires twenty-three sections");
    const std::size_t count = std::count_if(container.sections.begin(), container.sections.end(),
        [](const SaveSection &section) { return section.tag == EVRT_TAG && section.required; });
    if (count != 1)
        throw std::runtime_error("MM6 v4 save requires one EVRT section");
}

SaveContainer asV3(SaveContainer container) {
    container.version = 3;
    std::erase_if(container.sections, [](const SaveSection &section) {
        return section.tag == EVRT_TAG;
    });
    return container;
}

} // namespace

std::vector<std::uint8_t> encodeV4Save(const V4SaveFields &fields, GameTitle title) {
    requireMM6(title);
    const SaveSection events = encodeEventRuntimeSection(fields.eventRuntime, title);
    requireEventMapsInSave(fields);
    SaveContainer container = decodeSaveContainer(encodeV3Save(fields.v3, title));
    container.version = 4;
    container.sections.push_back(events);
    requireExactManifest(container);
    return encodeSaveContainer(container);
}

V4SaveFields decodeV4Save(std::span<const std::uint8_t> bytes, GameTitle title) {
    requireMM6(title);
    SaveContainer container = decodeSaveContainer(bytes);
    requireExactManifest(container);
    V4SaveFields result;
    result.eventRuntime = decodeEventRuntimeSection(container, title);
    result.v3 = decodeV3Save(encodeSaveContainer(asV3(std::move(container))), title);
    requireEventMapsInSave(result);
    if (encodeV4Save(result, title) != std::vector<std::uint8_t>(bytes.begin(), bytes.end()))
        throw std::runtime_error("MM6 v4 save is not canonical");
    return result;
}

} // namespace mm6
