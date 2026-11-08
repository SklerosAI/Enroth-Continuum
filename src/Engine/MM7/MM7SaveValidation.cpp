#include "Engine/MM7/MM7SaveValidation.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <exception>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Engine/Objects/ItemEnums.h"
#include "Engine/Objects/MonsterEnums.h"
#include "Engine/Objects/SpriteEnums.h"
#include "Engine/Snapshots/CompositeSnapshots.h"
#include "Engine/Spells/SpellEnums.h"

#include "GUI/GUIDialogues.h"

#include "Library/Image/Pcx.h"
#include "Library/Lod/LodReader.h"
#include "Library/LodFormats/LodFormatSnapshots.h"
#include "Library/LodFormats/LodFormats.h"

#include "Utility/Exception.h"
#include "Utility/Streams/MemoryInputStream.h"
#include "Utility/String/Ascii.h"

namespace mm7 {
namespace {

class DeltaCursor {
 public:
    explicit DeltaCursor(const Blob &blob)
        : _bytes(static_cast<const std::uint8_t *>(blob.data()), blob.size()) {}

    bool take(std::size_t bytes) {
        if (bytes > _bytes.size() - _offset)
            return false;
        _offset += bytes;
        return true;
    }

    bool takeRecords(std::size_t count, std::size_t recordBytes) {
        return recordBytes && count <= (_bytes.size() - _offset) / recordBytes && take(count * recordBytes);
    }

    bool takeCountedRecords(std::size_t recordBytes) {
        if (_bytes.size() - _offset < sizeof(std::uint32_t))
            return false;
        const std::uint32_t count = static_cast<std::uint32_t>(_bytes[_offset]) |
                                    static_cast<std::uint32_t>(_bytes[_offset + 1]) << 8 |
                                    static_cast<std::uint32_t>(_bytes[_offset + 2]) << 16 |
                                    static_cast<std::uint32_t>(_bytes[_offset + 3]) << 24;
        _offset += sizeof(std::uint32_t);
        return takeRecords(count, recordBytes);
    }

    bool finished() const { return _offset == _bytes.size(); }

 private:
    std::span<const std::uint8_t> _bytes;
    std::size_t _offset = 0;
};

bool validItemId(std::int32_t id) {
    return id == std::to_underlying(ITEM_NULL) ||
           (id >= std::to_underlying(ITEM_FIRST_VALID) &&
            id <= std::to_underlying(ITEM_LAST_VALID));
}

bool safeSpriteItemReferences(const std::vector<SpriteObject_MM7> &sprites) {
    return std::all_of(sprites.begin(), sprites.end(), [](const SpriteObject_MM7 &sprite) {
        return (sprite.uAttributes & std::to_underlying(SPRITE_MISSILE)) ||
               validItemId(sprite.containing_item.itemId);
    });
}

bool safeActorReferences(const std::vector<Actor_MM7> &actors) {
    return std::all_of(actors.begin(), actors.end(), [](const Actor_MM7 &actor) {
        const std::uint16_t monsterId = actor.monsterInfo.id;
        const std::uint32_t hostilityGroup = actor.hostilityGroup;
        return monsterId >= std::to_underlying(MONSTER_FIRST) &&
               monsterId <= std::to_underlying(MONSTER_LAST) &&
               actor.monsterId >= std::to_underlying(MONSTER_INVALID) &&
               actor.monsterId <= std::to_underlying(MONSTER_LAST) &&
               actor.uniqueNameIndex >= 0 && actor.uniqueNameIndex <= 30 &&
               (hostilityGroup <= std::to_underlying(MONSTER_TYPE_LAST) ||
                hostilityGroup == 9999);
    });
}

template<class Delta>
bool safeCommonDeltaReferences(const Delta &delta) {
    return safeActorReferences(delta.actors) &&
           safeSpriteItemReferences(delta.spriteObjects) &&
           std::all_of(delta.chests.begin(), delta.chests.end(),
                       [](const Chest_MM7 &chest) { return detail::hasSafeChestReferences(chest); });
}

std::string activeMapName(const SaveGameHeader_MM7 &header) {
    const auto end = std::find(header.locationName.begin(), header.locationName.end(), '\0');
    if (end == header.locationName.end())
        return {};
    return ascii::toLower(std::string_view(header.locationName.data(), end - header.locationName.begin()));
}

bool validMapName(std::string_view name) {
    if (name.size() < 5 || name.find('/') != std::string_view::npos ||
        name.find('\\') != std::string_view::npos || name.find("..") != std::string_view::npos)
        return false;
    return name.ends_with(".odm") || name.ends_with(".blv");
}

bool validateMapDelta(std::string_view deltaName, const Blob &source,
                      const LodReader &gamesLod) {
    const bool indoor = deltaName.ends_with(".dlv");
    std::string baseName(deltaName);
    baseName.replace(baseName.size() - 4, 4, indoor ? ".blv" : ".odm");
    if (!validMapName(baseName) || !gamesLod.exists(baseName))
        return false;

    // The active MM7 Games LOD is selected outside this preflight and is
    // treated as the read-only source of map geometry and delta sizes.
    Blob baseBytes = detail::decodeBoundedSaveEntry(gamesLod.read(baseName));
    Blob deltaBytes = detail::decodeBoundedSaveEntry(source);
    if (indoor) {
        auto base = std::make_unique<IndoorLocation_MM7>();
        deserialize(baseBytes, base.get());
        if (!detail::hasBoundedDeltaRecords(deltaBytes, true, base->faces.size(),
                                            base->decorations.size(), base->doorCount,
                                            base->header.doorsDataSizeBytes))
            return false;
        auto delta = std::make_unique<IndoorDelta_MM7>();
        deserialize(deltaBytes, delta.get(), tags::context(*base));
        if (!detail::hasSafeIndoorDeltaReferences(*delta, base->vertices.size(),
                                                   base->faces.size(), base->sectors.size()))
            return false;
    } else {
        auto base = std::make_unique<OutdoorLocation_MM7>();
        deserialize(baseBytes, base.get());
        std::size_t faceCount = 0;
        for (const BSPModelData_MM7 &model : base->models) {
            if (model.numFaces > detail::maxDecodedSaveEntryBytes - faceCount)
                return false;
            faceCount += model.numFaces;
        }
        if (!detail::hasBoundedDeltaRecords(deltaBytes, false, faceCount,
                                            base->decorations.size()))
            return false;
        auto delta = std::make_unique<OutdoorDelta_MM7>();
        deserialize(deltaBytes, delta.get(), tags::context(*base));
        if (!detail::hasSafeOutdoorDeltaReferences(*delta))
            return false;
    }
    return true;
}

} // namespace

Blob detail::decodeBoundedSaveEntry(const Blob &source) {
    if (source.size() > maxDecodedSaveEntryBytes)
        throw Exception("MM7 save entry exceeds the decoded-size limit");
    if (lod::detectCompressedData(source)) {
        MemoryInputStream stream(source.data(), source.size());
        LodCompressionHeader_MM6 header{};
        deserialize(stream, &header);
        if (header.decompressedSize > maxDecodedSaveEntryBytes)
            throw Exception("MM7 compressed save entry declares excessive output");
    } else if (lod::detectCompressedPseudoImage(source)) {
        MemoryInputStream stream(source.data(), source.size());
        LodImageHeader_MM6 header{};
        deserialize(stream, &header);
        if (header.decompressedSize > maxDecodedSaveEntryBytes)
            throw Exception("MM7 pseudo-image entry declares excessive output");
    }
    Blob decoded = lod::decodeMaybeCompressed(source);
    if (decoded.size() > maxDecodedSaveEntryBytes)
        throw Exception("MM7 decoded save entry exceeds the size limit");
    return decoded;
}

bool detail::hasBoundedDeltaRecords(const Blob &decoded, bool indoor,
                                     std::size_t faceCount, std::size_t decorationCount,
                                     std::size_t doorCount, std::size_t doorDataBytes) {
    if (decoded.size() > maxDecodedSaveEntryBytes || doorDataBytes % sizeof(std::int16_t))
        return false;
    DeltaCursor cursor(decoded);
    if (!cursor.take(sizeof(LocationHeader_MM7)))
        return false;
    if (indoor) {
        if (!cursor.take(875))
            return false;
    } else if (!cursor.take(2 * 88 * 11)) {
        return false;
    }
    if (!cursor.takeRecords(faceCount, sizeof(std::uint32_t)) ||
        !cursor.takeRecords(decorationCount, sizeof(std::uint16_t)) ||
        !cursor.takeCountedRecords(sizeof(Actor_MM7)) ||
        !cursor.takeCountedRecords(sizeof(SpriteObject_MM7)) ||
        !cursor.takeCountedRecords(sizeof(Chest_MM7)))
        return false;
    if (indoor && (!cursor.takeRecords(doorCount, sizeof(BLVDoor_MM7)) ||
                   !cursor.take(doorDataBytes)))
        return false;
    return cursor.take(sizeof(PersistentVariables_MM7) + sizeof(std::int64_t) + sizeof(MapWeather_MM7)) &&
           cursor.finished();
}

bool detail::hasSafeCharacterInventoryReferences(const Character_MM7 &character) {
    if (!std::all_of(character.inventoryItems.begin(), character.inventoryItems.end(),
                     [](const Item_MM7 &item) { return validItemId(item.itemId); }))
        return false;
    constexpr std::int32_t itemCount = 126;
    if (!std::all_of(character.inventoryMatrix.begin(), character.inventoryMatrix.end(),
                     [](std::int32_t index) { return index <= 0 || index <= itemCount; }))
        return false;
    return std::all_of(character.equipment.begin(), character.equipment.end(),
                       [](std::uint32_t index) { return index <= static_cast<std::uint32_t>(itemCount); });
}

bool detail::hasSafeChestReferences(const Chest_MM7 &chest) {
    if (chest.chestTypeId >= 8)
        return false;
    // MM7 chests use a 9 x 9 active grid. Unreferenced raw item records can
    // contain stale IDs, including in fresh saves. Reconstruction drops
    // invalid raw IDs but preserves valid random markers outside the grid.
    return std::all_of(chest.inventoryMatrix.begin(), chest.inventoryMatrix.begin() + 81,
                       [&chest](std::int16_t index) {
                           return index <= 0 ||
                                  (index <= 81 && validItemId(chest.items[index - 1].itemId));
                       });
}

bool detail::hasSafePartyInventoryReferences(const Party_MM7 &party) {
    return std::all_of(party.characters.begin(), party.characters.end(),
                       [](const Character_MM7 &character) {
                           return hasSafeCharacterInventoryReferences(character);
                       });
}

bool detail::hasSafePartySemanticReferences(const Party_MM7 &party) {
    if (party.alignment > 2)
        return false;

    const int arenaQuest = party.field_7B5_in_arena_quest;
    if (arenaQuest != -1 && arenaQuest != 0 &&
        (arenaQuest < std::to_underlying(DIALOGUE_ARENA_SELECT_PAGE) ||
         arenaQuest > std::to_underlying(DIALOGUE_ARENA_SELECT_LORD)))
        return false;

    return std::all_of(party.characters.begin(), party.characters.end(),
                       [](const Character_MM7 &character) {
                           // The selected face and voice can temporarily be
                           // 23 or 24 for a zombie; MM7's tables have 25 slots.
                           if (character.sex > 1 || character.classType > 35 ||
                               character.currentFace >= 25 || character.voiceId >= 25 ||
                               character.prevFace < 0 || character.prevFace >= 25 ||
                               character.prevVoiceId < 0 || character.prevVoiceId >= 25 ||
                               character.lastOpenedSpellbookPage < 0 ||
                               character.lastOpenedSpellbookPage > std::to_underlying(MAGIC_SCHOOL_LAST) ||
                               (character.quickSpell != std::to_underlying(SPELL_NONE) &&
                                (character.quickSpell < std::to_underlying(SPELL_FIRST_REGULAR) ||
                                 character.quickSpell > std::to_underlying(SPELL_LAST_REGULAR))))
                               return false;

                           return std::all_of(character.installedBeacons.begin(),
                                              character.installedBeacons.end(),
                                              [](const LloydBeacon_MM7 &beacon) {
                                                  // Inactive slots may retain stale map indices.
                                                  return beacon.beaconTime == 0 ||
                                                         beacon.mapIndexInGamesLod <= 75;
                                              });
                       });
}

bool detail::hasSafeIndoorDeltaReferences(const IndoorDelta_MM7 &delta,
                                          std::size_t vertexCount, std::size_t faceCount,
                                          std::size_t sectorCount) {
    if (!safeCommonDeltaReferences(delta))
        return false;
    std::size_t offset = 0;
    const auto &data = delta.doorsData;
    auto takeIds = [&](std::size_t count, std::size_t bound) {
        if (count > data.size() - offset)
            return false;
        const bool valid = std::all_of(data.begin() + offset, data.begin() + offset + count,
                                       [bound](std::int16_t id) {
                                           return id >= 0 && static_cast<std::size_t>(id) < bound;
                                       });
        offset += count;
        return valid;
    };
    auto skip = [&](std::size_t count) {
        if (count > data.size() - offset)
            return false;
        offset += count;
        return true;
    };
    for (const BLVDoor_MM7 &door : delta.doors) {
        // Closed-door reconstruction indexes offsets by vertex number.
        if (door.numOffsets < door.numVertices ||
            !takeIds(door.numVertices, vertexCount) ||
            !takeIds(door.numFaces, faceCount) ||
            !takeIds(door.numSectors, sectorCount) ||
            !skip(door.numFaces) || !skip(door.numFaces) ||
            !skip(door.numOffsets) || !skip(door.numOffsets) ||
            !skip(door.numOffsets))
            return false;
    }
    return true;
}

bool detail::hasSafeOutdoorDeltaReferences(const OutdoorDelta_MM7 &delta) {
    return safeCommonDeltaReferences(delta);
}

bool preflightNativeSaveStructure(std::span<const std::uint8_t> bytes,
                                  GameTitle title, const LodReader &gamesLod) {
    if (title != GameTitle::MM7 || !gamesLod.isOpen() || bytes.empty() ||
        bytes.size() > detail::maxDecodedSaveEntryBytes)
        return false;
    try {
        auto save = std::make_unique<SaveGame_MM7>();
        deserialize(Blob::view(bytes.data(), bytes.size()), save.get());

        const std::string currentMap = activeMapName(save->header);
        if (!validMapName(currentMap) || !gamesLod.exists(currentMap))
            return false;
        std::string currentDelta = currentMap;
        currentDelta[currentDelta.size() - 3] = 'd';
        if (!save->mapDeltas.contains(currentDelta))
            return false;
        if (!detail::hasSafePartyInventoryReferences(save->party) ||
            !detail::hasSafePartySemanticReferences(save->party))
            return false;

        // The current menu tolerates a bad thumbnail; this detached preflight
        // uses the stricter policy until legacy-save compatibility is checked.
        (void) pcx::decode(save->thumbnail);
        for (int player = 0; player < 4; ++player) {
            for (int beacon = 0; beacon < 5; ++beacon) {
                const auto key = std::pair(player, beacon);
                const bool active = save->party.characters[player].installedBeacons[beacon].beaconTime != 0;
                const auto image = save->lloydImages.find(key);
                if (active && image == save->lloydImages.end())
                    return false;
                if (image != save->lloydImages.end())
                    (void) pcx::decode(image->second);
            }
        }
        for (const auto &[name, delta] : save->mapDeltas)
            if (!validateMapDelta(name, delta, gamesLod))
                return false;
        return true;
    } catch (const std::exception &) {
        return false;
    }
}

} // namespace mm7
