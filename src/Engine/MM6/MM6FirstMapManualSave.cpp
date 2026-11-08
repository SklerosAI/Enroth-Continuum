#include "MM6FirstMapManualSave.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "Engine/Engine.h"
#include "Engine/Party.h"

#include "Library/Logger/Logger.h"

#include "MM6V4SaveFields.h"
#include "MM6V5FirstMapLoadAttempt.h"
#include "MM6V5FirstMapLoadPreflight.h"
#include "MM6V5FullSessionPreflight.h"
#include "MM6V5FirstMapNativeSlotPolicy.h"

namespace mm6 {

static std::optional<std::filesystem::path> startupRoot;
static std::optional<std::filesystem::path> testRoot;
static std::optional<std::vector<std::uint8_t>> armedSourceV4;
static detail::AtomicSaveFault testFault;

static const std::optional<std::filesystem::path> &profileRoot() {
    return testRoot ? testRoot : startupRoot;
}

static SaveCandidateValidator firstMapValidator() {
    return [](std::span<const std::uint8_t> bytes) {
        (void) stageV5FirstMapLoadPreflight(bytes, GameTitle::MM6);
    };
}

static std::string firstFreeSlot(const std::filesystem::path &root) {
    for (int i = 0; i < 1000; i++) {
        std::string name = fmt::format("save{:03}.ec6", i);
        const auto primary = std::filesystem::symlink_status(root / "saves" / name);
        const auto backup = std::filesystem::symlink_status(root / "saves" / (name + ".bak"));
        if (!std::filesystem::exists(primary) && !std::filesystem::exists(backup))
            return name;
    }
    throw std::runtime_error("MM6 profile has no free numbered save slot");
}

void setStartupNativeProfileRoot(bool ramFsUserData, const std::filesystem::path &userPath) {
    if (ramFsUserData) {
        startupRoot.reset();
    } else {
        startupRoot = std::filesystem::weakly_canonical(std::filesystem::absolute(userPath));
    }
}

void armFirstMapManualSave(std::span<const std::uint8_t> sourceV4, GameTitle title) {
    const auto source = std::unique_ptr<V4SaveFields>(new V4SaveFields(decodeV4Save(sourceV4, title)));
    if (title != GameTitle::MM6 || source->v3.raw.metadata.header.mapFileName != "oute3.odm" ||
        source->eventRuntime.maps.size() != 1 || source->eventRuntime.maps[0].mapName != "oute3.odm")
        throw std::runtime_error("MM6 manual save anchor is not a first-map v4 source");
    armedSourceV4.emplace(sourceV4.begin(), sourceV4.end());
}

void disarmFirstMapManualSave() {
    armedSourceV4.reset();
}

bool isFirstMapManualSaveArmed() {
    return armedSourceV4.has_value();
}

FirstMapManualSaveResult saveFirstMapManualSlot(std::string_view fileName, std::string_view displayName) {
    FirstMapManualSaveResult result;
    try {
        const auto &root = profileRoot();
        if (!engine || engine->gameTitle() != GameTitle::MM6 || !armedSourceV4 || !root)
            throw std::runtime_error("MM6 manual save has no armed first-map session or profile root");
        if (!pParty || pParty->pPickedItem.itemId != ITEM_NULL)
            throw std::runtime_error("MM6 manual save refuses an item held on the cursor"); // The first-map load admits only an empty cursor.
        result.fileName = fileName.empty() ? firstFreeSlot(*root) : std::string(fileName);
        const V5FirstMapSaveAttempt attempt = writeV5FirstMapNativeLiveSave(
            {*root, *root}, result.fileName, *armedSourceV4, displayName, GameTitle::MM6, testFault);
        switch (attempt.outcome) {
        case V5FirstMapSaveOutcome::Committed:
            result.outcome = MANUAL_SAVE_COMMITTED;
            break;
        case V5FirstMapSaveOutcome::NoNewPrimaryNeedsPreflight:
            result.outcome = MANUAL_SAVE_NOT_WRITTEN;
            break;
        case V5FirstMapSaveOutcome::IndeterminateNeedsPreflight:
            result.outcome = MANUAL_SAVE_UNCONFIRMED;
            break;
        }
    } catch (const std::exception &e) {
        MM_WARNING("MM6 manual save refused before writing: {}", e.what());
        result.outcome = MANUAL_SAVE_REFUSED;
    }
    return result;
}

std::vector<SaveSlotInfo> listFirstMapManualSlots() {
    const auto &root = profileRoot();
    if (!root)
        return {};
    try {
        return listV5FirstMapNativeSlots({*root, *root}, GameTitle::MM6, firstMapValidator());
    } catch (const std::exception &e) {
        MM_WARNING("MM6 save list refused: {}", e.what());
        return {};
    }
}

std::vector<std::uint8_t> readFirstMapManualSlot(std::string_view fileName) {
    const auto &root = profileRoot();
    if (!root)
        throw std::runtime_error("MM6 save load has no profile root");
    return readV5FirstMapNativeLoadSlot({*root, *root}, fileName, GameTitle::MM6, firstMapValidator());
}

bool mayLoadFirstMapManualSlot(std::string_view fileName) {
    return profileRoot() && isNumberedV5FirstMapSlot(fileName);
}

void loadFirstMapManualSlot(std::string_view fileName) {
    if (!engine || engine->gameTitle() != GameTitle::MM6 || engine->_mm6FirstMapLoadAttempt)
        throw std::runtime_error("MM6 slot load needs a fresh MM6 session");
    try {
        engine->_mm6FirstMapLoadAttempt = V5FirstMapLoadAttempt::fromNativeSlot(readFirstMapManualSlot(fileName), fileName, GameTitle::MM6);
        engine->_mm6FirstMapLoadAttempt->publishOwners(GameTitle::MM6);
    } catch (const std::exception &e) {
        MM_ERROR("MM6 slot '{}' was not loaded: {}", fileName, e.what());
        throw;
    }
    engine->_lastLoadedSaveFileName = fileName;
}

void testing::setNativeProfileRootForTest(std::optional<std::filesystem::path> root) {
    if (root) {
        const auto tempRoot = std::filesystem::canonical(std::filesystem::temp_directory_path());
        if (!root->is_absolute() || std::filesystem::canonical(*root) != *root ||
            root->parent_path() != tempRoot || !root->filename().string().starts_with("mm6-restart-") ||
            (std::filesystem::status(*root).permissions() &
             (std::filesystem::perms::group_all | std::filesystem::perms::others_all)) != std::filesystem::perms::none)
            throw std::invalid_argument("MM6 test profile root must be an owner-only mm6-restart-* temp directory");
    }
    testRoot = std::move(root);
}

void testing::setManualSaveFaultForTest(::mm6::detail::AtomicSaveFault fault) {
    testFault = std::move(fault);
}

} // namespace mm6
