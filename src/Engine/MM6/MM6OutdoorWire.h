#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace mm6 {

struct WireSpan {
    size_t offset = 0;
    size_t size = 0;
};

struct OutdoorBaseWire {
    struct ModelFaces {
        WireSpan faceRecords; // 308 bytes per face, in model-local zero-based order.
        WireSpan textureNames; // 10 bytes per face in the same order.
        std::uint32_t count = 0;
    };

    std::vector<std::uint8_t> bytes;
    std::vector<ModelFaces> modelFaces;
    std::uint32_t modelCount = 0;
    std::uint32_t decorationCount = 0;
    std::uint32_t decorationPidCount = 0;
    std::uint32_t spawnCount = 0;

    /** Checked zero-based model/facet lookup for MM6 outdoor event adapters. */
    std::span<const std::uint8_t> faceBytes(size_t modelIndex, size_t facetIndex) const;
    std::span<std::uint8_t> faceBytes(size_t modelIndex, size_t facetIndex);
    std::span<const std::uint8_t> textureNameBytes(size_t modelIndex, size_t facetIndex) const;
    std::span<std::uint8_t> textureNameBytes(size_t modelIndex, size_t facetIndex);
};

enum class OutdoorDeltaPart : size_t {
    ResetHeader,
    Visibility,
    Actors,
    GroundItems,
    Chests,
    PersistentBytes,
    Tail,
    Count,
};

struct OutdoorDeltaWire {
    std::vector<std::uint8_t> bytes;
    std::array<WireSpan, static_cast<size_t>(OutdoorDeltaPart::Count)> parts{};
    std::uint32_t actorCount = 0;
    std::uint32_t groundItemCount = 0;
    std::uint32_t chestCount = 0;

    std::span<const std::uint8_t> part(OutdoorDeltaPart part) const;
    std::span<std::uint8_t> part(OutdoorDeltaPart part);
};

/** Parse a decoded MM6 `.odm` payload with exact-end and count bounds. */
OutdoorBaseWire decodeOutdoorBase(std::span<const std::uint8_t> bytes);

/** Parse a decoded MM6 `.ddm` payload without assigning unknown field meanings. */
OutdoorDeltaWire decodeOutdoorDelta(std::span<const std::uint8_t> bytes);

/** Revalidate and return the exact source bytes, including opaque fields. */
std::vector<std::uint8_t> encodeOutdoorBase(const OutdoorBaseWire &wire);
std::vector<std::uint8_t> encodeOutdoorDelta(const OutdoorDeltaWire &wire);

} // namespace mm6
