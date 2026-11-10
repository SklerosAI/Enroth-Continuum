#include "MM6PotionCombinations.h"

#include <array>
#include <charconv>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace mm6 {
namespace {

bool asciiLetter(char c) { return c >= 'A' && c <= 'Z' || c >= 'a' && c <= 'z'; }
bool asciiDigit(char c) { return c >= '0' && c <= '9'; }

std::string_view normalized(std::string_view value) {
    while (!value.empty() && (value.front() == ' ' || value.front() == '\r'))
        value.remove_prefix(1);
    while (!value.empty() && (value.back() == ' ' || value.back() == '\r'))
        value.remove_suffix(1);
    if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
        value.remove_prefix(1);
        value.remove_suffix(1);
    }
    return value;
}

std::optional<int> unsignedId(std::string_view value) {
    value = normalized(value);
    if (value.empty())
        return std::nullopt;
    for (char c : value)
        if (!asciiDigit(c))
            return std::nullopt;
    int result = 0;
    auto parsed = std::from_chars(value.data(), value.data() + value.size(), result);
    return parsed.ec == std::errc() && parsed.ptr == value.data() + value.size()
        ? std::optional<int>(result) : std::nullopt;
}

std::vector<std::string_view> cells(std::string_view line) {
    std::vector<std::string_view> result;
    while (true) {
        std::size_t end = line.find('\t');
        result.push_back(line.substr(0, end));
        if (end == std::string_view::npos)
            return result;
        line.remove_prefix(end + 1);
    }
}

PotionCombinationOutcome parseCell(std::string_view token) {
    token = normalized(token);
    if (std::optional<int> id = unsignedId(token)) {
        if (*id < PotionCombinations::FIRST_ID || *id > PotionCombinations::LAST_ID)
            throw std::runtime_error("MM6 potion product is outside the matrix axis");
        return {PotionCombinationKind::Product, static_cast<ItemId>(*id), {}};
    }
    if (token.size() != 2 || !asciiLetter(token[0]) ||
        !(asciiDigit(token[1]) || asciiLetter(token[1])))
        throw std::runtime_error("unsupported MM6 potion combination token");
    return {PotionCombinationKind::OpaqueMarker, ITEM_NULL, {token[0], token[1]}};
}

} // namespace

PotionCombinations PotionCombinations::parse(std::string_view text) {
    if (text.size() > 65536)
        throw std::runtime_error("MM6 potion matrix exceeds size limit");

    std::array<std::array<std::string_view, COUNT>, COUNT> tokens = {};
    std::array<bool, COUNT> seen = {};
    int rows = 0;
    int matchingHeaders = 0;
    int lineCount = 0;
    while (!text.empty()) {
        if (++lineCount > 256)
            throw std::runtime_error("MM6 potion matrix exceeds line limit");
        std::size_t end = text.find('\n');
        std::string_view line = text.substr(0, end);
        text = end == std::string_view::npos ? std::string_view() : text.substr(end + 1);
        std::vector<std::string_view> row = cells(line);

        if (std::optional<int> id = unsignedId(row[0])) {
            if (*id < FIRST_ID || *id > LAST_ID || row.size() != 35)
                throw std::runtime_error("invalid MM6 potion matrix row");
            int index = *id - FIRST_ID;
            if (seen[index])
                throw std::runtime_error("duplicate MM6 potion matrix row");
            seen[index] = true;
            ++rows;
            for (int column = 0; column < COUNT; ++column)
                tokens[index][column] = row[column + 6];
        } else if (row.size() >= 35) {
            bool matching = true;
            for (int column = 0; column < COUNT; ++column)
                matching &= unsignedId(row[column + 6]) == FIRST_ID + column;
            matchingHeaders += matching;
        }
    }
    if (rows != COUNT || matchingHeaders != 1)
        throw std::runtime_error("incomplete MM6 potion matrix axes");

    PotionCombinations result;
    for (int row = 0; row < COUNT; ++row)
        for (int column = 0; column < COUNT; ++column)
            result._cells[row * COUNT + column] = parseCell(tokens[row][column]);
    for (int row = 0; row < COUNT; ++row) {
        if (result._cells[row * COUNT + row] != result._cells[0])
            throw std::runtime_error("inconsistent MM6 potion matrix diagonal");
        for (int column = row + 1; column < COUNT; ++column)
            if (result._cells[row * COUNT + column] != result._cells[column * COUNT + row])
                throw std::runtime_error("asymmetric MM6 potion matrix");
    }
    if (result._cells[0].kind != PotionCombinationKind::OpaqueMarker ||
        !asciiLetter(result._cells[0].marker[1]))
        throw std::runtime_error("unexpected MM6 potion matrix diagonal");
    result._loaded = true;
    return result;
}

PotionCombinationOutcome PotionCombinations::lookup(ItemId first, ItemId second) const {
    int row = static_cast<int>(first) - FIRST_ID;
    int column = static_cast<int>(second) - FIRST_ID;
    if (!_loaded || row < 0 || row >= COUNT || column < 0 || column >= COUNT)
        throw std::out_of_range("MM6 potion combination is not loaded or is outside the matrix axis");
    return _cells[row * COUNT + column];
}

} // namespace mm6
