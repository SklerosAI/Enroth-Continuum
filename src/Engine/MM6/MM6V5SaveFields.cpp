#include "MM6V5SaveFields.h"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mm6 {
namespace {

constexpr std::array<char, 4> SPRT = {'S', 'P', 'R', 'T'};
constexpr std::array<char, 4> ARTR = {'A', 'R', 'T', 'R'};
constexpr std::array<char, 4> DECR = {'D', 'E', 'C', 'R'};

void requireManifest(const SaveContainer &container) {
    if (container.version != 5 || container.sections.size() != 26 ||
        std::count_if(container.sections.begin(), container.sections.end(),
                      [](const SaveSection &section) {
                          return section.tag == SPRT && section.required;
                      }) != 1 ||
        std::count_if(container.sections.begin(), container.sections.end(),
                      [](const SaveSection &section) {
                          return section.tag == ARTR && section.required;
                      }) != 1 ||
        std::count_if(container.sections.begin(), container.sections.end(),
                      [](const SaveSection &section) {
                          return section.tag == DECR && section.required;
                      }) != 1)
        throw std::runtime_error("MM6 v5 requires twenty-six exact sections");
}

void requireNativeJoin(const V5SaveFields &fields, GameTitle title) {
    const std::string &mapName = fields.v4.v3.raw.metadata.header.mapFileName;
    if (fields.sprites.mapName != mapName || !mapName.ends_with(".odm"))
        throw std::runtime_error("MM6 v5 SPRT differs from active HEAD map");
    const std::string deltaName = mapName.substr(0, mapName.size() - 4) + ".ddm";
    const auto &chest = fields.v4.v3.raw.chest;
    if (chest.activeDeltaName != deltaName)
        throw std::runtime_error("MM6 v5 SPRT lacks matching CHST map");
    const auto &deltas = fields.v4.v3.raw.maps.deltas;
    const auto active = std::find_if(deltas.begin(), deltas.end(),
                                     [&deltaName](const SavedMapDelta &entry) {
                                         return entry.name == deltaName;
                                     });
    if (active == deltas.end() || active->bytes != chest.rawActiveDelta)
        throw std::runtime_error("MM6 v5 native MAPS and CHST anchors differ");
    requireSpriteRuntimeNativeAgreement(fields.sprites, mapName,
                                        chest.rawActiveDelta, title);
    requireActorRuntimeBaselineAgreement(fields.actors, mapName,
                                         fields.v4.v3.activeActorPayload, title);
    if (fields.decorations.mapName != mapName)
        throw std::runtime_error("MM6 v5 DECR differs from active HEAD map");
}

} // namespace

std::vector<std::uint8_t> encodeV5Save(const V5SaveFields &fields, GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 v5 save requires MM6 title");
    requireNativeJoin(fields, title);
    SaveContainer container = decodeSaveContainer(encodeV4Save(fields.v4, title));
    container.version = 5;
    container.sections.push_back(encodeSpriteRuntimeSection(fields.sprites, title));
    container.sections.push_back(encodeActorRuntimeSection(fields.actors, title));
    container.sections.push_back(encodeDecorationRuntimeSection(fields.decorations, title));
    requireManifest(container);
    return encodeSaveContainer(container);
}

V5SaveFields decodeV5Save(std::span<const std::uint8_t> bytes, GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 v5 save requires MM6 title");
    SaveContainer container = decodeSaveContainer(bytes);
    requireManifest(container);
    V5SaveFields result;
    result.sprites = decodeSpriteRuntimeSection(container, title);
    result.actors = decodeActorRuntimeSection(container, title);
    result.decorations = decodeDecorationRuntimeSection(container, title);
    container.version = 4;
    std::erase_if(container.sections, [](const SaveSection &section) {
        return section.tag == SPRT || section.tag == ARTR || section.tag == DECR;
    });
    result.v4 = decodeV4Save(encodeSaveContainer(container), title);
    requireNativeJoin(result, title);
    if (encodeV5Save(result, title) != std::vector<std::uint8_t>(bytes.begin(), bytes.end()))
        throw std::runtime_error("MM6 v5 save is not canonical");
    return result;
}

} // namespace mm6
