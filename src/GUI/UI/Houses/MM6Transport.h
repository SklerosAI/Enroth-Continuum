#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>

#include "MM6House.h"
#include "MM6TransportRules.h"

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

 private:
    /**
     * @return                          Route of each menu slot today, `nullptr` for a hidden slot.
     */
    std::array<const mm6::TransportRoute *, 3> offeredRoutes() const;

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
     * Moves the paid party to the route's arrival point and leaves the house.
     */
    void depart();

    const mm6::TransportRoute *_route = nullptr; // Route the party paid for.
    std::optional<int64_t> _departureTime; // Platform tick count at which the paid party leaves.
    bool _departed = false;
};
