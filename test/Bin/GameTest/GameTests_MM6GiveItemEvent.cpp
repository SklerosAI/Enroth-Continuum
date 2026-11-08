#include <cstdint>
#include <string>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Evt/EvtInterpreter.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/MM6/MM6ChestTitleState.h"
#include "Engine/Party.h"

#include "Io/Mouse.h"

#include "Utility/Memory/Blob.h"

namespace {

struct ScopedPartyAndTitleState {
    Party party = *pParty;
    mm6::ChestTitleState titleState = mm6::chestTitleState();

    ~ScopedPartyAndTitleState() {
        *pParty = party;
        mm6::chestTitleState() = titleState;
        mouse->SetCursorImage("MICON1");
    }
};

std::string giveItem(char step, char treasureLevel, char kind, char itemId) {
    return std::string("\x0a\x01\x00", 3) + step + '\x29' + treasureLevel + kind + itemId + std::string(3, '\0');
}

int inventoryCount(int characterIndex, ItemId itemId) {
    int count = 0;
    for (InventoryConstEntry entry : pParty->pCharacters[characterIndex].inventory.entries())
        count += entry->itemId == itemId;
    return count;
}

} // namespace

GAME_TEST(MM6GiveItemEvent, RollsOnMM6GeneratorAndMovesHeldItemToActiveCharacter) {
    // MM6 GiveItem rolled its item with MM7 rules on the shared engine random instead of the MM6 generator.
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ScopedPartyAndTitleState restore;
    auto heldTape = tapes.custom([] { return pParty->pPickedItem.itemId; });
    auto heldSpecialTape = tapes.custom([] { return pParty->pPickedItem.specialEnchantment; });
    auto heldIdentifiedTape = tapes.custom([] { return pParty->pPickedItem.IsIdentified(); });
    auto lcgTape = tapes.custom([] { return mm6::chestTitleState().lcgState; });
    auto artifactCountTape = tapes.custom([] {
        int count = 0;
        for (std::uint8_t flag : mm6::chestTitleState().generatedArtifactFlags)
            count += flag;
        return count;
    });
    auto firstCharacterTape = tapes.custom([] { return inventoryCount(0, static_cast<ItemId>(8)); });
    auto activeCharacterTape = tapes.custom([] { return inventoryCount(2, static_cast<ItemId>(8)); });

    for (Character &character : pParty->pCharacters)
        character.inventory.clear();
    pParty->pPickedItem.Reset();
    pParty->setActiveCharacterIndex(2);
    ASSERT_TRUE(mm6::restoreChestTitleState(GameTitle::MM6, &mm6::chestTitleState(), 2, {}));

    // A level 6 sword with a fixed item, then a level 4 armor piece. Shipped MM6 scripts use both operands.
    EvtProgram program = EvtProgram::load(Blob::fromString(giveItem(0, 6, 23, 8) + giveItem(1, 4, 21, 0)), GameTitle::MM6);
    EvtInterpreter interpreter;
    test.startTaping();
    game.tick();
    EXPECT_EQ(interpreter.executeInstruction(program.instruction(1, 0)).type, EVT_FLOW_NEXT);
    game.tick();
    EXPECT_EQ(interpreter.executeInstruction(program.instruction(1, 1)).type, EVT_FLOW_NEXT);
    game.tick();
    test.stopTaping();

    // The sword roll is another sword with a special bonus, and the fixed item keeps that bonus. The armor roll has no
    // bonus, and the held sword goes to the active character, not the first one.
    EXPECT_EQ(heldTape, tape(ITEM_NULL, static_cast<ItemId>(8), static_cast<ItemId>(76)));
    EXPECT_EQ(heldSpecialTape, tape(ITEM_ENCHANTMENT_NULL, static_cast<ItemEnchantment>(9), ITEM_ENCHANTMENT_NULL));
    EXPECT_EQ(heldIdentifiedTape, tape(false));
    EXPECT_EQ(lcgTape, tape(2u, 1585885983u, 4079776401u));
    EXPECT_EQ(artifactCountTape, tape(0));
    EXPECT_EQ(firstCharacterTape, tape(0));
    EXPECT_EQ(activeCharacterTape, tape(0, 1));
    InventoryEntry stored = pParty->pCharacters[2].inventory.find(static_cast<ItemId>(8));
    ASSERT_TRUE(stored);
    EXPECT_EQ(stored->specialEnchantment, static_cast<ItemEnchantment>(9));
}
