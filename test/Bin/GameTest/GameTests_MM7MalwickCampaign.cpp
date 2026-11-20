#include "Testing/Game/GameTest.h"

#include "Engine/EngineGlobals.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Party.h"
#include "Engine/Resources/EngineFileSystem.h"

#include "MM7CampaignChain.h"

GAME_TEST(MM7MalwickCampaign, DISABLED_LightChapter3) {
    // The Light Arbiter choice used to omit Malwick's Wand, leaving his six-week countdown dormant.
    assertCampaignChainProfile();
    loadCampaignChainSlot(game, 3);
    EXPECT_TRUE(pParty->_questBits[QBIT_LIGHT_PATH]);
    EXPECT_FALSE(pParty->_questBits[QBIT_DARK_PATH]);
    EXPECT_TRUE(pParty->_questBits[QBIT_MALWICK_FIREBALL_WAND_ACCEPTED]);
    EXPECT_TRUE(pParty->hasItem(ITEM_ALACORN_WAND_OF_FIREBALLS));
    EXPECT_TRUE(pParty->PartyTimes.CounterEventValues[4].isValid());
    EXPECT_FALSE(pParty->_questBits[QBIT_MERCENARY_GUILD_VISIT_ACTIVE]);
    EXPECT_FALSE(pParty->hasItem(ITEM_MESSAGE_FROM_MR_STANTLEY));
    EXPECT_TRUE(ufs->exists("saves/save003.mm7"));
}

GAME_TEST(MM7MalwickCampaign, DISABLED_DarkChapter3) {
    // The Dark Arbiter choice used to omit Malwick's Wand, leaving his six-week countdown dormant.
    assertCampaignChainProfile();
    loadCampaignChainSlot(game, 3);
    EXPECT_FALSE(pParty->_questBits[QBIT_LIGHT_PATH]);
    EXPECT_TRUE(pParty->_questBits[QBIT_DARK_PATH]);
    EXPECT_TRUE(pParty->_questBits[QBIT_MALWICK_FIREBALL_WAND_ACCEPTED]);
    EXPECT_TRUE(pParty->hasItem(ITEM_ALACORN_WAND_OF_FIREBALLS));
    EXPECT_TRUE(pParty->PartyTimes.CounterEventValues[4].isValid());
    EXPECT_FALSE(pParty->_questBits[QBIT_MERCENARY_GUILD_VISIT_ACTIVE]);
    EXPECT_FALSE(pParty->hasItem(ITEM_MESSAGE_FROM_MR_STANTLEY));
    EXPECT_TRUE(ufs->exists("saves/save003.mm7"));
}
