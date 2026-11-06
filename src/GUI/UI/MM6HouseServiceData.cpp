#include "GUI/UI/MM6HouseServiceData.h"

#include <array>
#include <cmath>
#include <stdexcept>
#include <string>
#include <string_view>

#include "Engine/Tables/HouseTable.h"

#include "Library/Serialization/Serialization.h"

#include "Utility/Memory/Blob.h"
#include "Utility/String/Split.h"

namespace mm6 {
namespace {

std::optional<int> optionalNumber(std::string_view token, int minimum, int maximum) {
    if (token.empty())
        return std::nullopt;
    int value = 0;
    if (!tryDeserialize(token, &value) || value < minimum || value > maximum)
        throw std::runtime_error("MM6 house service field is outside retained bounds");
    return value;
}

std::string boundedText(std::string_view token) {
    if (token.size() > 256)
        throw std::runtime_error("MM6 house service text is too long");
    return std::string(token);
}

std::optional<float> optionalValue(std::string_view token) {
    if (token.empty())
        return std::nullopt;
    float value = 0;
    if (!tryDeserialize(token, &value) || !std::isfinite(value) || value < 0 || value > 1000)
        throw std::runtime_error("MM6 house service value is outside retained bounds");
    return value;
}

} // namespace

const HouseServiceRow &HouseServiceData::at(int houseId) const {
    if (houseId < 1 || houseId > 557)
        throw std::runtime_error("MM6 house service ID is outside retained table");
    return _rows[houseId];
}

HouseServiceData decodeHouseServiceData(const Blob &source, const RoomDescriptors &rooms) {
    if (source.size() > 131072)
        throw std::runtime_error("MM6 house service table is too large");

    const std::string_view text = source.str();
    HouseServiceData candidate;
    int expectedId = 1;
    for (std::string_view line : split(text).by("\r\n").drop(2).skip("")) {
        const std::array<std::string_view, 24> fields = split(line).by('\t').resize(24, "");
        const std::optional<int> houseId = optionalNumber(fields[0], 1, 557);
        if (!houseId || *houseId != expectedId)
            throw std::runtime_error("MM6 house service rows are missing or out of order");
        const std::string_view label = fields[2];
        const HouseType type = parseHouseType(label, GameTitle::MM6);
        if (type == HOUSE_TYPE_INVALID && !label.empty() && label != "Not Used")
            throw std::runtime_error("MM6 house service has unknown type label");

        HouseServiceRow &row = candidate._rows[expectedId];
        row.houseId = expectedId;
        row.type = type;
        row.family = houseServiceFamily(type);
        row.room = selectHouseRoom(rooms, expectedId, optionalNumber(fields[4], 1, 118));
        row.value = optionalValue(fields[12]);
        row.stockOrSkillText = boundedText(fields[13]);
        row.auxiliaryText = boundedText(fields[15]);
        row.openHour = optionalNumber(fields[18], 0, 24);
        row.closeHour = optionalNumber(fields[19], 0, 24);
        if (row.openHour.has_value() != row.closeHour.has_value())
            throw std::runtime_error("MM6 house service hours must form a pair");
        ++expectedId;
    }
    if (expectedId != 558)
        throw std::runtime_error("MM6 house service table requires 557 rows");
    return candidate;
}

} // namespace mm6
