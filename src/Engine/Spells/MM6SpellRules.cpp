#include "Engine/Spells/MM6SpellRules.h"

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

using SpellRows = std::array<std::array<SpellManaRecovery, 3>, 100>;
std::optional<SpellRows> loadedSpellRows;

int checkedNumber(const Json &value, int minimum, int maximum) {
    if (!value.is_number_integer())
        throw std::runtime_error("MM6 spell rule field is not an integer");
    if (value.is_number_unsigned()) {
        const std::uint64_t number = value.get<std::uint64_t>();
        if (number > static_cast<std::uint64_t>(maximum))
            throw std::runtime_error("MM6 spell rule field exceeds range");
        return static_cast<int>(number);
    }
    const std::int64_t number = value.get<std::int64_t>();
    if (number < minimum || number > maximum)
        throw std::runtime_error("MM6 spell rule field exceeds range");
    return static_cast<int>(number);
}

std::array<int, 3> checkedTriple(const Json &row, const char *name, int minimum, int maximum) {
    const Json &values = row.at(name);
    if (!values.is_array() || values.size() != 3)
        throw std::runtime_error("MM6 spell rule triple has wrong shape");
    std::array<int, 3> result{};
    for (size_t rank = 0; rank < result.size(); ++rank)
        result[rank] = checkedNumber(values[rank], minimum, maximum);
    return result;
}

} // namespace

void loadSpellRules(const Blob &source) {
    if (source.size() > 16384)
        throw std::runtime_error("MM6 spell rule manifest is too large");
    const Json document = Json::parse(source.str());
    if (!document.is_object() || document.size() != 3 || document.at("format") != 1 ||
        !document.at("source_report_sha256").is_string())
        throw std::runtime_error("Invalid MM6 spell rule manifest header");
    const std::string hash = document.at("source_report_sha256").get<std::string>();
    if (hash.size() != 64)
        throw std::runtime_error("Invalid MM6 spell rule report hash");
    for (unsigned char character : hash)
        if (!std::isxdigit(character))
            throw std::runtime_error("Invalid MM6 spell rule report hash");

    const Json &rows = document.at("spells");
    if (!rows.is_array() || rows.size() != 99)
        throw std::runtime_error("MM6 spell rule manifest requires 99 rows");
    SpellRows candidate{};
    for (int spellId = 1; spellId <= 99; ++spellId) {
        const Json &row = rows[spellId - 1];
        if (!row.is_object() || row.size() != 3 || checkedNumber(row.at("id"), 1, 99) != spellId)
            throw std::runtime_error("MM6 spell rule IDs are incomplete or out of order");
        const auto mana = checkedTriple(row, "mana", 0, 32767);
        const auto recovery = checkedTriple(row, "recovery", 1, 32767);
        if (recovery[0] < recovery[1] || recovery[1] < recovery[2])
            throw std::runtime_error("MM6 spell recovery rises with mastery");
        for (int rank = 0; rank < 3; ++rank)
            candidate[spellId][rank] = {static_cast<std::uint16_t>(mana[rank]),
                                        static_cast<std::int16_t>(recovery[rank])};
    }
    loadedSpellRows = std::move(candidate);
}

SpellManaRecovery spellManaRecovery(SpellId spellId, Mastery mastery) {
    const int id = std::to_underlying(spellId);
    const int rank = std::to_underlying(mastery);
    if (id < 1 || id > 99 || rank < 1 || rank > 3)
        throw std::runtime_error("MM6 spell rule lookup has invalid spell ID or mastery");
    if (!loadedSpellRows)
        throw std::runtime_error("MM6 spell rules have not been loaded");
    return (*loadedSpellRows)[id][rank - 1];
}

} // namespace mm6
