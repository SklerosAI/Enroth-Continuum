#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <vector>

#include "Engine/GameTitle.h"

#include "MM6SaveContainer.h"

struct Party;

namespace mm6 {

// Raw current-runtime channels in Fire, Air, Water, Earth, Physical, Magic,
// Spirit, Mind, Body, Light, Dark order. MM6 gameplay mapping is still open.
struct ResistanceCharacterFields {
    std::array<std::int16_t, 11> base{};
    std::array<std::int16_t, 11> bonus{};

    bool operator==(const ResistanceCharacterFields &) const = default;
};

struct ResistanceSaveFields {
    std::array<ResistanceCharacterFields, 4> characters{};

    bool operator==(const ResistanceSaveFields &) const = default;
};

/** Required, versioned RSTS section; exact signed runtime values only. */
SaveSection encodeResistanceSection(const ResistanceSaveFields &fields);
ResistanceSaveFields decodeResistanceSection(const SaveContainer &container);
void loadResistanceFieldsBeforeMutation(std::span<const std::uint8_t> bytes,
                                        ResistanceSaveFields *destination);

/** Capture/stage all live resistance fields with MM6 title guards and copy semantics. */
ResistanceSaveFields captureResistanceFields(const Party &party, GameTitle title);
Party stageResistanceParty(const Party &live, std::span<const std::uint8_t> bytes, GameTitle title);

} // namespace mm6
