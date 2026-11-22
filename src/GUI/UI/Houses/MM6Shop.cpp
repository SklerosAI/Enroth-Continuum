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
#include "Engine/MM6/MM6ItemGenerator.h"
#include "Engine/MM6/MM6ShopBoundedPurchase.h"
#include "Engine/MM6/MM6ShopRefreshBoundary.h"
#include "Engine/MM6/MM6ShopTitleState.h"
#include "Engine/Objects/Item.h"
#include "Engine/Party.h"
#include "Engine/Random/Random.h"
#include "Engine/Tables/HouseTable.h"
#include "Engine/Tables/ItemTable.h"

#include "GUI/GUIButton.h"
#include "GUI/GUIFont.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/ItemGrid.h"
#include "GUI/UI/MM6CharacterScreen.h"
#include "GUI/UI/UIStatusBar.h"

#include "Io/Mouse.h"

#include "Library/Logger/Logger.h"

#include "Media/Audio/AudioPlayer.h"

#include "MM6ShopPrices.h"
#include "MM6ShopStock.h"

static constexpr LstrId LSTR_MM6_PACK_IS_FULL = static_cast<LstrId>(563);
static constexpr LstrId LSTR_MM6_DONE = static_cast<LstrId>(569);
static constexpr LstrId LSTR_MM6_GOOD_AS_NEW = static_cast<LstrId>(570);
static constexpr LstrId LSTR_MM6_SELECT_ITEM_TO_BUY = static_cast<LstrId>(195);
static constexpr LstrId LSTR_MM6_SELECT_SPECIAL_ITEM_TO_BUY = static_cast<LstrId>(196);
static constexpr LstrId LSTR_MM6_SELECT_ITEM_TO_IDENTIFY = static_cast<LstrId>(197);
static constexpr LstrId LSTR_MM6_SELECT_ITEM_TO_REPAIR = static_cast<LstrId>(198);
static constexpr LstrId LSTR_MM6_SELECT_ITEM_TO_SELL = static_cast<LstrId>(199);

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

/**
 * @param type                          Shop type.
 * @return                              Number of stock slots the shop puts on display.
 */
static int displayedSlotCount(HouseType type) {
    switch (type) {
        case HOUSE_TYPE_WEAPON_SHOP: return 6;
        case HOUSE_TYPE_ARMOR_SHOP: return 8;
        case HOUSE_TYPE_GENERAL_STORE: return 6;
        default: return 12;
    }
}

static LstrId footerLine(mm6::HouseServiceOption screen) {
    using enum mm6::HouseServiceOption;
    switch (screen) {
        case BuySpecialItems: return LSTR_MM6_SELECT_SPECIAL_ITEM_TO_BUY;
        case SellItems: return LSTR_MM6_SELECT_ITEM_TO_SELL;
        case IdentifyItems: return LSTR_MM6_SELECT_ITEM_TO_IDENTIFY;
        case RepairItems: return LSTR_MM6_SELECT_ITEM_TO_REPAIR;
        default: return LSTR_MM6_SELECT_ITEM_TO_BUY;
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
            render->DrawQuad2D(stockIcon(slot), rect.topLeft());
        }
    } else {
        render->DrawQuad2D(assets->getIcon("leather"), {8, 8});
        if (pParty->hasActiveCharacter())
            CharacterUI_InventoryTab_Draw(&pParty->activeCharacter(), false);
    }
    drawMM6CharacterScreenBorders();

    const std::string footer = localization->str(footerLine(*_screen));
    render->DrawQuad2D(assets->getIcon("footer"), {0, 352});
    DrawText(assets->pFontLucida.get(), {assets->pFontLucida->AlignText_Center(450, footer) + 11, 355}, colorTable.White, footer, frameRect);

    if (pParty->hasActiveCharacter() && !pParty->activeCharacter().CanAct()) {
        const std::string refusal = localization->format(LSTR_S_IS_IN_NO_CONDITION_TO_S, pParty->activeCharacter().name, localization->str(LSTR_DO_ANYTHING));
        drawPanelText(refusal, (212 - panelTextHeight(refusal)) / 2 + 101, colorTable.PaleCanary);
    }
}

void GUIWindow_MM6Shop::selectService(int index) {
    if (turnsAwayActiveCharacter())
        return;
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
    if (!pParty->activeCharacter().CanAct()) {
        pAudioPlayer->playUISound(SOUND_error);
        return;
    }
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

    const Sizei size = stockIcon(slot)->size();
    const int column = slot % 6;
    if (_screen == mm6::HouseServiceOption::BuySpells)
        return Recti(32 + 70 * column, slot < 6 ? 90 : 250, size.w, size.h); // Two rows of six books.
    if (slot >= displayedSlotCount(_house.type))
        return {};

    if (_house.type == HOUSE_TYPE_WEAPON_SHOP)
        return Recti(60 + 70 * slot - size.w / 2, 30 + _weaponDrops[slot], size.w, size.h); // Weapons hang from the top of the table.
    if (_house.type == HOUSE_TYPE_ARMOR_SHOP) {
        if (slot < 4)
            return Recti(86 + 105 * slot - size.w / 2, 98 - size.h, size.w, size.h);
        return Recti(90 + 105 * (slot - 4) - size.w / 2, 126, size.w, size.h);
    }

    // Items stand on a shelf, centered 75 pixels apart. The general store has one shelf, the magic shop two.
    int x = 40 + 75 * column - size.w / 2;
    if (column == 0)
        x = std::max(x, 18);
    if (column == 5)
        x = std::min(x, 457 - size.w);
    int shelf = slot < 6 ? 152 : 306;
    if (_house.type == HOUSE_TYPE_GENERAL_STORE)
        shelf = 308;
    int y = shelf - size.h;
    if (slot < 6 && y < 1)
        y = 0;
    return Recti(x, y, size.w, size.h);
}

bool GUIWindow_MM6Shop::menuNeedsActingCharacter() const {
    const HouseType type = _house.type;
    return !_screen && isProprietorSelected() &&
           (type == HOUSE_TYPE_WEAPON_SHOP || type == HOUSE_TYPE_ARMOR_SHOP || type == HOUSE_TYPE_MAGIC_SHOP || type == HOUSE_TYPE_GENERAL_STORE);
}

void GUIWindow_MM6Shop::openScreen(mm6::HouseServiceOption option) {
    _screen = option;
    if (isBuyScreen(option) && !refreshStock()) {
        _screen.reset();
        engine->_statusBar->setEvent("This service is not available yet.");
        return;
    }
    if (_house.type == HOUSE_TYPE_WEAPON_SHOP && option != mm6::HouseServiceOption::BuySpells) {
        for (int slot = 0; slot < 6; ++slot)
            if (mm6::shopTitleState().stock.records[stockRecordIndex(slot)].itemNumberRaw())
                _weaponDrops[slot] = vrng->random(300 - stockIcon(slot)->height());
    }
    clearOptionButtons();
    _screenButton = CreateButton("MM6Shop_Screen", {8, 8}, {449, 319}, BUTTON_TYPE_NORMAL, 0, UIMSG_HouseScreenClick, 0);
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
            refreshed = mm6::refreshGuildStockForEntry(&shops.stock, &shops.clocks, house, now, next, [&](mm6::ShopStockFields &stock) {
                mm6::generateGuildStockWithTitleRng(&stock, rules, house, type, &rng);
            }, GameTitle::MM6, _refreshPolicy);
        } else {
            const mm6::ItemKindRoll kindRoll = engine->config->gameplay.FixItemGeneration.value() ? mm6::ITEM_KIND_ROLL_FIXED : mm6::ITEM_KIND_ROLL_VANILLA;
            const auto generate = [&](mm6::ShopStockBank stockBank) {
                return [&, stockBank](mm6::ShopStockFields &stock) {
                    mm6::generateShopBankWithTitleRng(&stock, rules, stockBank, house, type, &rng, *pItemTable, kindRoll);
                };
            };
            refreshed = mm6::refreshShopStockForEntry(&shops.stock, &shops.clocks, house, now, next,
                                                      generate(mm6::ShopStockBank::First), generate(mm6::ShopStockBank::Second), GameTitle::MM6,
                                                      _refreshPolicy);
        }
        if (refreshed)
            mm6::chestTitleState() = rng;
        _refreshPolicy = mm6::ShopRefreshPolicy::WhenExpired;
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
    const bool guild = _screen == mm6::HouseServiceOption::BuySpells;
    const int value = mm6::itemValue(source, *pItemTable);
    const int price = mm6::itemBuyPrice(value, multiplier(), mm6::merchantPercent(buyer));
    if (pParty->GetGold() < price) {
        engine->_statusBar->setEvent(LSTR_YOU_DONT_HAVE_ENOUGH_GOLD);
        mm6PlayHouseSound(houseId(), HOUSE_SOUND_GENERAL_NOT_ENOUGH_GOLD);
        return;
    }

    mm6::ItemRecord28 staged = source;
    if (!mm6::tryPurchaseBoundedStockItem(staged, buyer.inventory, *pItemTable, GameTitle::MM6)) {
        CharacterInventory probe = buyer.inventory;
        Item item;
        item.itemId = static_cast<ItemId>(source.itemNumberRaw());
        if (!probe.tryAdd(item)) {
            if (guild)
                return; // A guild says nothing when the pack is full.
            engine->_statusBar->setEvent(LSTR_MM6_PACK_IS_FULL);
            buyer.playReaction(SPEECH_NO_ROOM);
        } else {
            engine->_statusBar->setEvent("This item is not available yet.");
        }
        return;
    }
    source = staged;
    pParty->TakeGold(price);
    if (!guild)
        buyer.playReaction(SPEECH_ITEM_BUY);
}

void GUIWindow_MM6Shop::sell() {
    InventoryEntry entry = clickedBackpackEntry();
    if (!entry)
        return;
    Character &seller = pParty->activeCharacter();
    const bool blaster = engine->config->gameplay.FixShopsBuyingBlasters.value() && pItemTable->items[entry->itemId].baseValue == 0; // Blasters 64 and 65 have no value in MM6 items.txt.
    if (blaster || !mm6::shopTradesItem(std::to_underlying(houseId()), entry->itemId, pItemTable->items[entry->itemId])) {
        seller.playReaction(SPEECH_WRONG_SHOP);
        pAudioPlayer->playUISound(SOUND_error);
        return;
    }
    int value = 0;
    try {
        value = mm6::itemValue(*entry, *pItemTable);
    } catch (const std::exception &e) {
        MM_WARNING("MM6 shop {} can't price item {}: {}", std::to_underlying(houseId()), std::to_underlying(entry->itemId), e.what());
        engine->_statusBar->setEvent("This item can't be sold yet.");
        return;
    }
    const int payment = mm6::itemSalePayment(value, multiplier(), mm6::merchantPercent(seller),
                                             entry->IsBroken(), _house.type == HOUSE_TYPE_GENERAL_STORE);
    seller.inventory.take(entry);
    pParty->SetGold(pParty->GetGold() + payment);
    seller.playReaction(SPEECH_ITEM_SOLD);
}

void GUIWindow_MM6Shop::identify() {
    InventoryEntry entry = clickedBackpackEntry();
    if (!entry)
        return;
    Character &character = pParty->activeCharacter();
    if (entry->IsIdentified()) {
        character.playReaction(SPEECH_ALREADY_INDENTIFIED);
        return;
    }
    if (!mm6::shopTradesItem(std::to_underlying(houseId()), entry->itemId, pItemTable->items[entry->itemId])) {
        pAudioPlayer->playUISound(SOUND_error);
        character.playReaction(SPEECH_WRONG_SHOP);
        return;
    }
    const int price = mm6::itemIdentifyPrice(multiplier(), mm6::merchantPercent(character));
    if (pParty->GetGold() < price) {
        mm6PlayHouseSound(houseId(), HOUSE_SOUND_GENERAL_NOT_ENOUGH_GOLD);
        return;
    }
    pParty->TakeGold(price);
    entry->flags |= ITEM_IDENTIFIED;
    character.playReaction(SPEECH_SHOP_IDENTIFY);
    engine->_statusBar->setEvent(LSTR_MM6_DONE);
}

void GUIWindow_MM6Shop::repair() {
    InventoryEntry entry = clickedBackpackEntry();
    if (!entry)
        return;
    Character &character = pParty->activeCharacter();
    if (!entry->IsBroken()) {
        character.playReaction(SPEECH_ALREADY_INDENTIFIED); // MM6 answers an unbroken item with the already identified line.
        return;
    }
    if (!entry->mm6RawRecord && pItemTable->items[entry->itemId].type == ITEM_TYPE_POTION)
        return;
    if (!mm6::shopTradesItem(std::to_underlying(houseId()), entry->itemId, pItemTable->items[entry->itemId])) {
        pAudioPlayer->playUISound(SOUND_error);
        character.playReaction(SPEECH_WRONG_SHOP);
        return;
    }
    const int value = mm6::itemValue(*entry, *pItemTable);
    const int price = mm6::itemRepairPrice(value, multiplier(), mm6::merchantPercent(character));
    if (pParty->GetGold() < price) {
        mm6PlayHouseSound(houseId(), HOUSE_SOUND_GENERAL_NOT_ENOUGH_GOLD);
        return;
    }
    pParty->TakeGold(price);
    entry->flags = ITEM_IDENTIFIED; // MM6 overwrites the whole condition word.
    character.playReaction(SPEECH_SHOP_REPAIR);
    engine->_statusBar->setEvent(LSTR_MM6_GOOD_AS_NEW);
}

GraphicsImage *GUIWindow_MM6Shop::stockIcon(int slot) const {
    const mm6::ItemRecord28 &record = mm6::shopTitleState().stock.records[stockRecordIndex(slot)];
    return assets->getIcon(pItemTable->items[static_cast<ItemId>(record.itemNumberRaw())].iconName);
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
