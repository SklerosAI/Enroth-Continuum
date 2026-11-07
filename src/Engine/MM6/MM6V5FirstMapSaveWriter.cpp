#include "MM6V5FirstMapSaveWriter.h"

#include <memory>
#include <stdexcept>
#include <utility>

#include "MM6V5FirstMapLoadPreflight.h"
#include "MM6V5FullSessionPreflight.h"
#include "MM6V5LiveSaveCapture.h"
#include "MM6V5SaveFields.h"

namespace mm6 {

V5FirstMapSaveOutcome detail::writeV5FirstMapCandidateForTest(
    const std::filesystem::path &profileRoot, std::string_view slotName,
    std::span<const std::uint8_t> candidate, GameTitle title,
    const SaveCandidateValidator &validate, const AtomicSaveFault &fault) {
    if (title != GameTitle::MM6 || !validate)
        throw std::invalid_argument("MM6 first-map writer needs its title and validator");
    const auto decoded = std::make_unique<V5SaveFields>(decodeV5Save(candidate, title));
    if (decoded->v4.v3.raw.metadata.header.mapFileName != "oute3.odm")
        throw std::runtime_error("MM6 first-map writer has another HEAD map");
    validate(candidate);
    // Enforce the title, map, slot and owner-only destination before reading
    // the prior state or attempting any file mutation.
    (void) preflightRecoverableV5FirstMapSaveFileForTest(
        profileRoot, slotName, title, validate);
    const auto oldSlot = snapshotSaveSlotForRollback(profileRoot, slotName);
    if (!oldSlot.primary && oldSlot.backup)
        throw std::runtime_error("MM6 v5 backup-only slot needs read-only preflight");
    const V5FirstMapSlotState before{oldSlot.primary, oldSlot.backup};
    bool writerReturned = false;
    try {
        writeAtomicV5SaveFileForTest(profileRoot, slotName, candidate,
                                     validate, fault);
        writerReturned = true;
    } catch (...) {
        // The primary may already have been renamed. Inspect exact disk state
        // but never infer durability merely because the candidate is readable.
    }
    V5FirstMapSlotState after;
    try {
        const auto observed = snapshotSaveSlotForRollback(profileRoot, slotName);
        after = {observed.primary, observed.backup};
        const V5FirstMapWriteResult assessed = assessV5FirstMapWriteResultForTest(
            title, slotName, candidate, before, after, writerReturned, validate);
        if (assessed == V5FirstMapWriteResult::Committed)
            return V5FirstMapSaveOutcome::Committed;
        if (assessed == V5FirstMapWriteResult::NoNewPrimaryNeedsPreflight)
            return V5FirstMapSaveOutcome::NoNewPrimaryNeedsPreflight;
    } catch (...) {
        // An unreadable or unexpected post-attempt state is also uncertain.
    }
    return V5FirstMapSaveOutcome::IndeterminateNeedsPreflight;
}

V5FirstMapSaveAttempt writeV5FirstMapLiveSaveForTest(
    const std::filesystem::path &profileRoot, std::string_view slotName,
    std::span<const std::uint8_t> sourceV4, GameTitle title,
    V5FirstMapCaptureBoundary boundary) {
    if (title != GameTitle::MM6)
        throw std::invalid_argument("MM6 first-map writer requires MM6 title");
    auto candidate = captureV5FirstMapLiveSession(sourceV4, boundary);
    const SaveCandidateValidator validate = [title](
        std::span<const std::uint8_t> bytes) {
        (void) stageV5FirstMapLoadPreflight(bytes, title);
    };
    const V5FirstMapSaveOutcome outcome = detail::writeV5FirstMapCandidateForTest(
        profileRoot, slotName, candidate, title, validate, {});
    return {std::move(candidate), outcome};
}

} // namespace mm6
