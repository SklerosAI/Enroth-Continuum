#include "Engine/Spells/MM6FirstAidRules.h"

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

std::optional<std::array<int, 3>> loadedHealAmounts;

void requireHash(const Json &document, const char *key) {
    const Json &value = document.at(key);
    if (!value.is_string())
        throw std::runtime_error("MM6 First Aid source fingerprint must be a string");
    const std::string hash = value.get<std::string>();
    if (hash.size() != 64)
        throw std::runtime_error("MM6 First Aid source fingerprint has wrong length");
    for (unsigned char character : hash)
        if (!std::isxdigit(character))
            throw std::runtime_error("MM6 First Aid source fingerprint is not hexadecimal");
}

int checkedAmount(const Json &value) {
    if (!value.is_number_integer() || value.is_number_unsigned()) {
        // nlohmann's unsigned values still pass is_number_integer(); handle
        // them without a narrowing conversion below.
        if (!value.is_number_unsigned())
            throw std::runtime_error("MM6 First Aid amount is not an integer");
        const auto amount = value.get<std::uint64_t>();
        if (amount < 1 || amount > 1000)
            throw std::runtime_error("MM6 First Aid amount is outside bounds");
        return static_cast<int>(amount);
    }
    const auto amount = value.get<std::int64_t>();
    if (amount < 1 || amount > 1000)
        throw std::runtime_error("MM6 First Aid amount is outside bounds");
    return static_cast<int>(amount);
}

}  // namespace

void loadFirstAidRules(const Blob &source) {
    if (source.size() > 1024)
        throw std::runtime_error("MM6 First Aid manifest is too large");
    const Json document = Json::parse(source.str());
    if (!document.is_object() || document.size() != 5 || document.at("format") != 1 ||
        document.at("id") != std::to_underlying(SPELL_BODY_FIRST_AID))
        throw std::runtime_error("Invalid MM6 First Aid manifest header");
    requireHash(document, "spell_report_sha256");
    requireHash(document, "guide_report_sha256");

    const Json &values = document.at("heal");
    if (!values.is_array() || values.size() != 3)
        throw std::runtime_error("MM6 First Aid requires three mastery amounts");
    std::array<int, 3> candidate{};
    for (size_t i = 0; i < candidate.size(); ++i)
        candidate[i] = checkedAmount(values[i]);
    if (candidate[0] > candidate[1] || candidate[1] > candidate[2])
        throw std::runtime_error("MM6 First Aid heal amount falls with mastery");
    loadedHealAmounts = std::move(candidate);
}

int firstAidHealAmount(SpellId spellId, Mastery mastery) {
    const int rank = std::to_underlying(mastery);
    if (spellId != SPELL_BODY_FIRST_AID || rank < 1 || rank > 3)
        throw std::runtime_error("MM6 First Aid lookup has invalid ID or mastery");
    if (!loadedHealAmounts)
        throw std::runtime_error("MM6 First Aid rules have not been loaded");
    return (*loadedHealAmounts)[rank - 1];
}

}  // namespace mm6
