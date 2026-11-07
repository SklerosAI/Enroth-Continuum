#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace mm6 {

/**
 * A section of the new `.ec6` container. Payload codecs are separate and must be
 * locked against the MM6 runtime field manifest before this can store game state.
 */
struct SaveSection {
    std::array<char, 4> tag;
    bool required = true;
    std::vector<std::uint8_t> payload;

    bool operator==(const SaveSection &) const = default;
};

struct SaveContainer {
    std::vector<SaveSection> sections;
    std::uint16_t version = 1;

    bool operator==(const SaveContainer &) const = default;
};

/** Serialize a bounded, title-tagged MM6 container. Does not write a save file. */
std::vector<std::uint8_t> encodeSaveContainer(const SaveContainer &container);

/**
 * Parse and validate every header, section and checksum before returning any state.
 * Unknown required sections and future versions are rejected; unknown optional
 * sections are retained as opaque bytes for round-trip. Throws on invalid input.
 */
SaveContainer decodeSaveContainer(std::span<const std::uint8_t> bytes);

} // namespace mm6
