#include "MM6LoadedMapSource.h"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <unordered_set>

#include "Engine/Tables/MapTable.h"
#include "Engine/Snapshots/MM6IndoorSnapshot.h"
#include "Library/Lod/LodReader.h"
#include "MM6Archive.h"
#include "Utility/String/Ascii.h"

namespace mm6 {
namespace {

bool isBaseName(std::string_view name) {
    return name.ends_with(".blv") || name.ends_with(".odm");
}

bool isDeltaName(std::string_view name) {
    return name.ends_with(".dlv") || name.ends_with(".ddm");
}

std::string deltaNameFor(std::string_view base) {
    return std::string(base.substr(0, base.size() - 4)) +
        (base.ends_with(".blv") ? ".dlv" : ".ddm");
}

} // namespace

LoadedMapSource::LoadedMapSource(const LodReader &games, const MapTable &maps, GameTitle title)
    : _games(&games) {
    if (title != GameTitle::MM6 || !games.isOpen())
        throw std::runtime_error("MM6 loaded map source requires the MM6 Games archive");

    std::unordered_set<std::string> archiveMapNames;
    for (const std::string &rawName : games.ls()) {
        const std::string name = ascii::toLower(rawName);
        if (isBaseName(name) || isDeltaName(name))
            archiveMapNames.insert(name);
    }

    std::size_t indoorCount = 0;
    std::size_t outdoorCount = 0;
    for (MapId id : maps.pInfos.indices()) {
        const std::string &base = maps.pInfos[id].fileName;
        if (base.empty())
            continue;
        if (!isBaseName(base) || !_baseNames.insert(base).second)
            throw std::runtime_error("MM6 map table has an invalid or duplicate base name");
        const std::string delta = deltaNameFor(base);
        if (!archiveMapNames.contains(base) || !archiveMapNames.contains(delta))
            throw std::runtime_error("MM6 Games archive lacks a map table pair");
        _deltaNames.push_back(delta);
        _deltaNameSet.insert(delta);
        base.ends_with(".blv") ? ++indoorCount : ++outdoorCount;
    }
    std::sort(_deltaNames.begin(), _deltaNames.end());
    if (_deltaNames.size() != 67 || indoorCount != 52 || outdoorCount != 15 ||
        archiveMapNames.size() != 134 || _baseNames.size() != 67 || _deltaNameSet.size() != 67)
        throw std::runtime_error("MM6 loaded map source has incomplete map pairs");
}

Blob LoadedMapSource::readBase(std::string_view name) const {
    if (!_baseNames.contains(std::string(name)))
        throw std::runtime_error("MM6 loaded map base is outside the table");
    Blob entry = _games->read(name);
    return name.ends_with(".blv") ? decodeMM6IndoorBaseArchiveEntry(name, entry) :
                                    decodeMM6ArchiveEntry(entry);
}

Blob LoadedMapSource::readDefaultDelta(std::string_view name) const {
    if (!_deltaNameSet.contains(std::string(name)))
        throw std::runtime_error("MM6 loaded map delta is outside the table");
    return _games->read(name);
}

} // namespace mm6
