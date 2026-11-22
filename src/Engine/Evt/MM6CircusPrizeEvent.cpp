#include "Engine/Evt/MM6CircusPrizeEvent.h"

#include <algorithm>
#include <array>
#include <utility>

#include "Engine/Objects/Character.h"

namespace mm6 {

int circusPrizeScore(std::span<const Character> characters) {
    int score = 0;
    for (const Character &character : characters) {
        for (InventoryConstEntry entry : character.inventory.entries()) {
            switch (std::to_underlying(entry->itemId)) {
            case 470: score += 1; break;
            case 471: score += 3; break;
            case 477: score += 5; break;
            default: break;
            }
        }
    }
    return score;
}

bool isCircusPrizeTrade(const EvtContext &context, std::span<const EvtInstruction> instructions, const EvtInstruction &reward) {
    if (context.source != EVT_SOURCE_GLOBAL || context.eventId < 129 || context.eventId > 134)
        return false;
    const bool pyramid = context.eventId <= 131;
    const int prize = pyramid ? 472 : 473; // Golden Pyramid, Keg of Wine.
    const int level = pyramid ? 6 : 4;
    constexpr std::array<RandomItemType, 3> kinds = {RANDOM_ITEM_ARMOR, RANDOM_ITEM_WEAPON, RANDOM_ITEM_MICS};
    const RandomItemType kind = kinds[(context.eventId - 129) % 3];

    const auto givesItem = [&](const EvtInstruction &ir) {
        return ir.opcode == EVENT_GiveItem && ir.data.give_item_descr.item_id == ITEM_NULL &&
               std::to_underlying(ir.data.give_item_descr.treasure_level) == level && ir.data.give_item_descr.treasure_type == kind;
    };
    const auto at = [&](int step, auto matches) {
        return std::ranges::any_of(instructions, [&](const EvtInstruction &ir) { return ir.step == step && matches(ir); });
    };
    const auto usesPrize = [&](EvtOpcode opcode) {
        return [=](const EvtInstruction &ir) {
            return ir.opcode == opcode && ir.data.variable_descr.type == VAR_PlayerItemInHands && ir.data.variable_descr.value == prize;
        };
    };

    if (!givesItem(reward) || reward.step < 4 || reward.step > 25 || reward.step % 7 != 4)
        return false;
    for (int member = 0; member < 4; ++member) {
        const int branch = 7 * member;
        if (!at(branch + 1, usesPrize(EVENT_Compare)) || !at(branch + 3, usesPrize(EVENT_Subtract)) || !at(branch + 4, givesItem))
            return false;
    }
    return true;
}

} // namespace mm6
