#include "MM6HeldItemSaveFields.h"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <utility>

#include "Engine/Engine.h"
#include "Engine/AttackList.h"
#include "Engine/Objects/Item.h"
#include "Engine/Party.h"
#include "Engine/Spells/CastSpellInfo.h"
#include "Engine/Tables/ItemTable.h"
#include "Engine/Timer.h"
#include "Engine/TurnEngine/TurnEngine.h"
#include "Engine/mm7_data.h"

#include "GUI/GUIWindow.h"
#include "GUI/UI/UIRest.h"

namespace mm6 {
namespace {

void requireMM6(GameTitle title, bool requireEngine) {
    if (title != GameTitle::MM6 || (requireEngine && !engine) ||
        (engine && engine->gameTitle() != GameTitle::MM6))
        throw std::runtime_error("MM6 held item cannot apply to another title");
}

void requireLoadedItem(const HeldItemSaveFields &fields) {
    if (!fields.present)
        return;
    if (!pItemTable)
        throw std::runtime_error("MM6 item table must be loaded before held-item staging");
    const ItemId itemId = static_cast<ItemId>(fields.itemId);
    if (pItemTable->items[itemId].iconName.empty())
        throw std::runtime_error("MM6 held item has no loaded template icon");
    Sizei size = pItemTable->itemSizes[itemId];
    if (size.w < 1 || size.w > 14 || size.h < 1 || size.h > 9)
        throw std::runtime_error("MM6 held item has no usable loaded footprint");
}

} // namespace

HeldItemSaveFields captureHeldItem(const Party &party, GameTitle title) {
    // Empty cursor capture needs no engine or item table; populated items
    // still pass requireLoadedItem below.
    requireMM6(title, false);
    const Item &item = party.pPickedItem;
    HeldItemSaveFields fields;
    if (item.itemId == ITEM_NULL)
        return fields;

    fields.present = true;
    fields.itemId = std::to_underlying(item.itemId);
    fields.potionPower = item.potionPower;
    fields.goldAmount = item.goldAmount;
    fields.standardEnchantment = item.standardEnchantment
        ? std::to_underlying(*item.standardEnchantment) : -1;
    fields.standardStrength = item.standardEnchantmentStrength;
    fields.specialEnchantment = std::to_underlying(item.specialEnchantment);
    fields.charges = item.numCharges;
    fields.maxCharges = item.maxCharges;
    fields.flags = static_cast<std::uint32_t>(item.flags);
    fields.lichJarCharacterIndex = item.lichJarCharacterIndex;
    fields.expirationTicks = item.enchantmentExpirationTime.ticks();
    (void) encodeHeldItemSection(fields);
    requireLoadedItem(fields);
    return fields;
}

Party stageHeldItemParty(const Party &live, std::span<const std::uint8_t> bytes, GameTitle title) {
    requireMM6(title, false);
    HeldItemSaveFields fields;
    loadHeldItemBeforeMutation(bytes, &fields);
    requireLoadedItem(fields);

    Party staged = live;
    Item &item = staged.pPickedItem;
    item.Reset();
    if (fields.present) {
        item.itemId = static_cast<ItemId>(fields.itemId);
        item.potionPower = fields.potionPower;
        item.goldAmount = fields.goldAmount;
        if (fields.standardEnchantment >= 0)
            item.standardEnchantment = static_cast<Attribute>(fields.standardEnchantment);
        item.standardEnchantmentStrength = fields.standardStrength;
        item.specialEnchantment = static_cast<ItemEnchantment>(fields.specialEnchantment);
        item.numCharges = fields.charges;
        item.maxCharges = fields.maxCharges;
        item.flags = ItemFlags(fields.flags);
        item.lichJarCharacterIndex = fields.lichJarCharacterIndex;
        item.enchantmentExpirationTime = Time::fromTicks(fields.expirationTicks);
    }
    return staged;
}

PendingActionStateForSave capturePendingActionStateForSave(GameTitle title) {
    requireMM6(title, true);
    if (!pParty || !pTurnEngine || !gameTimer)
        throw std::runtime_error("MM6 pending-action preflight lacks live owners");
    PendingActionStateForSave state;
    state.spellQueueActive = CastSpellInfoHelpers::hasPendingSpellCastForSave();
    state.targetedSpellWindow = pGUIWindow_CastTargetedSpell != nullptr;
    state.enchanting = IsEnchantingInProgress || enchantingActiveCharacter != -1;
    state.turnModeActive = pParty->bTurnBasedModeOn;
    state.turnQueueActive = pTurnEngine->turn_stage != TE_NONE || !pTurnEngine->pQueue.empty();
    state.turnActionsPending = pTurnEngine->pending_actions != 0;
    state.turnPendingFlag = bool(pTurnEngine->flags & TE_HAVE_PENDING_ACTIONS);
    state.timerTurnModeActive = gameTimer->isTurnBased();
    state.restActionActive = currentRestType != REST_NONE || remainingRestTime != 0_ticks;
    state.restWindowActive = current_screen_type == SCREEN_REST;
    state.attackListActive = !attackList.empty();
    state.playerSelectionActive = std::any_of(pParty->playerAlreadyPicked.begin(),
        pParty->playerAlreadyPicked.end(), [](bool picked) { return picked; });
    return state;
}

HeldCursorRestorePlan stageHeldCursorRestorePlan(
    const HeldItemSaveFields &held, const ItemTable &items,
    const PendingActionStateForSave &actions, GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 held cursor cannot stage another title");
    if (actions.spellQueueActive || actions.targetedSpellWindow || actions.enchanting ||
        actions.turnModeActive || actions.turnQueueActive || actions.turnActionsPending ||
        actions.turnPendingFlag || actions.timerTurnModeActive ||
        actions.restActionActive || actions.restWindowActive ||
        actions.attackListActive || actions.playerSelectionActive)
        throw std::runtime_error("MM6 held cursor cannot discard a pending action");
    (void) encodeHeldItemSection(held); // Full scalar/canonical-empty validation.
    HeldCursorRestorePlan plan;
    if (!held.present)
        return plan;
    const ItemId id = static_cast<ItemId>(held.itemId);
    const auto &templateItem = items.items[id];
    const Sizei size = items.itemSizes[id];
    if (templateItem.iconName.empty() || size.w < 1 || size.w > 14 ||
        size.h < 1 || size.h > 9)
        throw std::runtime_error("MM6 held cursor has no usable loaded item icon");
    plan.iconName = templateItem.iconName;
    plan.itemCursor = true;
    return plan;
}

} // namespace mm6
