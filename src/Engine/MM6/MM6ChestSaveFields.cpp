#include "MM6ChestSaveFields.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace mm6 {
namespace {

constexpr std::array<char, 4> CHST_TAG = {'C', 'H', 'S', 'T'};
constexpr std::size_t MAX_RAW_DELTA = 8 * 1024 * 1024;

void put32(std::vector<std::uint8_t> *out, std::uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8)
        out->push_back(static_cast<std::uint8_t>(value >> shift));
}

std::uint32_t read32(std::span<const std::uint8_t> bytes, std::size_t *offset) {
    if (bytes.size() - *offset < 4)
        throw std::runtime_error("truncated MM6 CHST integer");
    std::uint32_t value = 0;
    for (int shift = 0; shift < 32; shift += 8)
        value |= std::uint32_t(bytes[(*offset)++]) << shift;
    return value;
}

void validate(const ChestSaveFields &fields) {
    const auto &name = fields.activeDeltaName;
    if (name.size() < 5 || name.size() > 16 ||
        (!name.ends_with(".dlv") && !name.ends_with(".ddm")) ||
        !std::all_of(name.begin(), name.end() - 4, [](char c) {
            return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-';
        }) || fields.rawActiveDelta.empty() || fields.rawActiveDelta.size() > MAX_RAW_DELTA)
        throw std::runtime_error("MM6 CHST lacks a bounded active raw delta");
    for (std::uint8_t flag : fields.generatedArtifactFlags)
        if (flag > 1)
            throw std::runtime_error("MM6 CHST artifact flag is not binary");
}

} // namespace

ChestSaveFields captureChestSaveFields(const ChestTitleState &owner,
                                      std::string activeDeltaName,
                                      std::span<const std::uint8_t> rawActiveDelta,
                                      GameTitle title) {
    if (title != GameTitle::MM6 || !owner.ready)
        throw std::runtime_error("MM6 CHST capture requires a ready MM6 title owner");
    ChestSaveFields fields{owner.lcgState, owner.generatedArtifactFlags,
                           std::move(activeDeltaName),
                           {rawActiveDelta.begin(), rawActiveDelta.end()}};
    validate(fields);
    return fields;
}

SaveSection encodeChestSection(const ChestSaveFields &fields) {
    validate(fields);
    SaveSection section{CHST_TAG, true, {}};
    auto &out = section.payload;
    out.reserve(1 + 1 + fields.activeDeltaName.size() + 4 + 30 + 4 + fields.rawActiveDelta.size());
    out.push_back(1);
    out.push_back(static_cast<std::uint8_t>(fields.activeDeltaName.size()));
    out.insert(out.end(), fields.activeDeltaName.begin(), fields.activeDeltaName.end());
    put32(&out, fields.lcgState);
    out.insert(out.end(), fields.generatedArtifactFlags.begin(), fields.generatedArtifactFlags.end());
    put32(&out, static_cast<std::uint32_t>(fields.rawActiveDelta.size()));
    out.insert(out.end(), fields.rawActiveDelta.begin(), fields.rawActiveDelta.end());
    return section;
}

ChestSaveFields decodeChestSection(const SaveContainer &container) {
    auto it = std::find_if(container.sections.begin(), container.sections.end(),
                           [](const SaveSection &section) { return section.tag == CHST_TAG; });
    if (it == container.sections.end() || !it->required ||
        it->payload.size() > 1 + 1 + 16 + 4 + 30 + 4 + MAX_RAW_DELTA)
        throw std::runtime_error("MM6 save lacks bounded required CHST section");
    const std::span<const std::uint8_t> bytes = it->payload;
    std::size_t offset = 0;
    if (bytes.empty() || bytes[offset++] != 1)
        throw std::runtime_error("unsupported MM6 CHST version");
    if (offset == bytes.size())
        throw std::runtime_error("truncated MM6 CHST name");
    const std::size_t nameSize = bytes[offset++];
    if (nameSize > bytes.size() - offset)
        throw std::runtime_error("truncated MM6 CHST name");
    ChestSaveFields fields;
    fields.activeDeltaName.assign(bytes.begin() + offset, bytes.begin() + offset + nameSize);
    offset += nameSize;
    fields.lcgState = read32(bytes, &offset);
    if (bytes.size() - offset < fields.generatedArtifactFlags.size())
        throw std::runtime_error("truncated MM6 CHST artifact ledger");
    std::copy_n(bytes.begin() + offset, fields.generatedArtifactFlags.size(),
                fields.generatedArtifactFlags.begin());
    offset += fields.generatedArtifactFlags.size();
    const std::uint32_t deltaSize = read32(bytes, &offset);
    if (deltaSize > MAX_RAW_DELTA || deltaSize != bytes.size() - offset)
        throw std::runtime_error("invalid MM6 CHST raw delta length");
    fields.rawActiveDelta.assign(bytes.begin() + offset, bytes.end());
    validate(fields);
    return fields;
}

void restoreChestFieldsBeforeMutation(const ChestSaveFields &fields,
                                      ChestTitleState *owner,
                                      std::vector<std::uint8_t> *rawActiveDelta,
                                      GameTitle title) {
    if (title != GameTitle::MM6 || !owner || !rawActiveDelta)
        throw std::runtime_error("MM6 CHST restore requires detached MM6 owners");
    validate(fields);
    ChestTitleState staged;
    if (!restoreChestTitleState(title, &staged, fields.lcgState, fields.generatedArtifactFlags))
        throw std::runtime_error("MM6 CHST title restore rejected state");
    std::vector<std::uint8_t> stagedDelta = fields.rawActiveDelta;
    rawActiveDelta->swap(stagedDelta);
    *owner = staged;
}

} // namespace mm6
