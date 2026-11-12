#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Objects/Character.h"

#include "GUI/GUIWindow.h"

static constexpr Pointi firstFaceNextArrow = {134, 39};
static constexpr Pointi paladinClassName = {174, 425};
static constexpr Pointi bowAvailableSkill = {348, 425};
static constexpr Pointi minusCap = {491, 410};
static constexpr Pointi plusCap = {591, 410};
static constexpr Pointi okThumb = {542, 452};

static constexpr Class mm6Knight = static_cast<Class>(0);
static constexpr Class mm6Paladin = static_cast<Class>(9); // MM6 class families are three apart.

static void openPartyCreation(EngineController &game) {
    game.pressGuiButton("MainMenu_NewGame");
    game.tick(2); // The main menu closes on the first frame, party creation opens on the second.
}

GAME_TEST(MM6PartyCreationHeadless, ClicksReachVisibleControls) {
    // MM6 party creation used MM7's hit boxes over MM6's art, so clicks on the visible face arrow, class names,
    // available skills and plus/minus caps changed nothing or hit a different control.
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    openPartyCreation(game);
    ASSERT_EQ(current_screen_type, SCREEN_PARTY_CREATION);

    test.startTaping();
    auto screenTape = tapes.screen();
    auto faceTape = charTapes.face(0);
    auto classTape = charTapes.clazz(0);
    auto bowTape = charTapes.hasSkill(0, SKILL_BOW);
    auto mightTape = charTapes.stat(0, ATTRIBUTE_MIGHT);
    auto bonusTape = tapes.custom([] { return CharacterCreation_GetUnspentAttributePointCount(); });
    game.tick(); // Tapes the party as it was created.

    game.pressAndReleaseButton(BUTTON_LEFT, firstFaceNextArrow);
    game.tick();
    game.pressAndReleaseButton(BUTTON_LEFT, bowAvailableSkill);
    game.tick();
    game.pressAndReleaseButton(BUTTON_LEFT, plusCap);
    game.tick();
    game.pressAndReleaseButton(BUTTON_LEFT, minusCap);
    game.tick();
    game.pressAndReleaseButton(BUTTON_LEFT, paladinClassName);
    game.tick();

    EXPECT_EQ(faceTape, tape(0, 1));
    EXPECT_EQ(classTape, tape(mm6Knight, mm6Paladin));
    EXPECT_EQ(bowTape, tape(false, true, false)); // Changing class clears the optional skills.
    EXPECT_EQ(mightTape, tape(14, 15, 14));
    EXPECT_EQ(bonusTape, tape(50, 49, 50));
    EXPECT_EQ(screenTape, tape(SCREEN_PARTY_CREATION));
}

GAME_TEST(MM6PartyCreationHeadless, ThumbStartsGame) {
    // MM6's OK thumb sat under MM7's Clear hit box, so clicking it reset the party instead of starting the game.
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    test.startTaping();
    auto screenTape = tapes.screen();
    auto bonusTape = tapes.custom([] { return CharacterCreation_GetUnspentAttributePointCount(); });

    openPartyCreation(game);
    ASSERT_EQ(screenTape.back(), SCREEN_PARTY_CREATION);

    // Each character takes its first two optional skills and up to three points in every attribute.
    for (PlatformKey key : {PlatformKey::KEY_DIGIT_1, PlatformKey::KEY_DIGIT_2, PlatformKey::KEY_DIGIT_3, PlatformKey::KEY_DIGIT_4}) {
        game.pressAndReleaseKey(key);
        game.tick();
        for (int skill = 0; skill < 2; skill++) {
            game.pressGuiButton(fmt::format("PartyCreation_Skill{}", skill));
            game.tick();
        }
        for (int attribute = 0; attribute < 7; attribute++) {
            for (int point = 0; point < 3; point++) {
                game.pressAndReleaseKey(PlatformKey::KEY_ADD);
                game.tick();
            }
            game.pressAndReleaseKey(PlatformKey::KEY_DOWN);
            game.tick();
        }
    }
    ASSERT_EQ(bonusTape.back(), 0);

    game.pressAndReleaseButton(BUTTON_LEFT, okThumb);
    game.skipLoadingScreen();
    game.tick();

    EXPECT_EQ(screenTape.back(), SCREEN_GAME);
}
