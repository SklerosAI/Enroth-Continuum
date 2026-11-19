#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "Library/Lod/LodReader.h"

#include "Utility/Memory/Blob.h"

class FileSystem;

/**
 * The MM6 install named by `OPENENROTH_MM6_PATH`, for unit tests that run production readers over the shipped files.
 * CI has no MM6 data, so a test that needs it skips when `open()` returns nothing.
 */
class MM6TestData {
 public:
    ~MM6TestData();

    /**
     * @return                          The MM6 data, or `nullptr` if `OPENENROTH_MM6_PATH` is not set.
     */
    static std::unique_ptr<MM6TestData> open();

    /**
     * @param path                      Path of a file in the MM6 data folder, case-insensitive, e.g. `sounds/audio.snd`.
     * @return                          Contents of the file.
     */
    Blob read(std::string_view path) const;

    const LodReader &games() const { return _games; }
    const LodReader &icons() const { return _icons; }

    /**
     * @param name                      Name of an `icons.lod` entry.
     * @return                          The entry decompressed, as the engine reads MM6 tables and event scripts.
     */
    Blob iconsData(std::string_view name) const;

    /**
     * @param extension                 Map file extension, e.g. `.blv`.
     * @return                          Lowercase names of the `games.lod` entries with that extension, sorted.
     */
    std::vector<std::string> mapNames(std::string_view extension) const;

    /**
     * @return                          Lowercase names of the event scripts the game runs, one for every map in
     *                                  `games.lod` plus `global.evt`, sorted.
     */
    std::vector<std::string> activeEventScripts() const;

 private:
    MM6TestData() = default;

 private:
    std::unique_ptr<FileSystem> _native;
    std::unique_ptr<FileSystem> _lowercase;
    LodReader _games;
    LodReader _icons;
};

inline constexpr std::string_view mm6TestDataMissing = "Set OPENENROTH_MM6_PATH to the MM6 data folder";
