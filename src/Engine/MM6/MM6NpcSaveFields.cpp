#include "MM6NpcSaveFields.h"

#include <bit>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace mm6 {
namespace {

constexpr std::array<char, 4> RESC_TAG = {'R', 'E', 'S', 'C'};
constexpr std::uint8_t VERSION = 1;
constexpr size_t MAX_NAME_BYTES = 1024;
constexpr size_t MAX_PAYLOAD = 1 + 2 + 1 + 2 + 501 * MAX_NPC_RECORD_BYTES + 51 * 2;

class Reader {
 public:
    explicit Reader(std::span<const std::uint8_t> bytes) : _bytes(bytes) {}

    std::uint8_t u8() {
        if (_offset >= _bytes.size())
            throw std::runtime_error("truncated MM6 RESC field");
        return _bytes[_offset++];
    }

    std::uint16_t u16() {
        std::uint16_t value = u8();
        return value | (std::uint16_t(u8()) << 8);
    }

    std::uint32_t u32() {
        std::uint32_t value = 0;
        for (int shift = 0; shift < 32; shift += 8)
            value |= std::uint32_t(u8()) << shift;
        return value;
    }

    std::string string(size_t length) {
        if (length > _bytes.size() - _offset)
            throw std::runtime_error("truncated MM6 RESC name");
        std::string result(reinterpret_cast<const char *>(_bytes.data() + _offset), length);
        _offset += length;
        return result;
    }

    void finish() const {
        if (_offset != _bytes.size())
            throw std::runtime_error("trailing MM6 RESC bytes");
    }

 private:
    std::span<const std::uint8_t> _bytes;
    size_t _offset = 0;
};

void put8(std::vector<std::uint8_t> *bytes, std::uint8_t value) { bytes->push_back(value); }

void put16(std::vector<std::uint8_t> *bytes, std::uint16_t value) {
    put8(bytes, value);
    put8(bytes, value >> 8);
}

void put32(std::vector<std::uint8_t> *bytes, std::uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8)
        put8(bytes, value >> shift);
}

void validateNpc(const NPCData &npc) {
    // MM6's source NPC table uses -1 for a character with no house.
    if (npc.name.size() > MAX_NAME_BYTES || npc.name.find('\0') != std::string::npos ||
        std::to_underlying(npc.sex) > 1 || std::to_underlying(npc.house) < -1 ||
        std::to_underlying(npc.house) > std::to_underlying(HOUSE_MM6_LAST) ||
        std::to_underlying(npc.profession) < 0 ||
        std::to_underlying(npc.profession) > std::to_underlying(NPC_PROFESSION_MM6_LAST))
        throw std::runtime_error("invalid MM6 NPC field");
}

void validate(const NpcSaveFields &fields) {
    if (fields.activeCount < 0 || fields.activeCount > fields.npcData.size())
        throw std::runtime_error("invalid MM6 NPC active count");
    for (const NPCData &npc : fields.npcData)
        validateNpc(npc);
}

void encodeNpc(std::vector<std::uint8_t> *bytes, const NPCData &npc) {
    put16(bytes, npc.name.size());
    bytes->insert(bytes->end(), npc.name.begin(), npc.name.end());
    put32(bytes, npc.portraitId);
    put32(bytes, static_cast<std::uint32_t>(npc.flags));
    put32(bytes, std::bit_cast<std::uint32_t>(npc.fame));
    put32(bytes, std::bit_cast<std::uint32_t>(npc.rep));
    put32(bytes, std::to_underlying(npc.house));
    put32(bytes, std::to_underlying(npc.profession));
    put32(bytes, std::bit_cast<std::uint32_t>(npc.greetingIndex));
    put8(bytes, npc.canJoin ? 1 : 0);
    put32(bytes, std::bit_cast<std::uint32_t>(npc.field_24));
    put32(bytes, npc.dialogue_1_evt_id);
    put32(bytes, npc.dialogue_2_evt_id);
    put32(bytes, npc.dialogue_3_evt_id);
    put32(bytes, npc.dialogue_4_evt_id);
    put32(bytes, npc.dialogue_5_evt_id);
    put32(bytes, npc.dialogue_6_evt_id);
    put8(bytes, std::to_underlying(npc.sex));
    put32(bytes, std::bit_cast<std::uint32_t>(npc.hasUsedAbility));
    put32(bytes, std::bit_cast<std::uint32_t>(npc.newsTopic));
}

NPCData decodeNpc(Reader *reader) {
    NPCData npc;
    const size_t nameLength = reader->u16();
    if (nameLength > MAX_NAME_BYTES)
        throw std::runtime_error("MM6 RESC name exceeds size limit");
    npc.name = reader->string(nameLength);
    npc.portraitId = reader->u32();
    npc.flags = NpcFlags(reader->u32());
    npc.fame = std::bit_cast<std::int32_t>(reader->u32());
    npc.rep = std::bit_cast<std::int32_t>(reader->u32());
    npc.house = static_cast<HouseId>(std::bit_cast<std::int32_t>(reader->u32()));
    npc.profession = static_cast<NpcProfession>(reader->u32());
    npc.greetingIndex = std::bit_cast<std::int32_t>(reader->u32());
    const std::uint8_t canJoin = reader->u8();
    if (canJoin > 1)
        throw std::runtime_error("invalid MM6 RESC join flag");
    npc.canJoin = canJoin;
    npc.field_24 = std::bit_cast<std::int32_t>(reader->u32());
    npc.dialogue_1_evt_id = reader->u32();
    npc.dialogue_2_evt_id = reader->u32();
    npc.dialogue_3_evt_id = reader->u32();
    npc.dialogue_4_evt_id = reader->u32();
    npc.dialogue_5_evt_id = reader->u32();
    npc.dialogue_6_evt_id = reader->u32();
    npc.sex = static_cast<Sex>(reader->u8());
    npc.hasUsedAbility = std::bit_cast<std::int32_t>(reader->u32());
    npc.newsTopic = std::bit_cast<std::int32_t>(reader->u32());
    return npc;
}

void requireMM6(GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 NPC save fields cannot apply to another title");
}

} // namespace

std::vector<std::uint8_t> encodeNpcRecord(const NPCData &npc) {
    validateNpc(npc);
    std::vector<std::uint8_t> bytes;
    bytes.reserve(2 + npc.name.size() + 66);
    encodeNpc(&bytes, npc);
    return bytes;
}

NPCData decodeNpcRecord(std::span<const std::uint8_t> bytes) {
    if (bytes.size() > MAX_NPC_RECORD_BYTES)
        throw std::runtime_error("MM6 NPC record exceeds size limit");
    Reader reader(bytes);
    NPCData npc = decodeNpc(&reader);
    reader.finish();
    validateNpc(npc);
    return npc;
}

SaveSection encodeNpcSection(const NpcSaveFields &fields) {
    validate(fields);
    std::vector<std::uint8_t> bytes;
    bytes.reserve(MAX_PAYLOAD);
    put8(&bytes, VERSION);
    put16(&bytes, fields.npcData.size());
    put8(&bytes, fields.groups.size());
    put16(&bytes, fields.activeCount);
    for (const NPCData &npc : fields.npcData)
        encodeNpc(&bytes, npc);
    for (std::uint16_t group : fields.groups)
        put16(&bytes, group);
    return {RESC_TAG, true, std::move(bytes)};
}

NpcSaveFields decodeNpcSection(const SaveContainer &container) {
    const SaveSection *section = nullptr;
    for (const SaveSection &candidate : container.sections) {
        if (candidate.tag == RESC_TAG) {
            if (section)
                throw std::runtime_error("duplicate MM6 RESC section");
            section = &candidate;
        }
    }
    if (!section || !section->required || section->payload.size() > MAX_PAYLOAD)
        throw std::runtime_error("missing or oversized MM6 RESC section");

    Reader reader(section->payload);
    if (reader.u8() != VERSION || reader.u16() != 501 || reader.u8() != 51)
        throw std::runtime_error("unsupported MM6 RESC version or count");
    NpcSaveFields fields;
    fields.activeCount = reader.u16();
    for (NPCData &npc : fields.npcData)
        npc = decodeNpc(&reader);
    for (std::uint16_t &group : fields.groups)
        group = reader.u16();
    reader.finish();
    validate(fields);
    return fields;
}

void loadNpcFieldsBeforeMutation(std::span<const std::uint8_t> bytes, NpcSaveFields *destination) {
    if (!destination)
        throw std::runtime_error("null MM6 NPC save destination");
    NpcSaveFields staged = decodeNpcSection(decodeSaveContainer(bytes));
    *destination = std::move(staged);
}

NpcSaveFields captureNpcFields(const NPCStats &stats, GameTitle title) {
    requireMM6(title);
    NpcSaveFields fields{stats.pNPCData, stats.pGroups, stats.uNumNewNPCs};
    (void) encodeNpcSection(fields);
    return fields;
}

NPCStats stageNpcStats(const NPCStats &live, std::span<const std::uint8_t> bytes, GameTitle title) {
    requireMM6(title);
    NpcSaveFields fields;
    loadNpcFieldsBeforeMutation(bytes, &fields);
    NPCStats staged = live;
    staged.pNPCData = std::move(fields.npcData);
    staged.pGroups = fields.groups;
    staged.uNumNewNPCs = fields.activeCount;
    return staged;
}

} // namespace mm6
