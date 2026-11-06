#pragma once

#include <cstdint>

#include "Engine/MM6/MM6CreationRules.h"

// MM6 EVT operands are one-byte IDs. Their ranges differ from MM7's EvtVariable
// enum even where the numeric values happen to collide.
enum class MM6VariableFamily : std::uint8_t {
    Invalid,
    SharedCandidate,
    Resistance,
    Skill,
    Condition,
    MapByte,
    Autonote,
    OtherTitleSpecific,
};

enum class MM6VariableScope : std::uint8_t {
    Unknown,
    Character,
    Map,
    Party,
};

struct MM6VariableDescriptor {
    MM6VariableFamily family;
    MM6VariableScope scope;
    int index; // Zero-based within the family; -1 when unresolved.

    // Shared candidates retain the inherited path provisionally. All known
    // title-specific ranges require a real MM6 executor before side effects.
    constexpr bool canUseInheritedExecutor() const {
        return family == MM6VariableFamily::SharedCandidate;
    }
};

constexpr MM6VariableDescriptor describeMM6Variable(int rawId) {
    if (rawId >= 1 && rawId <= 45)
        return {MM6VariableFamily::SharedCandidate, MM6VariableScope::Unknown, rawId};
    if (rawId >= 46 && rawId <= 55)
        return {MM6VariableFamily::Resistance, MM6VariableScope::Character, rawId - 46};
    if (rawId >= 56 && rawId <= 86)
        return {MM6VariableFamily::Skill, MM6VariableScope::Character, rawId - 56};
    if (rawId >= 87 && rawId <= 104)
        return {MM6VariableFamily::Condition, MM6VariableScope::Character, rawId - 87};
    if (rawId >= 105 && rawId <= 204)
        return {MM6VariableFamily::MapByte, MM6VariableScope::Map, rawId - 105};
    if (rawId == 205)
        return {MM6VariableFamily::Autonote, MM6VariableScope::Party, 0};
    if (rawId >= 205 && rawId <= 226)
        return {MM6VariableFamily::OtherTitleSpecific, MM6VariableScope::Unknown, -1};
    return {MM6VariableFamily::Invalid, MM6VariableScope::Unknown, -1};
}

// Shared-looking IDs still need MM6 value ranges before inherited execution.
constexpr bool isValidMM6SharedCandidateValue(int rawId, int value) {
    if (rawId == 2)  // Class: eighteen MM6 classes, not the MM7 class range.
        return mm6::isValidClass(value);
    if (rawId == 12)  // Award: the retained MM6 table has numbered rows 1..100.
        return value >= 1 && value <= 100;
    if (rawId == 17)  // Item in hand: retained MM6 item IDs are 1..580.
        return value >= 1 && value <= 580;
    if (rawId == 19)  // Day of year: the MM6 calendar has twelve 28-day months.
        return value >= 1 && value <= 336;
    return true;
}
