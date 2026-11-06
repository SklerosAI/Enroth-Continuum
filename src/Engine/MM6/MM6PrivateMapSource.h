#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "MM6KnownSaveFields.h"
#include "MM6MapSaveValidation.h"

#include "Utility/Memory/Blob.h"

namespace mm6 {

// This reader accepts an explicit native directory containing retained MM6 F3
// reports, the ignored private map manifest, and decoded base-map copies. It
// verifies their identity before exposing names or base bytes to a save load.
class PrivateMapSource {
 public:
    explicit PrivateMapSource(const std::filesystem::path &researchRoot);

    std::span<const std::string> expectedNames() const { return _expectedNames; }
    Blob readBase(std::string_view baseName) const;

 private:
    std::filesystem::path _root;
    std::vector<std::string> _expectedNames;
    std::unordered_map<std::string, std::string> _baseHashes;
};

// Offline known-section staging through the verified private native source.
// This does not wire SaveLoad, a native profile slot, or live world mutation.
void loadKnownSaveWithPrivateMapsBeforeMutation(std::span<const std::uint8_t> bytes,
                                                const std::filesystem::path &researchRoot,
                                                const GroundItemVerifier &verifyGroundItem,
                                                KnownSaveFields *destination);

} // namespace mm6
