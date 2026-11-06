#include "Engine/Spells/MM6TimedCureRules.h"

#include <array>
#include <cctype>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

#include "Library/Json/Json.h"

#include "Utility/Memory/Blob.h"

namespace mm6 {
namespace {

constexpr std::array<int, 6> kSpellIds = {40, 49, 57, 60, 64, 67};
std::optional<std::array<int, 3>> loadedSecondsPerSkill;

Condition conditionForSpell(SpellId spellId) {
    switch (std::to_underlying(spellId)) {
        case 40: return CONDITION_PETRIFIED;
        case 49: return CONDITION_CURSED;
        case 57: return CONDITION_FEAR;
        case 60: return CONDITION_PARALYZED;
        case 64: return CONDITION_INSANE;
        case 67: return CONDITION_WEAK;
        default: throw std::runtime_error("Unsupported MM6 timed cure spell ID");
    }
}

void requireHash(const Json &document, const char *key) {
    const Json &value = document.at(key);
    if (!value.is_string())
        throw std::runtime_error("MM6 timed cure source fingerprint must be a string");
    const std::string hash = value.get<std::string>();
    if (hash.size() != 64)
        throw std::runtime_error("MM6 timed cure source fingerprint has wrong length");
    for (unsigned char character : hash)
        if (!std::isxdigit(character))
            throw std::runtime_error("MM6 timed cure source fingerprint is not hexadecimal");
}

int checkedPositive(const Json &value, int maximum) {
    if (!value.is_number_integer())
        throw std::runtime_error("MM6 timed cure field is not an integer");
    if (value.is_number_unsigned()) {
        const auto number = value.get<std::uint64_t>();
        if (number < 1 || number > static_cast<std::uint64_t>(maximum))
            throw std::runtime_error("MM6 timed cure field is outside bounds");
        return static_cast<int>(number);
    }
    const auto number = value.get<std::int64_t>();
    if (number < 1 || number > maximum)
        throw std::runtime_error("MM6 timed cure field is outside bounds");
    return static_cast<int>(number);
}

}  // namespace

void loadTimedCureRules(const Blob &source) {
    if (source.size() > 1024)
        throw std::runtime_error("MM6 timed cure manifest is too large");
    const Json document = Json::parse(source.str());
    if (!document.is_object() || document.size() != 5 || document.at("format") != 1)
        throw std::runtime_error("Invalid MM6 timed cure manifest header");
    requireHash(document, "spell_report_sha256");
    requireHash(document, "guide_report_sha256");

    const Json &ids = document.at("ids");
    if (!ids.is_array() || ids.size() != kSpellIds.size())
        throw std::runtime_error("MM6 timed cure manifest has wrong ID set");
    for (size_t i = 0; i < kSpellIds.size(); ++i)
        if (checkedPositive(ids[i], 99) != kSpellIds[i])
            throw std::runtime_error("MM6 timed cure manifest has wrong ID order");

    const Json &values = document.at("seconds_per_skill");
    if (!values.is_array() || values.size() != 3)
        throw std::runtime_error("MM6 timed cure requires three rank windows");
    std::array<int, 3> candidate{};
    for (size_t rank = 0; rank < candidate.size(); ++rank)
        candidate[rank] = checkedPositive(values[rank], 604800);
    if (candidate[0] > candidate[1] || candidate[1] > candidate[2])
        throw std::runtime_error("MM6 timed cure window falls with mastery");
    loadedSecondsPerSkill = candidate;
}

bool isTimedCureSpell(SpellId spellId) {
    for (int id : kSpellIds)
        if (std::to_underlying(spellId) == id)
            return true;
    return false;
}

bool applyTimedCure(SpellId spellId, Mastery mastery, int skillLevel,
                    Time now, CharacterConditions &conditions) {
    const Condition condition = conditionForSpell(spellId);
    const int rank = std::to_underlying(mastery);
    if (rank < 1 || rank > 3 || skillLevel < 1 || skillLevel > 63 || !now.isValid())
        throw std::runtime_error("MM6 timed cure has invalid mastery, skill or time");
    if (!loadedSecondsPerSkill)
        throw std::runtime_error("MM6 timed cure rules have not been loaded");
    if (!conditions.has(condition))
        return false;
    const Time onset = conditions.get(condition);
    if (onset > now)
        throw std::runtime_error("MM6 timed cure condition starts in the future");
    const int seconds = (*loadedSecondsPerSkill)[rank - 1] * skillLevel;
    if (now - onset >= Duration::fromSeconds(seconds))
        return false;
    conditions.reset(condition);
    return true;
}

}  // namespace mm6
