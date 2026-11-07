#include "MM6V5FirstMapNativeSlotPolicy.h"

#include <memory>
#include <ranges>
#include <stdexcept>
#include <string>

#include "MM6V5SaveFields.h"

namespace mm6 {
namespace {

void requireNativeRootAndSlot(const V5FirstMapNativeRoot &root,
                              std::string_view slotName, GameTitle title,
                              const SaveCandidateValidator &validate) {
    if (title != GameTitle::MM6 || !validate || slotName.size() != 11 ||
        !slotName.starts_with("save") || !slotName.ends_with(".ec6") ||
        slotName[4] < '0' || slotName[4] > '9' ||
        slotName[5] < '0' || slotName[5] > '9' ||
        slotName[6] < '0' || slotName[6] > '9')
        throw std::invalid_argument("MM6 native first-map write needs numbered title-private slot");
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

void validateFirstMapV5(std::span<const std::uint8_t> bytes,
                        const SaveCandidateValidator &validate) {
    const auto saved = std::make_unique<V5SaveFields>(
        decodeV5Save(bytes, GameTitle::MM6));
    if (saved->v4.v3.raw.metadata.header.mapFileName != "oute3.odm")
        throw std::runtime_error("MM6 native slot has another HEAD map");
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
        validateFirstMapV5(*state.primary, validate);
    if (state.backup)
        validateFirstMapV5(*state.backup, validate);
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

V5FirstMapSlotState preflightV5FirstMapNativeWriteSlot(
    const V5FirstMapNativeRoot &root, std::string_view slotName,
    std::span<const std::uint8_t> candidate, GameTitle title,
    const SaveCandidateValidator &validate) {
    requireNativeRootAndSlot(root, slotName, title, validate);
    validateFirstMapV5(candidate, validate);
    auto prior = snapshot(root.requestedRoot, slotName);
    validatePrior(prior, validate);
    return prior;
}

V5FirstMapSaveOutcome writeV5FirstMapNativeCandidateForTest(
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

} // namespace mm6
