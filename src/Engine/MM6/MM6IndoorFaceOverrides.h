#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "Engine/GameTitle.h"
#include "MM6SaveContainer.h"

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

using IndoorBaseFaceReader = std::function<std::vector<IndoorFaceState>(std::string_view)>;

// Returns no map entry when every face still matches the pre-event base. The caller
// must supply stable texture identity, including for animated faces; an adapter
// must reject a face whose identity it cannot establish.
std::optional<IndoorFaceMapOverrides> captureIndoorFaceOverrides(
    std::string_view mapName, std::span<const IndoorFaceState> base,
    std::span<const IndoorFaceState> current, GameTitle title);

// IFOV is an optional, versioned EC6 section. It is deliberately outside the
// required KnownSaveFields manifest and is not wired to live SaveLoad.
SaveSection encodeIndoorFaceOverrideSection(const IndoorFaceOverrides &fields);
IndoorFaceOverrides decodeIndoorFaceOverrideSection(const SaveContainer &container);

// Verify every map against its separately supplied BLV base before returning
// any candidate. The wrapper decodes the whole EC6 container and only assigns
// destination after all sections/maps pass. Neither mutates the live engine.
IndoorFaceOverrides stageIndoorFaceOverrides(const IndoorFaceOverrides &fields,
                                             const IndoorBaseFaceReader &readBase,
                                             GameTitle title);
void loadIndoorFaceOverridesBeforeMutation(std::span<const std::uint8_t> bytes,
                                           const IndoorBaseFaceReader &readBase,
                                           IndoorFaceOverrides *destination,
                                           GameTitle title);

// Fork precedence: call only after BLV/DLV reconstruction and OnMapReload.
// Changed fields in the saved override win; untouched fields keep script state.
// The detached vector is replaced only after all indices and names validate.
void applyIndoorFaceOverridesAfterMapReload(const IndoorFaceOverrides &staged,
                                            std::string_view mapName,
                                            std::vector<IndoorFaceState> *faces,
                                            GameTitle title);

} // namespace mm6
