#include "MM6V5FirstMapNativeSlotPolicy.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <ranges>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "MM6SessionSave.h"
#include "MM6V4SaveFields.h"
#include "MM6V5FirstMapLoadPreflight.h"
#include "MM6V5FullSessionPreflight.h"
#include "MM6V5LiveSaveCapture.h"
#include "MM6V5SaveFields.h"

namespace mm6 {
namespace {

void requireNativeRoot(const V5FirstMapNativeRoot &root, GameTitle title,
                       const SaveCandidateValidator &validate) {
    if (title != GameTitle::MM6 || !validate)
        throw std::invalid_argument("MM6 native first-map slot needs its title and validator");
    const auto &path = root.requestedRoot;
    if (!path.is_absolute() || path.empty() ||
        path.lexically_normal() != path ||
        path != root.startupResolvedMm6Root ||
        !std::filesystem::is_directory(std::filesystem::symlink_status(path)) ||
        std::filesystem::canonical(path) != path)
        throw std::invalid_argument("MM6 native first-map write needs exact startup profile root");
    const auto saves = path / "saves";
    const auto status = std::filesystem::symlink_status(saves);
    if (status.type() != std::filesystem::file_type::not_found &&
        (!std::filesystem::is_directory(status) ||
         std::filesystem::canonical(saves) != saves))
        throw std::invalid_argument("MM6 native saves path must be a native directory");
}

void requireNativeRootAndSlot(const V5FirstMapNativeRoot &root,
                              std::string_view slotName, GameTitle title,
                              const SaveCandidateValidator &validate) {
    if (!isNumberedV5FirstMapSlot(slotName))
        throw std::invalid_argument("MM6 native first-map write needs numbered title-private slot");
    requireNativeRoot(root, title, validate);
}

/**
 * @param bytes                         Bytes of a numbered slot file.
 * @return                              Header of a v7 save on any map or of a v5 save on the first map. Throws for
 *                                      anything else.
 */
SaveHeaderFields numberedSlotHeader(std::span<const std::uint8_t> bytes) {
    if (decodeSaveContainer(bytes).version == SESSION_SAVE_VERSION)
        return decodeSessionSave(bytes, GameTitle::MM6).raw.metadata.header;
    const auto saved = std::make_unique<V5SaveFields>(decodeV5Save(bytes, GameTitle::MM6));
    if (saved->v4.v3.raw.metadata.header.mapFileName != "oute3.odm")
        throw std::runtime_error("MM6 native slot has another HEAD map");
    return saved->v4.v3.raw.metadata.header;
}

void validateNumberedSlot(std::span<const std::uint8_t> bytes,
                          const SaveCandidateValidator &validate) {
    (void) numberedSlotHeader(bytes);
    validate(bytes);
}

V5FirstMapSlotState snapshot(const std::filesystem::path &root,
                             std::string_view slotName) {
    const auto saved = detail::snapshotSaveSlotForRollback(root, slotName);
    return {saved.primary, saved.backup};
}

void validatePrior(const V5FirstMapSlotState &state,
                   const SaveCandidateValidator &validate) {
    if (!state.primary && state.backup)
        throw std::runtime_error("MM6 native slot needs recovery before overwrite");
    if (state.primary)
        validateNumberedSlot(*state.primary, validate);
    if (state.backup)
        validateNumberedSlot(*state.backup, validate);
}

V5FirstMapSaveOutcome classify(const V5FirstMapSlotState &before,
                               const V5FirstMapSlotState &after,
                               std::span<const std::uint8_t> candidate,
                               bool writerReturned,
                               const SaveCandidateValidator &validate) {
    validatePrior(after, validate);
    const auto expectedBackup = before.primary ? before.primary : before.backup;
    if (after.backup != before.backup && after.backup != expectedBackup)
        return V5FirstMapSaveOutcome::IndeterminateNeedsPreflight;
    const bool candidateIsPrimary = after.primary &&
        std::ranges::equal(*after.primary, candidate);
    if (writerReturned && candidateIsPrimary && after.backup == expectedBackup)
        return V5FirstMapSaveOutcome::Committed;
    if (!writerReturned && after.primary == before.primary)
        return V5FirstMapSaveOutcome::NoNewPrimaryNeedsPreflight;
    return V5FirstMapSaveOutcome::IndeterminateNeedsPreflight;
}

} // namespace

bool isNumberedV5FirstMapSlot(std::string_view slotName) {
    return slotName.size() == 11 && slotName.starts_with("save") && slotName.ends_with(".ec6") &&
           std::ranges::all_of(slotName.substr(4, 3), [](char digit) { return digit >= '0' && digit <= '9'; });
}

V5FirstMapSlotState preflightV5FirstMapNativeWriteSlot(
    const V5FirstMapNativeRoot &root, std::string_view slotName,
    std::span<const std::uint8_t> candidate, GameTitle title,
    const SaveCandidateValidator &validate) {
    requireNativeRootAndSlot(root, slotName, title, validate);
    validateNumberedSlot(candidate, validate);
    auto prior = snapshot(root.requestedRoot, slotName);
    validatePrior(prior, validate);
    return prior;
}

V5FirstMapSaveOutcome writeV5FirstMapNativeCandidateForTest(
    const V5FirstMapNativeRoot &root, std::string_view slotName,
    std::span<const std::uint8_t> candidate, GameTitle title,
    const SaveCandidateValidator &validate, const detail::AtomicSaveFault &fault) {
    return writeNumberedSlotCandidate(root, slotName, candidate, title, validate, fault);
}

V5FirstMapSaveOutcome writeNumberedSlotCandidate(
    const V5FirstMapNativeRoot &root, std::string_view slotName,
    std::span<const std::uint8_t> candidate, GameTitle title,
    const SaveCandidateValidator &validate, const detail::AtomicSaveFault &fault) {
    const auto before = preflightV5FirstMapNativeWriteSlot(
        root, slotName, candidate, title, validate);
    bool writerReturned = false;
    try {
        detail::writeAtomicV5FirstMapNativeSaveFileForTest(
            root.requestedRoot, slotName, candidate, validate, fault);
        writerReturned = true;
    } catch (...) {
        // A failed directory sync can leave a new readable but unconfirmed
        // primary. Never turn this into a successful result.
    }
    try {
        return classify(before, snapshot(root.requestedRoot, slotName),
                        candidate, writerReturned, validate);
    } catch (...) {
        return V5FirstMapSaveOutcome::IndeterminateNeedsPreflight;
    }
}

V5FirstMapSaveAttempt writeV5FirstMapNativeLiveSave(
    const V5FirstMapNativeRoot &root, std::string_view slotName,
    std::span<const std::uint8_t> sourceV4, std::string_view displayName,
    GameTitle title, const SaveCandidateValidator &validate, const detail::AtomicSaveFault &fault) {
    requireNativeRootAndSlot(root, slotName, title, validate);
    const auto source = std::unique_ptr<V4SaveFields>(new V4SaveFields(decodeV4Save(sourceV4, title)));
    source->v3.raw.metadata.header.displayName = displayName;
    auto candidate = captureV5FirstMapLiveSession(
        encodeV4Save(*source, title), V5FirstMapCaptureBoundary::PausedSaveMenu);
    const V5FirstMapSaveOutcome outcome = writeNumberedSlotCandidate(
        root, slotName, candidate, title, validate, fault);
    return {std::move(candidate), outcome};
}

std::vector<SaveSlotInfo> listV5FirstMapNativeSlots(
    const V5FirstMapNativeRoot &root, GameTitle title,
    const SaveCandidateValidator &validate) {
    requireNativeRoot(root, title, validate);
    const auto saves = root.requestedRoot / "saves";
    if (!std::filesystem::exists(std::filesystem::symlink_status(saves)))
        return {};
    std::set<std::string> names;
    for (const auto &entry : std::filesystem::directory_iterator(saves)) {
        std::string name = entry.path().filename().string();
        if (name.ends_with(".bak"))
            name.resize(name.size() - 4);
        if (isNumberedV5FirstMapSlot(name))
            names.insert(std::move(name));
    }
    std::vector<SaveSlotInfo> result;
    for (const std::string &name : names) {
        try {
            const auto bytes = readV5FirstMapNativeLoadSlot(root, name, title, validate);
            result.push_back({name, numberedSlotHeader(bytes), false});
        } catch (const std::exception &) {
            // A slot that cannot be loaded as it is stays out of the menu.
        }
    }
    return result;
}

std::vector<std::uint8_t> readV5FirstMapNativeLoadSlot(
    const V5FirstMapNativeRoot &root, std::string_view slotName,
    GameTitle title, const SaveCandidateValidator &validate) {
    requireNativeRootAndSlot(root, slotName, title, validate);
    auto state = snapshot(root.requestedRoot, slotName);
    validatePrior(state, validate);
    if (!state.primary)
        throw std::runtime_error("MM6 native first-map slot has no primary");
    return std::move(*state.primary);
}

} // namespace mm6
