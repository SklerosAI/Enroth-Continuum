#pragma once

#include <array>
#include <optional>
#include <string>
#include <vector>

#include "Engine/Data/HouseEnumFunctions.h"
#include "Engine/Data/SoundEnums.h"
#include "Engine/GameTitle.h"
#include "Engine/LocalizationEnums.h"
#include "Engine/MM6/MM6HouseExit.h"

#include "GUI/UI/MM6RoomDescriptors.h"

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
    HouseExit exit;
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

// The game-local source is loaded once from the active MM6 resources and
// private user-profile room manifest. MM7 does not call this path.
const HouseServiceData &houseServiceData();

enum class HouseEntryKind { TableBacked, BlankOrUnused, Terminal };

struct HouseEntryPlan {
    int houseId = 0;
    HouseEntryKind kind = HouseEntryKind::BlankOrUnused;
    HouseServiceFamily family = HouseServiceFamily::Other;
    std::optional<RoomDescriptor> room;
    std::optional<std::string> proprietorIconName;
    std::optional<int> openHour;
    std::optional<int> closeHour;
};

// A pre-entry candidate. It does not open the room or mutate the party/UI.
HouseEntryPlan planHouseEntry(const HouseServiceData &houses, int houseId);

/**
 * @param openHour                      House's open hour from 2DEvents.txt, `std::nullopt` for a house without hours.
 * @param closeHour                     House's close hour from 2DEvents.txt.
 * @param hour                          Current hour of the day.
 * @return                              Whether the house lets the party in. A house is open from the start of its open
 *                                      hour to the end of the hour before its close hour, across midnight when the
 *                                      close hour doesn't come after the open hour.
 */
bool isHouseOpen(std::optional<int> openHour, std::optional<int> closeHour, int hour);

struct HousePresentationState {
    int houseId = 0;
    std::string videoName;
    std::optional<std::string> proprietorIconName;
    std::optional<SoundId> candidateGreetingSoundId; // Read-only; the preview does not play it.
};

// A display-only MM6 room. Blank and terminal entries, and other titles,
// cannot activate the house preview window.
HousePresentationState planHousePresentation(GameTitle title, const HouseServiceData &houses, int houseId);

enum class HouseServiceOption {
    TempleHeal,
    TempleDonate,
    Train,
    BuyItems,
    SellItems,
    IdentifyItems,
    RepairItems,
    BuySpecialItems,
    BuySpells,
    LearnSkills,
    Deposit,
    Withdraw,
    RentRoom,
    FillPacks,
    HaveADrink,
    TipBarkeep,
    Travel,
    FixPrices
};

/**
 * @param type                          Parsed MM6 house type.
 * @return                              Services the proprietor of a house of this type lists, in menu order. A
 *                                      transport house lists `Travel` once for each of its three route slots, and a
 *                                      stable adds `FixPrices`.
 */
std::vector<HouseServiceOption> houseServiceOptions(HouseType type);

/**
 * @param option                        House service.
 * @return                              Id of the service's label in MM6's global.txt. Many of these ids hold other
 *                                      strings in MM7's global.txt. Throws for `FixPrices`, an NPC topic.
 */
LstrId houseServiceOptionLabel(HouseServiceOption option);

} // namespace mm6
