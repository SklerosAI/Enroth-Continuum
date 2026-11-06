#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "Engine/GameTitle.h"

#include "MM6KnownSaveFields.h"
#include "MM6MapSaveValidation.h"

namespace mm6 {

// Read a default .dlv/.ddm entry from the title-specific Games archive. The
// caller must verify the archive and supply the signed 67-name MM6 manifest.
using DefaultDeltaReader = std::function<Blob(std::string_view)>;

constexpr std::string_view NEW_GAME_AUTOSAVE_SLOT = "autosave.ec6";
constexpr std::string_view NEW_GAME_FIRST_MAP = "oute3.odm";

// Build only the currently represented known-section candidate. The seed must
// already contain detached Party, NPC and visual capture; this replaces MAPS
// with all 67 default archive deltas and validates each against its MM6 base.
// It is not a complete save or permission to open live SaveLoad.
std::vector<std::uint8_t> encodeOfflineNewGameAutosave(
    const KnownSaveFields &seed, std::span<const std::string> expectedNames,
    const DefaultDeltaReader &readDefaultDelta, const MapBaseReader &readBase,
    const GroundItemVerifier &verifyItem, GameTitle title);

// Validate the complete known candidate before touching disk, then use the
// native atomic .ec6 writer in an explicit MM6 profile. The live new-game
// route must stay guarded until omitted owners and reload are proved.
std::vector<std::uint8_t> writeOfflineNewGameAutosave(
    const std::filesystem::path &profileRoot, const KnownSaveFields &seed,
    std::span<const std::string> expectedNames, const DefaultDeltaReader &readDefaultDelta,
    const MapBaseReader &readBase, const GroundItemVerifier &verifyItem, GameTitle title);

} // namespace mm6
