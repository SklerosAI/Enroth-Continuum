#include "MM6CoreSaveFields.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace mm6 {
namespace {

constexpr std::array<char, 4> PART_TAG = {'P', 'A', 'R', 'T'};
constexpr std::array<char, 4> CHAR_TAG = {'C', 'H', 'A', 'R'};
constexpr std::uint8_t PARTY_VERSION = 4;
constexpr std::uint8_t CHARACTER_VERSION = 4;
constexpr size_t MAX_NAME_BYTES = 63;
constexpr size_t MAX_CHARACTER_PAYLOAD = 4 * (1 + MAX_NAME_BYTES + 4 + 2 + 8 + 12 + 28 + 62 + 13 + 39 + 2 + 8) + 2;

class Reader {
 public:
    explicit Reader(std::span<const std::uint8_t> bytes) : _bytes(bytes) {}

    std::uint8_t u8() {
        if (_offset == _bytes.size())
            throw std::runtime_error("truncated MM6 core save field");
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

    std::uint64_t u64() {
        std::uint64_t value = 0;
        for (int shift = 0; shift < 64; shift += 8)
            value |= std::uint64_t(u8()) << shift;
        return value;
    }

    std::string string(size_t length) {
        if (length > _bytes.size() - _offset)
            throw std::runtime_error("truncated MM6 character name");
        std::string result(reinterpret_cast<const char *>(_bytes.data() + _offset), length);
        _offset += length;
        return result;
    }

    void finish() const {
        if (_offset != _bytes.size())
            throw std::runtime_error("trailing MM6 core save field bytes");
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

void put64(std::vector<std::uint8_t> *bytes, std::uint64_t value) {
    for (int shift = 0; shift < 64; shift += 8)
        put8(bytes, value >> shift);
}

bool validName(const std::string &name) {
    return !name.empty() && name.size() <= MAX_NAME_BYTES &&
           std::none_of(name.begin(), name.end(), [](unsigned char c) { return c < 0x20 || c == 0x7f; });
}

bool validSkill(std::uint16_t joined) {
    if (joined > 0xff || (joined & 0xc0) == 0xc0)
        return false; // MM6 has no Grandmaster flag or simultaneous mastery flags.
    return (joined & 0x3f) != 0 || joined == 0;
}

void validate(const CoreSaveFields &fields) {
    if (fields.party.gold < 0 || fields.party.food < 0 || fields.party.bankGold < 0 ||
        fields.party.playingTicks < 0 || fields.party.activeCharacter < -1 || fields.party.activeCharacter > 3 ||
        std::any_of(fields.party.position.begin(), fields.party.position.end(),
                    [](float component) { return !std::isfinite(component) || std::abs(component) > 10000000; }) ||
        !std::isfinite(fields.party.verticalVelocity) ||
        std::abs(fields.party.verticalVelocity) > 10000000 ||
        fields.party.savedFlightZ < -10000000 || fields.party.savedFlightZ > 10000000 ||
        fields.party.waterLavaTimerTicks < 0 ||
        fields.party.fallStartZ < -10000000 || fields.party.fallStartZ > 10000000 ||
        fields.party.flying > 1 || fields.party.fine < 0)
        throw std::runtime_error("invalid MM6 core party fields");

    for (const CoreCharacterFields &character : fields.characters) {
        if (!validName(character.name) || character.sex > 1 || character.classId >= 18 ||
            character.faceId >= 12 || character.skillPoints < 0 ||
            character.modifiers.birthYear < 0 || character.modifiers.birthYear > 10000 ||
            character.spellPreferences.quickSpell > 99 ||
            character.spellPreferences.spellbookPage > 8 ||
            std::any_of(character.skills.begin(), character.skills.end(),
                        [](std::uint16_t value) { return !validSkill(value); }))
            throw std::runtime_error("invalid MM6 core character fields");
    }
}

std::vector<std::uint8_t> encodeParty(const CorePartyFields &party) {
    std::vector<std::uint8_t> bytes;
    put8(&bytes, PARTY_VERSION);
    put32(&bytes, std::bit_cast<std::uint32_t>(party.gold));
    put32(&bytes, std::bit_cast<std::uint32_t>(party.food));
    put32(&bytes, std::bit_cast<std::uint32_t>(party.bankGold));
    put64(&bytes, std::bit_cast<std::uint64_t>(party.playingTicks));
    for (float component : party.position)
        put32(&bytes, std::bit_cast<std::uint32_t>(component));
    put32(&bytes, std::bit_cast<std::uint32_t>(party.viewYaw));
    put32(&bytes, std::bit_cast<std::uint32_t>(party.viewPitch));
    put8(&bytes, std::bit_cast<std::uint8_t>(party.activeCharacter));
    put32(&bytes, std::bit_cast<std::uint32_t>(party.verticalVelocity));
    put32(&bytes, std::bit_cast<std::uint32_t>(party.savedFlightZ));
    put64(&bytes, std::bit_cast<std::uint64_t>(party.waterLavaTimerTicks));
    put32(&bytes, std::bit_cast<std::uint32_t>(party.fallStartZ));
    put32(&bytes, party.flying);
    put32(&bytes, party.partyFlags);
    put32(&bytes, party.partyFlags2);
    put32(&bytes, std::bit_cast<std::uint32_t>(party.fine));
    put32(&bytes, std::bit_cast<std::uint32_t>(party.deaths));
    put32(&bytes, std::bit_cast<std::uint32_t>(party.prisonTerms));
    put32(&bytes, std::bit_cast<std::uint32_t>(party.bountiesCollected));
    put32(&bytes, std::bit_cast<std::uint32_t>(party.reputation));
    return bytes;
}

CorePartyFields decodeParty(std::span<const std::uint8_t> bytes) {
    if (bytes.size() != 94)
        throw std::runtime_error("invalid MM6 PART field length");
    Reader reader(bytes);
    if (reader.u8() != PARTY_VERSION)
        throw std::runtime_error("unsupported MM6 PART field version");
    CorePartyFields party;
    party.gold = std::bit_cast<std::int32_t>(reader.u32());
    party.food = std::bit_cast<std::int32_t>(reader.u32());
    party.bankGold = std::bit_cast<std::int32_t>(reader.u32());
    party.playingTicks = std::bit_cast<std::int64_t>(reader.u64());
    for (float &component : party.position)
        component = std::bit_cast<float>(reader.u32());
    party.viewYaw = std::bit_cast<std::int32_t>(reader.u32());
    party.viewPitch = std::bit_cast<std::int32_t>(reader.u32());
    party.activeCharacter = std::bit_cast<std::int8_t>(reader.u8());
    party.verticalVelocity = std::bit_cast<float>(reader.u32());
    party.savedFlightZ = std::bit_cast<std::int32_t>(reader.u32());
    party.waterLavaTimerTicks = std::bit_cast<std::int64_t>(reader.u64());
    party.fallStartZ = std::bit_cast<std::int32_t>(reader.u32());
    party.flying = reader.u32();
    party.partyFlags = reader.u32();
    party.partyFlags2 = reader.u32();
    party.fine = std::bit_cast<std::int32_t>(reader.u32());
    party.deaths = std::bit_cast<std::int32_t>(reader.u32());
    party.prisonTerms = std::bit_cast<std::int32_t>(reader.u32());
    party.bountiesCollected = std::bit_cast<std::int32_t>(reader.u32());
    party.reputation = std::bit_cast<std::int32_t>(reader.u32());
    reader.finish();
    return party;
}

std::vector<std::uint8_t> encodeCharacters(const std::array<CoreCharacterFields, 4> &characters) {
    std::vector<std::uint8_t> bytes;
    bytes.reserve(MAX_CHARACTER_PAYLOAD);
    put8(&bytes, CHARACTER_VERSION);
    put8(&bytes, characters.size());
    for (const CoreCharacterFields &character : characters) {
        put8(&bytes, character.name.size());
        bytes.insert(bytes.end(), character.name.begin(), character.name.end());
        put8(&bytes, character.sex);
        put8(&bytes, character.classId);
        put8(&bytes, character.faceId);
        put8(&bytes, character.voiceId);
        put16(&bytes, character.level);
        put64(&bytes, character.experience);
        put32(&bytes, std::bit_cast<std::uint32_t>(character.health));
        put32(&bytes, std::bit_cast<std::uint32_t>(character.mana));
        put32(&bytes, std::bit_cast<std::uint32_t>(character.skillPoints));
        for (std::int32_t stat : character.baseStats)
            put32(&bytes, std::bit_cast<std::uint32_t>(stat));
        for (std::uint16_t skill : character.skills)
            put16(&bytes, skill);
        for (size_t offset = 0; offset < character.spells.size(); offset += 8) {
            std::uint8_t packed = 0;
            for (size_t bit = 0; bit < 8 && offset + bit < character.spells.size(); ++bit)
                packed |= std::uint8_t(character.spells[offset + bit]) << bit;
            put8(&bytes, packed);
        }
        for (std::int32_t bonus : character.modifiers.statBonuses)
            put32(&bytes, std::bit_cast<std::uint32_t>(bonus));
        put16(&bytes, std::bit_cast<std::uint16_t>(character.modifiers.ac));
        put16(&bytes, std::bit_cast<std::uint16_t>(character.modifiers.level));
        put16(&bytes, std::bit_cast<std::uint16_t>(character.modifiers.age));
        put32(&bytes, std::bit_cast<std::uint32_t>(character.modifiers.birthYear));
        std::uint8_t potionFlags = 0;
        for (size_t bit = 0; bit < character.modifiers.pureStatPotionUsed.size(); ++bit)
            potionFlags |= std::uint8_t(character.modifiers.pureStatPotionUsed[bit]) << bit;
        put8(&bytes, potionFlags);
        put8(&bytes, character.spellPreferences.quickSpell);
        put8(&bytes, character.spellPreferences.spellbookPage);
        for (std::uint8_t value : character.runtimeBonuses.bytes)
            put8(&bytes, value);
    }
    return bytes;
}

std::array<CoreCharacterFields, 4> decodeCharacters(std::span<const std::uint8_t> bytes) {
    if (bytes.size() > MAX_CHARACTER_PAYLOAD)
        throw std::runtime_error("MM6 CHAR field exceeds size limit");
    Reader reader(bytes);
    if (reader.u8() != CHARACTER_VERSION || reader.u8() != 4)
        throw std::runtime_error("unsupported MM6 CHAR field version or count");
    std::array<CoreCharacterFields, 4> characters;
    for (CoreCharacterFields &character : characters) {
        const size_t nameLength = reader.u8();
        if (nameLength > MAX_NAME_BYTES)
            throw std::runtime_error("MM6 character name exceeds size limit");
        character.name = reader.string(nameLength);
        character.sex = reader.u8();
        character.classId = reader.u8();
        character.faceId = reader.u8();
        character.voiceId = reader.u8();
        character.level = reader.u16();
        character.experience = reader.u64();
        character.health = std::bit_cast<std::int32_t>(reader.u32());
        character.mana = std::bit_cast<std::int32_t>(reader.u32());
        character.skillPoints = std::bit_cast<std::int32_t>(reader.u32());
        for (std::int32_t &stat : character.baseStats)
            stat = std::bit_cast<std::int32_t>(reader.u32());
        for (std::uint16_t &skill : character.skills)
            skill = reader.u16();
        for (size_t offset = 0; offset < character.spells.size(); offset += 8) {
            const std::uint8_t packed = reader.u8();
            if (offset == 96 && (packed & 0xf8) != 0)
                throw std::runtime_error("nonzero MM6 spell padding bits");
            for (size_t bit = 0; bit < 8 && offset + bit < character.spells.size(); ++bit)
                character.spells[offset + bit] = (packed >> bit) & 1;
        }
        for (std::int32_t &bonus : character.modifiers.statBonuses)
            bonus = std::bit_cast<std::int32_t>(reader.u32());
        character.modifiers.ac = std::bit_cast<std::int16_t>(reader.u16());
        character.modifiers.level = std::bit_cast<std::int16_t>(reader.u16());
        character.modifiers.age = std::bit_cast<std::int16_t>(reader.u16());
        character.modifiers.birthYear = std::bit_cast<std::int32_t>(reader.u32());
        const std::uint8_t potionFlags = reader.u8();
        if (potionFlags & 0x80)
            throw std::runtime_error("nonzero MM6 pure-stat potion padding bit");
        for (size_t bit = 0; bit < character.modifiers.pureStatPotionUsed.size(); ++bit)
            character.modifiers.pureStatPotionUsed[bit] = (potionFlags >> bit) & 1;
        character.spellPreferences.quickSpell = reader.u8();
        character.spellPreferences.spellbookPage = reader.u8();
        for (std::uint8_t &value : character.runtimeBonuses.bytes)
            value = reader.u8();
    }
    reader.finish();
    return characters;
}

} // namespace

std::array<SaveSection, 2> encodeCoreSections(const CoreSaveFields &fields) {
    validate(fields);
    return {SaveSection{PART_TAG, true, encodeParty(fields.party)},
            SaveSection{CHAR_TAG, true, encodeCharacters(fields.characters)}};
}

CoreSaveFields decodeCoreSections(const SaveContainer &container) {
    const SaveSection *party = nullptr;
    const SaveSection *characters = nullptr;
    for (const SaveSection &section : container.sections) {
        if (section.tag == PART_TAG)
            party = &section;
        else if (section.tag == CHAR_TAG)
            characters = &section;
    }
    if (!party || !characters || !party->required || !characters->required)
        throw std::runtime_error("MM6 core save fields are missing or optional");

    CoreSaveFields result;
    result.party = decodeParty(party->payload);
    result.characters = decodeCharacters(characters->payload);
    validate(result);
    return result;
}

void loadCoreFieldsBeforeMutation(std::span<const std::uint8_t> bytes, CoreSaveFields *destination) {
    if (!destination)
        throw std::runtime_error("null MM6 core save destination");
    CoreSaveFields staged = decodeCoreSections(decodeSaveContainer(bytes));
    *destination = std::move(staged);
}

} // namespace mm6
