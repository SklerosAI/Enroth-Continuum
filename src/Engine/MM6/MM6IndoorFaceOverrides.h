#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace mm6 {

// Detached identity supplied by a title-specific base/runtime adapter. Base
// means the deterministic face state just before OnMapReload, including fixed
// load-time repairs. Texture names are canonical bitmap keys, never pointers.
struct IndoorFaceState {
    std::uint32_t attributes = 0;
    std::string textureName;

    bool operator==(const IndoorFaceState &) const = default;
};

struct IndoorFaceOverride {
    std::uint32_t faceIndex = 0;
    std::optional<std::uint32_t> attributes;
    std::optional<std::string> textureName;

    bool operator==(const IndoorFaceOverride &) const = default;
};

struct IndoorFaceMapOverrides {
    std::string mapName; // Canonical lower-case .blv basename.
    std::uint32_t baseFaceCount = 0;
    std::uint64_t baseFingerprint = 0;
    std::vector<IndoorFaceOverride> faces;

    bool operator==(const IndoorFaceMapOverrides &) const = default;
};

struct IndoorFaceOverrides {
    std::vector<IndoorFaceMapOverrides> maps;

    bool operator==(const IndoorFaceOverrides &) const = default;
};

} // namespace mm6
