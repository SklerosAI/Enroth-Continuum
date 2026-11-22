#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>

#include "Engine/PartyEnums.h"

#include "MM6House.h"
#include "MM6TransportRules.h"

namespace mm6 {

/**
 * @param houseId                       MM6 stable or boat.
 * @return                              Route of each of the house's menu slots today, `nullptr` for a hidden slot.
 */
std::array<const TransportRoute *, 3> routesOfferedToday(HouseId houseId);

/**
 * @offset 0x49CDA0
 * @param houseId                       MM6 house id.
 * @return                              Whether the house is a stable or boat that offers no route today.
 */
bool isTransportIdleToday(HouseId houseId);

} // namespace mm6

/**
 * MM6 stable or boat. The proprietor lists the house's routes for today, and a paid trip passes its days with rest and
 * takes the party to the route's arrival point.
 */
class GUIWindow_MM6Transport : public GUIWindow_MM6House {
 public:
    GUIWindow_MM6Transport(HouseId houseId, const mm6::HousePresentationState &presentation);

    virtual void Update() override;
    virtual void selectService(int index) override;
    virtual bool goBack() override;

 protected:
    virtual std::string serviceLabel(int index) const override;
    virtual int stackedMenuTop() const override;

 private:
    /**
     * @return                          Line above the routes with what a trip costs the active character.
     */
    std::string priceLine() const;

    /**
     * @return                          Gold a trip costs the active character.
     */
    int price() const;

    /**
     * @param route                     Route the party takes.
     * @return                          Days the trip takes, fewer with a hired sailor, navigator, pirate, horseman or
     *                                  explorer, at least one.
     */
    int travelDays(const mm6::TransportRoute &route) const;

    /**
     * @return                          Whether the stable offers to fix its prices: the party has taken on Loretta
     *                                  Fleise's price fixing and this stable hasn't agreed yet.
     */
    bool offersToFixPrices() const;

    /**
     * The stable agrees to fix its prices. Once all nine have, the price fixing is done.
     */
    void fixPrices();

    /**
     * @return                          Quest bit set once this stable has agreed to fix its prices.
     */
    QuestBit stablePricesFixedBit() const;

    /**
     * Moves the party to the route's arrival point and leaves the house.
     */
    void depart();

    /**
     * Takes the fare and passes the trip's days with rest.
     */
    void payForTrip();

    const mm6::TransportRoute *_route = nullptr; // Route the party chose.
    int _fare = 0;
    int _tripDays = 0;
    bool _paysOnDeparture = false; // The fare and the days come after the departure's autosave.
    std::optional<int64_t> _departureTime; // Platform tick count at which the party leaves.
    bool _departed = false;
};
