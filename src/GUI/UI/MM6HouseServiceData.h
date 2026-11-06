#pragma once

#include <array>
#include <optional>
#include <string>

#include "Engine/Data/HouseEnumFunctions.h"

#include "GUI/UI/MM6RoomDescriptors.h"
#include "GUI/UI/Houses/MM6TransportRules.h"

class Blob;

namespace mm6 {

struct HouseServiceRow {
    int houseId = 0;
    HouseType type = HOUSE_TYPE_INVALID;
    HouseServiceFamily family = HouseServiceFamily::Other;
    HouseRoomSelection room{HouseRoomKind::Blank, std::nullopt};
    std::optional<float> value;
    std::string stockOrSkillText; // Raw MM6 column A, not MM7's skill-price multiplier.
    std::string auxiliaryText;    // Raw MM6 column C; can be a route or arena note.
    std::optional<int> openHour;
    std::optional<int> closeHour;
};

class HouseServiceData {
 public:
    const HouseServiceRow &at(int houseId) const;

 private:
    std::array<HouseServiceRow, 558> _rows;
    friend HouseServiceData decodeHouseServiceData(const Blob &source, const RoomDescriptors &rooms);
};

// Preserves the MM6 table's service inputs without applying MM7 stock, price,
// travel, sound, or dialogue rules. The house entry gate remains closed.
HouseServiceData decodeHouseServiceData(const Blob &source, const RoomDescriptors &rooms);

struct TransportMenuData {
    std::array<std::optional<TransportRoute>, 3> routes;

    bool hasAvailableRoute() const;
};

// Plans the three visible route slots only for a validated MM6 transport house.
// Unresolved weekday, special, and quest-gated routes stay absent.
TransportMenuData selectTransportMenu(const HouseServiceData &houses, const TransportRules &rules,
                                      int houseId, int weekday);

} // namespace mm6
