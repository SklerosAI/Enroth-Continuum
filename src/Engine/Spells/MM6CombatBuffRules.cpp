#include "Engine/Spells/MM6CombatBuffRules.h"

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

struct CombatBuffRule {
    int baseSeconds = 0;
    std::array<int, 3> perSkillSeconds{};
    int heroismBase = 0;
    int heroismPerSkill = 0;
};

std::optional<CombatBuffRule> loadedRule;

void requireHash(const Json &document, const char *key) {
    const Json &value = document.at(key);
    if (!value.is_string())
        throw std::runtime_error("MM6 combat buff source fingerprint must be a string");
    const std::string hash = value.get<std::string>();
    if (hash.size() != 64)
        throw std::runtime_error("MM6 combat buff source fingerprint has wrong length");
    for (unsigned char character : hash)
        if (!std::isxdigit(character))
            throw std::runtime_error("MM6 combat buff source fingerprint is not hexadecimal");
}

int checkedNumber(const Json &value, int minimum, int maximum) {
    if (!value.is_number_integer())
        throw std::runtime_error("MM6 combat buff field is not an integer");
    if (value.is_number_unsigned()) {
        const auto number = value.get<std::uint64_t>();
        if (number < static_cast<std::uint64_t>(minimum) || number > static_cast<std::uint64_t>(maximum))
            throw std::runtime_error("MM6 combat buff field exceeds bounds");
        return static_cast<int>(number);
    }
    const auto number = value.get<std::int64_t>();
    if (number < minimum || number > maximum)
        throw std::runtime_error("MM6 combat buff field exceeds bounds");
    return static_cast<int>(number);
}

}  // namespace

void loadCombatBuffRules(const Blob &source) {
    if (source.size() > 1024)
        throw std::runtime_error("MM6 combat buff manifest is too large");
    const Json document = Json::parse(source.str());
    if (!document.is_object() || document.size() != 7 || document.at("format") != 1)
        throw std::runtime_error("Invalid MM6 combat buff manifest header");
    requireHash(document, "spell_report_sha256");
    requireHash(document, "guide_report_sha256");
    const Json &ids = document.at("ids");
    if (!ids.is_array() || ids.size() != 2 || checkedNumber(ids[0], 1, 99) != 17 ||
        checkedNumber(ids[1], 1, 99) != 51)
        throw std::runtime_error("MM6 combat buff manifest has wrong ID set");

    CombatBuffRule candidate;
    candidate.baseSeconds = checkedNumber(document.at("base_seconds"), 1, 86400);
    const Json &values = document.at("per_skill_seconds");
    if (!values.is_array() || values.size() != 3)
        throw std::runtime_error("MM6 combat buff requires three rank durations");
    for (size_t rank = 0; rank < candidate.perSkillSeconds.size(); ++rank)
        candidate.perSkillSeconds[rank] = checkedNumber(values[rank], 0, 86400);
    if (candidate.perSkillSeconds[0] > candidate.perSkillSeconds[1] ||
        candidate.perSkillSeconds[1] > candidate.perSkillSeconds[2])
        throw std::runtime_error("MM6 combat buff duration falls with mastery");
    const Json &power = document.at("heroism_power");
    if (!power.is_array() || power.size() != 2)
        throw std::runtime_error("MM6 Heroism requires base and skill power");
    candidate.heroismBase = checkedNumber(power[0], 0, 1000);
    candidate.heroismPerSkill = checkedNumber(power[1], 1, 100);
    loadedRule = candidate;
}

bool isCombatBuffSpell(SpellId spellId) {
    return spellId == SPELL_AIR_SHIELD || spellId == SPELL_SPIRIT_HEROISM;
}

CombatBuffPlan planCombatBuff(SpellId spellId, Mastery mastery, int skillLevel, Time now) {
    const int rank = std::to_underlying(mastery);
    if (!isCombatBuffSpell(spellId) || rank < 1 || rank > 3 || skillLevel < 1 || skillLevel > 63 ||
        !now.isValid())
        throw std::runtime_error("Invalid MM6 combat buff spell, mastery, skill or clock");
    if (!loadedRule)
        throw std::runtime_error("MM6 combat buff rules have not been loaded");
    const CombatBuffRule &rule = *loadedRule;
    const int durationSeconds = rule.baseSeconds + rule.perSkillSeconds[rank - 1] * skillLevel;
    CombatBuffPlan result;
    result.buff = spellId == SPELL_AIR_SHIELD ? CHARACTER_BUFF_SHIELD : CHARACTER_BUFF_HEROISM;
    result.expires = now + Duration::fromSeconds(durationSeconds);
    result.mastery = mastery;
    result.power = spellId == SPELL_AIR_SHIELD ? 0 : rule.heroismBase + rule.heroismPerSkill * skillLevel;
    result.partyWide = mastery >= MASTERY_EXPERT;
    return result;
}

bool applyCombatBuff(SpellBuff &slot, const CombatBuffPlan &plan, int casterIndex) {
    if (plan.mastery < MASTERY_NOVICE || plan.mastery > MASTERY_MASTER ||
        (plan.buff != CHARACTER_BUFF_SHIELD && plan.buff != CHARACTER_BUFF_HEROISM) ||
        !plan.expires.isValid() || plan.power < 0 || plan.power > 7300 ||
        plan.partyWide != (plan.mastery >= MASTERY_EXPERT) ||
        (plan.buff == CHARACTER_BUFF_SHIELD && plan.power != 0) ||
        casterIndex < 0 || casterIndex >= 4)
        throw std::runtime_error("Invalid MM6 combat buff plan or caster");
    return slot.Apply(plan.expires, plan.mastery, plan.power, 0, casterIndex);
}

}  // namespace mm6
