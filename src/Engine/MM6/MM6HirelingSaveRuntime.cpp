#include "MM6HirelingSaveFields.h"

#include <cstddef>
#include <stdexcept>
#include <utility>

#include "Engine/Party.h"

namespace mm6 {
namespace {

void requireMM6(GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 hireling fields cannot apply to another title");
}

} // namespace

HirelingSaveFields captureHirelingFields(const Party &party, GameTitle title) {
    requireMM6(title);
    HirelingSaveFields fields;
    fields.hirelings = party.pHirelings;
    fields.displayNames = {party.pHireling1Name, party.pHireling2Name};
    for (size_t index = 0; index < fields.sacrifice.size(); ++index) {
        const NPCSacrificeStatus &source = party.pHirelingsSacrifice[index];
        fields.sacrifice[index] = {source.inProgress, source.elapsedTime.ticks(), source.endTime.ticks()};
    }
    (void) encodeHirelingSection(fields);
    return fields;
}

Party stageHirelingParty(const Party &live, std::span<const std::uint8_t> bytes, GameTitle title) {
    requireMM6(title);
    HirelingSaveFields fields;
    loadHirelingFieldsBeforeMutation(bytes, &fields);
    Party staged = live;
    staged.pHirelings = std::move(fields.hirelings);
    staged.pHireling1Name = std::move(fields.displayNames[0]);
    staged.pHireling2Name = std::move(fields.displayNames[1]);
    for (size_t index = 0; index < fields.sacrifice.size(); ++index) {
        const SavedSacrificeStatus &source = fields.sacrifice[index];
        NPCSacrificeStatus &target = staged.pHirelingsSacrifice[index];
        target.inProgress = source.inProgress;
        target.elapsedTime = Duration::fromTicks(source.elapsedTicks);
        target.endTime = Duration::fromTicks(source.endTicks);
    }
    return staged;
}

} // namespace mm6
