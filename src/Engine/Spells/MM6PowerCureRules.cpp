#include "Engine/Spells/MM6PowerCureRules.h"

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

struct PowerCureRule {
    int base = 0;
    int perSkill = 0;
};

std::optional<PowerCureRule> loadedRule;

void requireHash(const Json &document, const char *key) {
    const Json &value = document.at(key);
    if (!value.is_string())
        throw std::runtime_error("MM6 Power Cure source fingerprint must be a string");
    const std::string hash = value.get<std::string>();
    if (hash.size() != 64)
        throw std::runtime_error("MM6 Power Cure source fingerprint has wrong length");
    for (unsigned char character : hash)
        if (!std::isxdigit(character))
            throw std::runtime_error("MM6 Power Cure source fingerprint is not hexadecimal");
}

int checkedPositive(const Json &value, int maximum) {
    if (!value.is_number_integer())
        throw std::runtime_error("MM6 Power Cure amount is not an integer");
    if (value.is_number_unsigned()) {
        const auto number = value.get<std::uint64_t>();
        if (number < 1 || number > static_cast<std::uint64_t>(maximum))
            throw std::runtime_error("MM6 Power Cure amount is outside bounds");
        return static_cast<int>(number);
    }
    const auto number = value.get<std::int64_t>();
    if (number < 1 || number > maximum)
        throw std::runtime_error("MM6 Power Cure amount is outside bounds");
    return static_cast<int>(number);
}

}  // namespace

void loadPowerCureRules(const Blob &source) {
    if (source.size() > 1024)
        throw std::runtime_error("MM6 Power Cure manifest is too large");
    const Json document = Json::parse(source.str());
    if (!document.is_object() || document.size() != 6 || document.at("format") != 1 ||
        document.at("id") != std::to_underlying(SPELL_BODY_POWER_CURE))
        throw std::runtime_error("Invalid MM6 Power Cure manifest header");
    requireHash(document, "spell_report_sha256");
    requireHash(document, "guide_report_sha256");
    PowerCureRule candidate;
    candidate.base = checkedPositive(document.at("base"), 1000);
    candidate.perSkill = checkedPositive(document.at("per_skill"), 100);
    loadedRule = candidate;
}

int powerCureHealAmount(SpellId spellId, Mastery mastery, int skillLevel) {
    const int rank = std::to_underlying(mastery);
    if (spellId != SPELL_BODY_POWER_CURE || rank < 1 || rank > 3 || skillLevel < 1 || skillLevel > 63)
        throw std::runtime_error("MM6 Power Cure lookup has invalid ID, mastery or skill level");
    if (!loadedRule)
        throw std::runtime_error("MM6 Power Cure rules have not been loaded");
    return loadedRule->base + loadedRule->perSkill * skillLevel;
}

}  // namespace mm6
