#include <cstdlib>
#include <string_view>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Party.h"
#include "Engine/Objects/ItemEnums.h"

#include "GUI/UI/UIHouses.h"

namespace {

bool vendorModEnabledForTest() {
    const char *value = std::getenv("ENROTH_MM7_VENDOR_REFRESH_ON_ENTRY");
    return value && std::string_view(value) == "1";
}

constexpr int sentinelCharges = 1234567;

} // namespace

GAME_TEST(VendorRefreshIntegration, WeaponShopRefreshesOnEntryButNotBuyTab) {
    if (!vendorModEnabledForTest())
        GTEST_SKIP() << "Set the MM7 vendor mod flag to exercise a real entry";
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM7);

    engine->config->debug.NoMargaret.setValue(true);
    game.startNewGame();
    constexpr HouseId shop = HOUSE_WEAPON_SHOP_EMERALD_ISLAND;
    auto &standard = pParty->standartItemsInShops[shop][0];
    auto &special = pParty->specialItemsInShops[shop][0];
    standard.numCharges = sentinelCharges;
    special.numCharges = sentinelCharges;
    pParty->PartyTimes.shopNextRefreshTime[shop] =
        pParty->GetPlayingTime() + Duration::fromDays(365);

    game.teleportTo(MAP_EMERALD_ISLAND, Vec3f(13510, 4480, 128), 0);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(window_SpeakInHouse->houseId(), shop);
    EXPECT_NE(standard.itemId, ITEM_NULL);
    EXPECT_NE(special.itemId, ITEM_NULL);
    EXPECT_NE(standard.numCharges, sentinelCharges);
    EXPECT_NE(special.numCharges, sentinelCharges);

    // A future deadline makes any extra generation on this buy-tab change
    // detectable without relying on a random item ID changing by chance.
    standard.numCharges = sentinelCharges;
    special.numCharges = sentinelCharges;
    game.pressGuiButton("HouseDialogue_Option0"); // Buy Standard.
    game.tick();
    EXPECT_EQ(standard.numCharges, sentinelCharges);
    EXPECT_EQ(special.numCharges, sentinelCharges);
}

GAME_TEST(VendorRefreshIntegration, MagicGuildRefreshesOnEntryButNotBuyTab) {
    if (!vendorModEnabledForTest())
        GTEST_SKIP() << "Set the MM7 vendor mod flag to exercise a real entry";
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM7);

    game.startNewGame();
    constexpr HouseId guild = HOUSE_BODY_GUILD_ERATHIA;
    pParty->pCharacters[0]._achievedAwardsBits[AWARD_MEMBERSHIP_BODY_GUILD] = true;
    auto &book = pParty->spellBooksInGuilds[guild][0];
    book.numCharges = sentinelCharges;
    pParty->PartyTimes.guildNextRefreshTime[guild] =
        pParty->GetPlayingTime() + Duration::fromDays(365);

    game.teleportTo(MAP_ERATHIA, Vec3f(-528, 5710, 1440), 90);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(window_SpeakInHouse->houseId(), guild);
    EXPECT_NE(book.itemId, ITEM_NULL);
    EXPECT_NE(book.numCharges, sentinelCharges);

    book.numCharges = sentinelCharges;
    game.pressGuiButton("HouseDialogue_Option0"); // Buy Spells.
    game.tick();
    EXPECT_EQ(book.numCharges, sentinelCharges);
}
