#include "MM6SaveContainer.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <set>
#include <stdexcept>
#include <vector>
#include <utility>

namespace mm6 {
namespace {

constexpr std::array<std::uint8_t, 8> MAGIC = {'E', 'C', '6', 'S', 'A', 'V', 'E', 0};
constexpr std::uint16_t MIN_VERSION = 1;
constexpr std::uint16_t MAX_VERSION = 7;
constexpr size_t HEADER_SIZE = 24;
constexpr size_t SECTION_HEADER_SIZE = 16;
constexpr size_t MAX_FILE_SIZE = 64 * 1024 * 1024;
constexpr size_t MAX_SECTION_SIZE = 32 * 1024 * 1024;
constexpr size_t MAX_SECTIONS = 32;

constexpr std::array<std::array<char, 4>, 38> KNOWN_TAGS = {{
    {'H', 'E', 'A', 'D'}, {'P', 'A', 'R', 'T'}, {'T', 'I', 'M', 'E'}, {'C', 'H', 'A', 'R'},
    {'E', 'F', 'F', 'T'}, {'I', 'T', 'E', 'M'}, {'R', 'E', 'S', 'C'}, {'Q', 'E', 'S', 'T'},
    {'M', 'A', 'P', 'S'}, {'V', 'I', 'S', 'U'}, {'U', 'I', 'S', 'T'}, {'M', 'E', 'T', 'A'},
    {'R', 'S', 'T', 'S'}, {'H', 'E', 'L', 'D'}, {'B', 'U', 'F', 'F'}, {'H', 'I', 'R', 'E'},
    {'R', 'C', 'V', 'R'}, {'C', 'H', 'S', 'T'}, {'S', 'C', 'L', 'K'},
    {'S', 'H', 'O', 'P'}, {'B', 'E', 'A', 'C'}, {'I', 'N', 'V', 'R'},
    {'G', 'N', 'P', 'C'}, {'R', 'N', 'G', 'S'}, {'S', 'P', 'A', 'W'},
    {'A', 'C', 'T', 'R'}, {'E', 'V', 'R', 'T'}, {'S', 'P', 'R', 'T'},
    {'A', 'R', 'T', 'R'},
    {'D', 'E', 'C', 'R'}, {'I', 'A', 'C', 'R'}, {'I', 'S', 'C', 'R'},
    {'I', 'D', 'C', 'R'}, {'I', 'D', 'R', 'T'}, {'I', 'S', 'R', 'T'},
    {'I', 'A', 'R', 'T'}, {'V', 'I', 'S', 'T'}, {'P', 'R', 'K', 'M'},
}};

constexpr std::array<std::uint32_t, 256> makeCrcTable() {
    std::array<std::uint32_t, 256> result{};
    for (std::uint32_t i = 0; i < result.size(); ++i) {
        std::uint32_t value = i;
        for (int bit = 0; bit < 8; ++bit)
            value = (value >> 1) ^ ((value & 1) ? 0xEDB88320u : 0u);
        result[i] = value;
    }
    return result;
}

constexpr auto CRC_TABLE = makeCrcTable();

std::uint32_t checksum(std::span<const std::uint8_t> bytes) {
    std::uint32_t result = 0xFFFFFFFFu;
    for (std::uint8_t byte : bytes)
        result = CRC_TABLE[(result ^ byte) & 0xFFu] ^ (result >> 8);
    return result ^ 0xFFFFFFFFu;
}

void requireBytes(std::span<const std::uint8_t> bytes, size_t offset, size_t count) {
    if (offset > bytes.size() || count > bytes.size() - offset)
        throw std::runtime_error("truncated MM6 save container");
}

std::uint16_t read16(std::span<const std::uint8_t> bytes, size_t offset) {
    requireBytes(bytes, offset, 2);
    return std::uint16_t(bytes[offset]) | (std::uint16_t(bytes[offset + 1]) << 8);
}

std::uint32_t read32(std::span<const std::uint8_t> bytes, size_t offset) {
    requireBytes(bytes, offset, 4);
    return std::uint32_t(bytes[offset]) | (std::uint32_t(bytes[offset + 1]) << 8) |
           (std::uint32_t(bytes[offset + 2]) << 16) | (std::uint32_t(bytes[offset + 3]) << 24);
}

void write16(std::vector<std::uint8_t> *bytes, std::uint16_t value) {
    bytes->push_back(value & 0xFFu);
    bytes->push_back((value >> 8) & 0xFFu);
}

void write32(std::vector<std::uint8_t> *bytes, std::uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8)
        bytes->push_back((value >> shift) & 0xFFu);
}

void overwrite32(std::vector<std::uint8_t> *bytes, size_t offset, std::uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8)
        (*bytes)[offset + shift / 8] = (value >> shift) & 0xFFu;
}

bool validTag(const std::array<char, 4> &tag) {
    return std::all_of(tag.begin(), tag.end(), [](char c) {
        return (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
    });
}

bool knownTag(const std::array<char, 4> &tag) {
    return std::find(KNOWN_TAGS.begin(), KNOWN_TAGS.end(), tag) != KNOWN_TAGS.end();
}

} // namespace

std::vector<std::uint8_t> encodeSaveContainer(const SaveContainer &container) {
    if (container.version < MIN_VERSION || container.version > MAX_VERSION ||
        container.sections.empty() || container.sections.size() > MAX_SECTIONS)
        throw std::runtime_error("MM6 save container has invalid section count");

    size_t totalSize = HEADER_SIZE;
    std::set<std::array<char, 4>> tags;
    for (const SaveSection &section : container.sections) {
        if (!validTag(section.tag) || !tags.insert(section.tag).second)
            throw std::runtime_error("MM6 save container has invalid or duplicate section tag");
        if (section.payload.size() > MAX_SECTION_SIZE ||
            SECTION_HEADER_SIZE + section.payload.size() > MAX_FILE_SIZE - totalSize)
            throw std::runtime_error("MM6 save container exceeds size limit");
        totalSize += SECTION_HEADER_SIZE + section.payload.size();
    }

    std::vector<std::uint8_t> bytes;
    bytes.reserve(totalSize);
    bytes.insert(bytes.end(), MAGIC.begin(), MAGIC.end());
    write16(&bytes, container.version);
    write16(&bytes, 0); // Container flags are reserved.
    write16(&bytes, container.sections.size());
    write16(&bytes, 0); // Reserved.
    write32(&bytes, totalSize);
    write32(&bytes, 0); // Body checksum, filled below.

    for (const SaveSection &section : container.sections) {
        bytes.insert(bytes.end(), section.tag.begin(), section.tag.end());
        write16(&bytes, section.required ? 1 : 0);
        write16(&bytes, 0); // Reserved.
        write32(&bytes, section.payload.size());
        write32(&bytes, checksum(section.payload));
        bytes.insert(bytes.end(), section.payload.begin(), section.payload.end());
    }

    overwrite32(&bytes, 20, checksum(std::span(bytes).subspan(HEADER_SIZE)));
    return bytes;
}

SaveContainer decodeSaveContainer(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < HEADER_SIZE || bytes.size() > MAX_FILE_SIZE)
        throw std::runtime_error("MM6 save container has invalid size");
    if (!std::equal(MAGIC.begin(), MAGIC.end(), bytes.begin()))
        throw std::runtime_error("wrong save title or container signature");
    const std::uint16_t version = read16(bytes, 8);
    if (version < MIN_VERSION || version > MAX_VERSION)
        throw std::runtime_error("unsupported MM6 save version");
    if (read16(bytes, 10) != 0 || read16(bytes, 14) != 0)
        throw std::runtime_error("MM6 save container has nonzero reserved header fields");

    const size_t sectionCount = read16(bytes, 12);
    if (sectionCount == 0 || sectionCount > MAX_SECTIONS || read32(bytes, 16) != bytes.size())
        throw std::runtime_error("MM6 save container has invalid section count or length");
    if (read32(bytes, 20) != checksum(bytes.subspan(HEADER_SIZE)))
        throw std::runtime_error("MM6 save container checksum mismatch");

    SaveContainer result;
    result.version = version;
    result.sections.reserve(sectionCount);
    std::set<std::array<char, 4>> tags;
    size_t offset = HEADER_SIZE;
    for (size_t i = 0; i < sectionCount; ++i) {
        requireBytes(bytes, offset, SECTION_HEADER_SIZE);
        SaveSection section;
        for (size_t j = 0; j < section.tag.size(); ++j)
            section.tag[j] = static_cast<char>(bytes[offset + j]);
        if (!validTag(section.tag) || !tags.insert(section.tag).second)
            throw std::runtime_error("MM6 save container has invalid or duplicate section tag");
        const std::uint16_t flags = read16(bytes, offset + 4);
        if ((flags & ~std::uint16_t(1)) != 0 || read16(bytes, offset + 6) != 0)
            throw std::runtime_error("MM6 save container has invalid section flags");
        section.required = (flags & 1) != 0;
        if (section.required && !knownTag(section.tag))
            throw std::runtime_error("MM6 save container has unknown required section");
        const size_t length = read32(bytes, offset + 8);
        if (length > MAX_SECTION_SIZE)
            throw std::runtime_error("MM6 save section exceeds size limit");
        const std::uint32_t expectedChecksum = read32(bytes, offset + 12);
        offset += SECTION_HEADER_SIZE;
        requireBytes(bytes, offset, length);
        std::span<const std::uint8_t> payload = bytes.subspan(offset, length);
        if (expectedChecksum != checksum(payload))
            throw std::runtime_error("MM6 save section checksum mismatch");
        section.payload.assign(payload.begin(), payload.end());
        result.sections.push_back(std::move(section));
        offset += length;
    }
    if (offset != bytes.size())
        throw std::runtime_error("MM6 save container has trailing bytes");
    return result;
}

} // namespace mm6
