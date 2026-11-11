#include "MM6GeneratedNpcSaveFields.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <deque>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

#include "Engine/Objects/MonsterEnumFunctions.h"

#include "MM6NativeActorNpcPolicy.h"
#include "MM6NpcSaveFields.h"

namespace mm6 {
namespace {

constexpr std::array<char, 4> TAG = {'G', 'N', 'P', 'C'};
constexpr std::uint8_t VERSION = 1;
constexpr size_t MAX_MAP_NAME = 63;
constexpr size_t MAX_ROWS = 100;
constexpr size_t MAX_PAYLOAD = 3 + MAX_MAP_NAME + MAX_ROWS * (6 + MAX_NPC_RECORD_BYTES);

void requireMM6(GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 generated NPC fields require MM6 title");
}

bool validMapName(std::string_view name) {
    if (name.empty() || name.size() > MAX_MAP_NAME ||
        !(name.ends_with(".odm") || name.ends_with(".blv")))
        return false;
    return std::all_of(name.begin(), name.end(), [](unsigned char c) {
        return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
               c == '_' || c == '.';
    });
}

void put16(std::vector<std::uint8_t> *out, std::uint16_t value) {
    out->push_back(value);
    out->push_back(value >> 8);
}

class Reader {
 public:
    explicit Reader(std::span<const std::uint8_t> bytes) : _bytes(bytes) {}

    std::uint8_t u8() {
        if (_offset >= _bytes.size())
            throw std::runtime_error("truncated MM6 GNPC section");
        return _bytes[_offset++];
    }

    std::uint16_t u16() {
        const std::uint16_t lo = u8();
        return lo | (std::uint16_t(u8()) << 8);
    }

    std::span<const std::uint8_t> take(size_t length) {
        if (length > _bytes.size() - _offset)
            throw std::runtime_error("truncated MM6 GNPC record");
        const auto result = _bytes.subspan(_offset, length);
        _offset += length;
        return result;
    }

    void finish() const {
        if (_offset != _bytes.size())
            throw std::runtime_error("trailing MM6 GNPC bytes");
    }

 private:
    std::span<const std::uint8_t> _bytes;
    size_t _offset = 0;
};

void validateRow(const GeneratedNpcRow &row) {
    if (row.monster < MONSTER_FIRST || row.monster > MONSTER_LAST ||
        !nativeStreetNpcEligible(row.monster) || row.npc.name.empty() ||
        row.npc.portraitId == 0 || !row.npc.canJoin ||
        row.npc.field_24 != 1 || row.npc.house != HOUSE_INVALID ||
        row.npc.sex != sexForMonsterType(monsterTypeForMonsterId(row.monster)) ||
        row.npc.profession < NPC_PROFESSION_FIRST ||
        row.npc.profession >= NPC_PROFESSION_MM6_LAST)
        throw std::runtime_error("MM6 GNPC row is not a street NPC");
    (void) encodeNpcRecord(row.npc);
}

void validate(const GeneratedNpcSaveFields &fields) {
    if (!validMapName(fields.mapName) || fields.rows.size() > MAX_ROWS)
        throw std::runtime_error("invalid MM6 GNPC map or count");
    size_t previous = 0;
    for (size_t index = 0; index < fields.rows.size(); ++index) {
        const GeneratedNpcRow &row = fields.rows[index];
        if (index && row.actorSlot <= previous)
            throw std::runtime_error("MM6 GNPC actor slots are not increasing");
        previous = row.actorSlot;
        validateRow(row);
    }
}

} // namespace

GeneratedNpcSaveFields captureGeneratedNpcFields(
    const std::deque<Actor> &actors, const NPCStats &npcs,
    std::string_view mapName, GameTitle title) {
    requireMM6(title);
    if (!validMapName(mapName) || npcs.uNewlNPCBufPos < 0 ||
        static_cast<size_t>(npcs.uNewlNPCBufPos) > npcs.pAdditionalNPC.size())
        throw std::runtime_error("invalid MM6 generated NPC source");
    GeneratedNpcSaveFields result;
    result.mapName = mapName;
    for (size_t slot = 0; slot < actors.size(); ++slot) {
        const Actor &actor = actors[slot];
        if (actor.id != slot)
            throw std::runtime_error("invalid MM6 generated NPC actor source");
        // Pre-map native markers are not generated rows. The original MM6
        // map-load loop clears the marker of an ineligible actor.
        // The native source remains anchored separately from this capsule.
        if (actor.npcId < 5000)
            continue;
        if (slot > std::numeric_limits<std::uint16_t>::max() ||
            result.rows.size() >= MAX_ROWS ||
            actor.npcId != 5000 + result.rows.size())
            throw std::runtime_error("MM6 generated NPC links are not contiguous");
        result.rows.push_back({static_cast<std::uint16_t>(slot),
                               actor.monsterInfo.id,
                               npcs.pAdditionalNPC[result.rows.size()]});
    }
    if (result.rows.size() != static_cast<size_t>(npcs.uNewlNPCBufPos))
        throw std::runtime_error("MM6 generated NPC cursor differs from actors");
    validate(result);
    return result;
}

SaveSection encodeGeneratedNpcSection(const GeneratedNpcSaveFields &fields,
                                      GameTitle title) {
    requireMM6(title);
    validate(fields);
    std::vector<std::uint8_t> bytes;
    bytes.reserve(3 + fields.mapName.size() + fields.rows.size() * 80);
    bytes.push_back(VERSION);
    bytes.push_back(static_cast<std::uint8_t>(fields.mapName.size()));
    bytes.insert(bytes.end(), fields.mapName.begin(), fields.mapName.end());
    bytes.push_back(static_cast<std::uint8_t>(fields.rows.size()));
    for (const GeneratedNpcRow &row : fields.rows) {
        const auto record = encodeNpcRecord(row.npc);
        put16(&bytes, row.actorSlot);
        put16(&bytes, std::to_underlying(row.monster));
        put16(&bytes, record.size());
        bytes.insert(bytes.end(), record.begin(), record.end());
    }
    return {TAG, true, std::move(bytes)};
}

GeneratedNpcSaveFields decodeGeneratedNpcSection(const SaveContainer &container,
                                                  GameTitle title) {
    requireMM6(title);
    const SaveSection *section = nullptr;
    for (const SaveSection &candidate : container.sections) {
        if (candidate.tag == TAG) {
            if (section)
                throw std::runtime_error("duplicate MM6 GNPC section");
            section = &candidate;
        }
    }
    if (!section || !section->required || section->payload.size() > MAX_PAYLOAD)
        throw std::runtime_error("missing or oversized MM6 GNPC section");
    Reader reader(section->payload);
    if (reader.u8() != VERSION)
        throw std::runtime_error("unsupported MM6 GNPC version");
    GeneratedNpcSaveFields fields;
    const auto name = reader.take(reader.u8());
    fields.mapName.assign(reinterpret_cast<const char *>(name.data()), name.size());
    const auto count = reader.u8();
    if (count > MAX_ROWS)
        throw std::runtime_error("MM6 GNPC row count exceeds pool");
    fields.rows.reserve(count);
    for (size_t index = 0; index < count; ++index) {
        GeneratedNpcRow row;
        row.actorSlot = reader.u16();
        row.monster = static_cast<MonsterId>(reader.u16());
        row.npc = decodeNpcRecord(reader.take(reader.u16()));
        fields.rows.push_back(std::move(row));
    }
    reader.finish();
    validate(fields);
    return fields;
}

void requireGeneratedNpcActorAgreement(const GeneratedNpcSaveFields &fields,
                                       const std::deque<Actor> &actors,
                                       std::string_view loadedMapName,
                                       GameTitle title) {
    requireMM6(title);
    validate(fields);
    if (fields.mapName != loadedMapName)
        throw std::runtime_error("MM6 GNPC belongs to another map");
    size_t found = 0;
    for (size_t slot = 0; slot < actors.size(); ++slot) {
        const Actor &actor = actors[slot];
        if (actor.npcId < 5000)
            continue;
        if (found >= fields.rows.size() || fields.rows[found].actorSlot != slot ||
            fields.rows[found].monster != actor.monsterInfo.id ||
            actor.npcId != 5000 + found)
            throw std::runtime_error("MM6 GNPC actor links differ after map load");
        ++found;
    }
    if (found != fields.rows.size())
        throw std::runtime_error("MM6 GNPC actor links are incomplete");
}

} // namespace mm6
