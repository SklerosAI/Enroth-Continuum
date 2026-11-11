#include "MM6FirstMapManualSave.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "Engine/Engine.h"
#include "Engine/Party.h"

#include "Library/Logger/Logger.h"

#include "MM6SessionSave.h"
#include "MM6V4SaveFields.h"
#include "MM6V5FirstMapLoadAttempt.h"
#include "MM6V5FirstMapLoadPreflight.h"
#include "MM6V5FullSessionPreflight.h"
#include "MM6V5FirstMapNativeSlotPolicy.h"

namespace mm6 {

static std::optional<std::filesystem::path> startupRoot;
static std::optional<std::filesystem::path> testRoot;
static std::optional<std::vector<std::uint8_t>> armedSourceV4;
static std::unique_ptr<RawSaveFields> sessionTemplate;
static detail::AtomicSaveFault testFault;

static const std::optional<std::filesystem::path> &profileRoot() {
    return testRoot ? testRoot : startupRoot;
}

static bool isSessionSave(std::span<const std::uint8_t> bytes) {
    return decodeSaveContainer(bytes).version == SESSION_SAVE_VERSION;
}

static SaveCandidateValidator numberedSlotValidator() {
    return [](std::span<const std::uint8_t> bytes) {
        if (isSessionSave(bytes)) {
            (void) stageSessionSave(bytes, GameTitle::MM6);
        } else {
            (void) stageV5FirstMapLoadPreflight(bytes, GameTitle::MM6);
        }
    };
}

static ManualSaveOutcome manualSaveOutcome(V5FirstMapSaveOutcome outcome) {
    switch (outcome) {
    case V5FirstMapSaveOutcome::Committed:
        return MANUAL_SAVE_COMMITTED;
    case V5FirstMapSaveOutcome::NoNewPrimaryNeedsPreflight:
        return MANUAL_SAVE_NOT_WRITTEN;
    case V5FirstMapSaveOutcome::IndeterminateNeedsPreflight:
        return MANUAL_SAVE_UNCONFIRMED;
    }
    return MANUAL_SAVE_UNCONFIRMED;
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
    sessionTemplate = std::make_unique<RawSaveFields>(source->v3.raw);
}

void leaveFirstMapManualSave() {
    armedSourceV4.reset();
}

void disarmFirstMapManualSave() {
    armedSourceV4.reset();
    sessionTemplate.reset();
}

bool isFirstMapManualSaveArmed() {
    return armedSourceV4.has_value();
}

bool isManualSaveArmed() {
    return sessionTemplate != nullptr;
}

FirstMapManualSaveResult saveFirstMapManualSlot(std::string_view fileName, std::string_view displayName) {
    FirstMapManualSaveResult result;
    try {
        const auto &root = profileRoot();
        if (!engine || engine->gameTitle() != GameTitle::MM6 || !sessionTemplate || !root)
            throw std::runtime_error("MM6 manual save has no armed session or profile root");
        if (!pParty || pParty->pPickedItem.itemId != ITEM_NULL)
            throw std::runtime_error("MM6 manual save refuses an item held on the cursor"); // A load admits only an empty cursor.
        result.fileName = fileName.empty() ? firstFreeSlot(*root) : std::string(fileName);
        if (armedSourceV4) {
            result.outcome = manualSaveOutcome(writeV5FirstMapNativeLiveSave({*root, *root}, result.fileName, *armedSourceV4, displayName,
                                                                             GameTitle::MM6, numberedSlotValidator(), testFault).outcome);
        } else {
            const std::vector<std::uint8_t> candidate = captureSessionSave(*sessionTemplate, displayName, GameTitle::MM6);
            result.outcome = manualSaveOutcome(writeNumberedSlotCandidate({*root, *root}, result.fileName, candidate, GameTitle::MM6,
                                                                          numberedSlotValidator(), testFault));
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
        return listV5FirstMapNativeSlots({*root, *root}, GameTitle::MM6, numberedSlotValidator());
    } catch (const std::exception &e) {
        MM_WARNING("MM6 save list refused: {}", e.what());
        return {};
    }
}

std::vector<std::uint8_t> readFirstMapManualSlot(std::string_view fileName) {
    const auto &root = profileRoot();
    if (!root)
        throw std::runtime_error("MM6 save load has no profile root");
    return readV5FirstMapNativeLoadSlot({*root, *root}, fileName, GameTitle::MM6, numberedSlotValidator());
}

bool mayLoadFirstMapManualSlot(std::string_view fileName) {
    return profileRoot() && isNumberedV5FirstMapSlot(fileName);
}

void loadFirstMapManualSlot(std::string_view fileName) {
    if (!engine || engine->gameTitle() != GameTitle::MM6 || engine->_mm6FirstMapLoadAttempt)
        throw std::runtime_error("MM6 slot load needs a fresh MM6 session");
    try {
        const std::vector<std::uint8_t> bytes = readFirstMapManualSlot(fileName);
        if (isSessionSave(bytes)) {
            const std::unique_ptr<StagedSessionSave> staged = stageSessionSave(bytes, GameTitle::MM6);
            auto savedTemplate = std::make_unique<RawSaveFields>(staged->represented.raw.saved);
            publishSessionSave(staged.get(), GameTitle::MM6);
            sessionTemplate = std::move(savedTemplate);
        } else {
            engine->_mm6FirstMapLoadAttempt = V5FirstMapLoadAttempt::fromNativeSlot(bytes, fileName, GameTitle::MM6);
            engine->_mm6FirstMapLoadAttempt->publishOwners(GameTitle::MM6);
        }
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
