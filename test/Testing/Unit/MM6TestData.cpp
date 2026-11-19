#include "MM6TestData.h"

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "Library/FileSystem/Lowercase/LowercaseFileSystem.h"
#include "Library/FileSystem/Native/NativeFileSystem.h"
#include "Library/LodFormats/LodFormats.h"

#include "Utility/String/Ascii.h"

MM6TestData::~MM6TestData() = default;

std::unique_ptr<MM6TestData> MM6TestData::open() {
    const char *path = std::getenv("OPENENROTH_MM6_PATH");
    if (!path)
        return nullptr;

    std::unique_ptr<MM6TestData> result(new MM6TestData());
    result->_native = std::make_unique<NativeFileSystem>(Path(path));
    result->_lowercase = std::make_unique<LowercaseFileSystem>(result->_native.get());
    result->_games.open(result->read("data/games.lod"));
    result->_icons.open(result->read("data/icons.lod"));
    return result;
}

Blob MM6TestData::read(std::string_view path) const {
    return _lowercase->read(path);
}

Blob MM6TestData::iconsData(std::string_view name) const {
    return lod::decodeMaybeCompressed(_icons.read(name));
}

std::vector<std::string> MM6TestData::mapNames(std::string_view extension) const {
    std::vector<std::string> result;
    for (const std::string &name : _games.ls()) {
        std::string lower = ascii::toLower(name);
        if (lower.ends_with(extension))
            result.push_back(std::move(lower));
    }
    std::ranges::sort(result);
    return result;
}

std::vector<std::string> MM6TestData::activeEventScripts() const {
    std::vector<std::string> result = {"global.evt"};
    for (std::string_view extension : {".blv", ".odm"})
        for (const std::string &map : mapNames(extension))
            result.push_back(map.substr(0, map.size() - extension.size()) + ".evt");
    std::ranges::sort(result);
    return result;
}

std::optional<Blob> readMM6TestRules(std::string_view name) {
    const char *path = std::getenv("OPENENROTH_MM6_RULES_PATH");
    if (!path)
        return std::nullopt;
    return Blob::fromFile(Path(path) / name);
}
