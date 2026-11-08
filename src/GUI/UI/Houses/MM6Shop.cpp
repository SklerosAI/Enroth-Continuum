#include "MM6Shop.h"

#include <algorithm>
#include <exception>
#include <string>

#include "Engine/AssetsManager.h"
#include "Engine/Engine.h"
#include "Engine/EngineIocContainer.h"
#include "Engine/Graphics/Image.h"
#include "Engine/Graphics/Renderer/Renderer.h"
#include "Engine/Localization.h"
#include "Engine/MM6/MM6ChestTitleState.h"
#include "Engine/MM6/MM6ShopBoundedPurchase.h"
#include "Engine/MM6/MM6ShopRefreshBoundary.h"
#include "Engine/MM6/MM6ShopTitleState.h"
#include "Engine/Objects/Item.h"
#include "Engine/Party.h"
#include "Engine/Tables/HouseTable.h"
#include "Engine/Tables/ItemTable.h"

#include "GUI/GUIButton.h"
#include "GUI/GUIFont.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/ItemGrid.h"
#include "GUI/UI/UIStatusBar.h"

#include "Io/Mouse.h"

#include "Library/Logger/Logger.h"

#include "MM6ShopPrices.h"
#include "MM6ShopStock.h"

static constexpr LstrId LSTR_MM6_PACK_IS_FULL = static_cast<LstrId>(563);
static constexpr LstrId LSTR_MM6_DONE = static_cast<LstrId>(569);
static constexpr LstrId LSTR_MM6_GOOD_AS_NEW = static_cast<LstrId>(570);

static bool isBuyScreen(std::optional<mm6::HouseServiceOption> screen) {
    using enum mm6::HouseServiceOption;
    return screen == BuyItems || screen == BuySpecialItems || screen == BuySpells;
}

static bool isShopService(mm6::HouseServiceOption option) {
    using enum mm6::HouseServiceOption;
    return option == BuyItems || option == SellItems || option == IdentifyItems || option == RepairItems || option == BuySpecialItems ||
           option == BuySpells;
}

static const char *shelfIconName(HouseType type) {
    switch (type) {
        case HOUSE_TYPE_WEAPON_SHOP: return "wepntabl";
        case HOUSE_TYPE_ARMOR_SHOP: return "armory";
        case HOUSE_TYPE_GENERAL_STORE: return "genshelf";
        default: return "magshelf";
    }
}

static InventoryEntry clickedBackpackEntry() {
    const Pointi pt = EngineIocContainer::ResolveMouse()->position();
    if (pt.x <= 13 || pt.x >= 462)
        return {};
    return pParty->activeCharacter().inventory.entry(mapToInventoryGrid(pt, Pointi(14, 17)));
}

GUIWindow_MM6Shop::GUIWindow_MM6Shop(HouseId houseId, const mm6::HousePresentationState &presentation)
    : GUIWindow_MM6House(houseId, presentation) {}

void GUIWindow_MM6Shop::Update() {
    GUIWindow_MM6House::Update();
    if (!_screen)
        return;

    if (isBuyScreen(_screen)) {
        render->DrawQuad2D(assets->getIcon(shelfIconName(_house.type)), {8, 8});
        for (int slot = 0; slot < 12; ++slot) {
            const Recti rect = stockItemRect(slot);
            if (rect.w == 0)
                continue;
            const mm6::ItemRecord28 &record = mm6::shopTitleState().stock.records[stockRecordIndex(slot)];
            render->DrawQuad2D(assets->getIcon(pItemTable->items[static_cast<ItemId>(record.itemNumberRaw())].iconName), rect.topLeft());
        }
    } else if (pParty->hasActiveCharacter()) {
        CharacterUI_InventoryTab_Draw(&pParty->activeCharacter(), false);
    }
}

void GUIWindow_MM6Shop::selectService(int index) {
    if (_screen || !isProprietorSelected() || index < 0 || index >= static_cast<int>(_services.size()) || !isShopService(_services[index])) {
        GUIWindow_MM6House::selectService(index);
        return;
    }
    openScreen(_services[index]);
}

bool GUIWindow_MM6Shop::goBack() {
    if (!_screen)
        return GUIWindow_MM6House::goBack();
    closeScreen();
    return true;
}

void GUIWindow_MM6Shop::houseScreenClick() {
    if (!_screen || !pParty->hasActiveCharacter())
        return;
    switch (*_screen) {
        case mm6::HouseServiceOption::BuyItems:
        case mm6::HouseServiceOption::BuySpecialItems:
        case mm6::HouseServiceOption::BuySpells: {
            const Pointi pt = EngineIocContainer::ResolveMouse()->position();
            for (int slot = 0; slot < 12; ++slot) {
                if (stockItemRect(slot).contains(pt)) {
                    buy(slot);
                    return;
                }
            }
            return;
        }
        case mm6::HouseServiceOption::SellItems:
            sell();
            return;
        case mm6::HouseServiceOption::IdentifyItems:
            identify();
            return;
        case mm6::HouseServiceOption::RepairItems:
            repair();
            return;
        default:
            return;
    }
}

Recti GUIWindow_MM6Shop::stockItemRect(int slot) const {
    if (!isBuyScreen(_screen) || slot < 0 || slot >= 12)
        return {};
    const mm6::ShopTitleState &shops = mm6::shopTitleState();
    if (!shops.stock.records[stockRecordIndex(slot)].itemNumberRaw())
        return {};

    const auto iconSize = [&](int index) {
        const mm6::ItemRecord28 &record = shops.stock.records[stockRecordIndex(index)];
        GraphicsImage *icon = assets->getIcon(pItemTable->items[static_cast<ItemId>(record.itemNumberRaw())].iconName);
        return Sizei(icon->width(), icon->height());
    };
    const Sizei size = iconSize(slot);
    const int column = slot % 6;
    if (_screen == mm6::HouseServiceOption::BuySpells)
        return Recti(32 + 70 * column, slot < 6 ? 90 : 250, size.w, size.h); // Two rows of six books.

    // Two shelves of six, items centered 75 pixels apart and standing on the shelf line.
    int x = 40 + 75 * column - size.w / 2;
    if (column == 0)
        x = std::max(x, 18);
    if (column == 5)
        x = std::min(x, 457 - iconSize(slot < 6 ? 5 : 11).w);
    int y = (slot < 6 ? 152 : 306) - size.h;
    if (slot < 6 && y < 1)
        y = 0;
    return Recti(x, y, size.w, size.h);
}

void GUIWindow_MM6Shop::openScreen(mm6::HouseServiceOption option) {
    _screen = option;
    if (isBuyScreen(option) && !refreshStock()) {
        _screen.reset();
        engine->_statusBar->setEvent("This service is not available yet.");
        return;
    }
    clearOptionButtons();
    _screenButton = CreateButton("MM6Shop_Screen", {8, 8}, {454, 312}, BUTTON_TYPE_NORMAL, 0, UIMSG_HouseScreenClick, 0);
}

void GUIWindow_MM6Shop::closeScreen() {
    _screen.reset();
    if (_screenButton) {
        _screenButton->Release();
        _screenButton = nullptr;
    }
    showServiceButtons();
}

bool GUIWindow_MM6Shop::refreshStock() {
    mm6::ShopTitleState &shops = mm6::shopTitleState();
    if (!shops.ready())
        return false;
    const int house = std::to_underlying(houseId());
    const HouseType type = _house.type;
    mm6::ChestTitleState rng = mm6::chestTitleState();
    try {
        const mm6::ShopStockRules &rules = mm6::shopStockRules();
        const std::uint64_t now = pParty->GetPlayingTime().ticks();
        const std::uint64_t next = now + mm6::shopRefreshTicks(houseTable[houseId()].generation_interval_days);
        bool refreshed = false;
        if (_screen == mm6::HouseServiceOption::BuySpells) {
            refreshed = mm6::refreshGuildStockIfExpired(&shops.stock, &shops.clocks, house, now, next, [&](mm6::ShopStockFields &stock) {
                mm6::generateGuildStockWithTitleRng(&stock, rules, house, type, &rng);
            }, GameTitle::MM6);
        } else {
            const auto generate = [&](mm6::ShopStockBank stockBank) {
                return [&, stockBank](mm6::ShopStockFields &stock) {
                    mm6::generateShopBankWithTitleRng(&stock, rules, stockBank, house, type, &rng, *pItemTable);
                };
            };
            refreshed = mm6::refreshShopStockIfExpired(&shops.stock, &shops.clocks, house, now, next,
                                                       generate(mm6::ShopStockBank::First), generate(mm6::ShopStockBank::Second), GameTitle::MM6);
        }
        if (refreshed)
            mm6::chestTitleState() = rng;
    } catch (const std::exception &e) {
        MM_WARNING("MM6 shop {} stock refused: {}", house, e.what());
        return false;
    }
    return true;
}

void GUIWindow_MM6Shop::buy(int slot) {
    mm6::ShopTitleState &shops = mm6::shopTitleState();
    mm6::ItemRecord28 &source = shops.stock.records[stockRecordIndex(slot)];
    Character &buyer = pParty->activeCharacter();
    const int value = mm6::itemValue(source, *pItemTable);
    const int price = mm6::itemBuyPrice(value, multiplier(), mm6::merchantPercent(buyer));
    if (pParty->GetGold() < price) {
        engine->_statusBar->setEvent(LSTR_YOU_DONT_HAVE_ENOUGH_GOLD);
        return;
    }

    mm6::ItemRecord28 staged = source;
    if (!mm6::tryPurchaseBoundedStockItem(staged, buyer.inventory, *pItemTable, GameTitle::MM6)) {
        CharacterInventory probe = buyer.inventory;
        Item item;
        item.itemId = static_cast<ItemId>(source.itemNumberRaw());
        if (!probe.tryAdd(item)) {
            engine->_statusBar->setEvent(LSTR_MM6_PACK_IS_FULL);
        } else {
            engine->_statusBar->setEvent("This item is not available yet.");
        }
        return;
    }
    source = staged;
    pParty->TakeGold(price);
}

void GUIWindow_MM6Shop::sell() {
    InventoryEntry entry = clickedBackpackEntry();
    if (!entry || !mm6::shopTradesItem(std::to_underlying(houseId()), entry->itemId, pItemTable->items[entry->itemId]))
        return;
    int value = 0;
    try {
        value = mm6::itemValue(*entry, *pItemTable);
    } catch (const std::exception &e) {
        MM_WARNING("MM6 shop {} can't price item {}: {}", std::to_underlying(houseId()), std::to_underlying(entry->itemId), e.what());
        engine->_statusBar->setEvent("This item can't be sold yet.");
        return;
    }
    const int payment = mm6::itemSalePayment(value, multiplier(), mm6::merchantPercent(pParty->activeCharacter()),
                                             entry->IsBroken(), _house.type == HOUSE_TYPE_GENERAL_STORE);
    pParty->activeCharacter().inventory.take(entry);
    pParty->SetGold(pParty->GetGold() + payment);
}

void GUIWindow_MM6Shop::identify() {
    InventoryEntry entry = clickedBackpackEntry();
    if (!entry || entry->IsIdentified() ||
        !mm6::shopTradesItem(std::to_underlying(houseId()), entry->itemId, pItemTable->items[entry->itemId]))
        return;
    const int price = mm6::itemIdentifyPrice(multiplier(), mm6::merchantPercent(pParty->activeCharacter()));
    if (pParty->GetGold() < price) {
        engine->_statusBar->setEvent(LSTR_YOU_DONT_HAVE_ENOUGH_GOLD);
        return;
    }
    pParty->TakeGold(price);
    entry->flags |= ITEM_IDENTIFIED;
    engine->_statusBar->setEvent(LSTR_MM6_DONE);
}

void GUIWindow_MM6Shop::repair() {
    InventoryEntry entry = clickedBackpackEntry();
    if (!entry || !entry->IsBroken() || (!entry->mm6RawRecord && pItemTable->items[entry->itemId].type == ITEM_TYPE_POTION) ||
        !mm6::shopTradesItem(std::to_underlying(houseId()), entry->itemId, pItemTable->items[entry->itemId]))
        return;
    const int value = mm6::itemValue(*entry, *pItemTable);
    const int price = mm6::itemRepairPrice(value, multiplier(), mm6::merchantPercent(pParty->activeCharacter()));
    if (pParty->GetGold() < price) {
        engine->_statusBar->setEvent(LSTR_YOU_DONT_HAVE_ENOUGH_GOLD);
        return;
    }
    pParty->TakeGold(price);
    entry->flags = ITEM_IDENTIFIED; // MM6 overwrites the whole condition word.
    engine->_statusBar->setEvent(LSTR_MM6_GOOD_AS_NEW);
}

size_t GUIWindow_MM6Shop::stockRecordIndex(int slot) const {
    const int house = std::to_underlying(houseId());
    if (_screen == mm6::HouseServiceOption::BuySpells)
        return mm6::guildStockRecordIndex(house, slot, GameTitle::MM6);
    const mm6::ShopStockBank bank = _screen == mm6::HouseServiceOption::BuySpecialItems ? mm6::ShopStockBank::Second : mm6::ShopStockBank::First;
    return mm6::shopStockRecordIndex(bank, house, slot, GameTitle::MM6);
}

float GUIWindow_MM6Shop::multiplier() const {
    return _house.value.value_or(0);
}
