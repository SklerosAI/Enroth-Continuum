#pragma once

#include <cstddef>
#include <optional>

#include "Library/Geometry/Rect.h"

#include "MM6House.h"

class GUIButton;

/**
 * MM6 weapon, armor and magic shop, general store and spell guild. Buying takes an item or a spellbook from the stock
 * into the active character's backpack. Selling, identifying and repairing work on items the player clicks in that
 * backpack.
 */
class GUIWindow_MM6Shop : public GUIWindow_MM6House {
 public:
    GUIWindow_MM6Shop(HouseId houseId, const mm6::HousePresentationState &presentation);

    virtual void Update() override;
    virtual void selectService(int index) override;
    virtual bool goBack() override;
    virtual void houseScreenClick() override;

    /**
     * @return                          The shop screen the party is on, or `std::nullopt` on the proprietor's menu.
     */
    std::optional<mm6::HouseServiceOption> screen() const {
        return _screen;
    }

    /**
     * @param slot                      Stock slot, 0..11.
     * @return                          Where the slot's item is drawn on a buy screen, or an empty rect if the slot is
     *                                  empty or the party isn't on a buy screen.
     */
    Recti stockItemRect(int slot) const;

 private:
    void openScreen(mm6::HouseServiceOption option);
    void closeScreen();
    bool refreshStock();
    void buy(int slot);
    void sell();
    void identify();
    void repair();
    size_t stockRecordIndex(int slot) const;
    float multiplier() const;

    std::optional<mm6::HouseServiceOption> _screen;
    GUIButton *_screenButton = nullptr;
};
