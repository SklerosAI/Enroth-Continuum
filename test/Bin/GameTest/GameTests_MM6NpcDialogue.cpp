#include <cstdlib>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/Graphics/Renderer/Renderer.h"
#include "Engine/MM6/MM6ChestMapLoadRoute.h"
#include "Engine/MM6/MM6ScopedNewGameSave.h"
#include "Engine/Objects/CharacterEnumFunctions.h"
#include "Engine/Party.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/MapTable.h"

#include "GUI/GUIButton.h"
#include "GUI/GUIWindow.h"

#include "Library/Image/Png.h"

#include "Utility/Streams/FileOutputStream.h"
#include "Utility/System/Path.h"

static const Vec3f kMarianneSpot(-10333, -8532, 300); // In front of the general store, Marianne the Teacher stands close by.
static const Vec3f kEdisonSpot(-6360, -5864, 300); // Edison the Negotiator stands to the east.
static const Vec3f kMariaSpot(-12028, -7957, 300); // Maria the Scholar stands to the east.
static const Vec3f kCastleIronfistVillagerSpot(15556, 2776, 96); // A villager stands to the east, south of the castle.

static void enterNewSorpigal(EngineController &game) {
    // Default party through the creation screen, then New Sorpigal. The test-only new-game callback seeds the map
    // cache from the loaded archives and writes no save.
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    std::unique_ptr<mm6::ScopedChestMapLoadOptIn> chestOptIn;
    mm6::testing::setNewGameSaveOverride([] {
        auto firstMap = mm6::prepareNewGameMM6FromLoadedMaps();
        pMapDeltas.swap(firstMap.session.staged.worldAndVisuals.world.mapDeltas);
        mm6::chestTitleState() = firstMap.session.staged.worldAndVisuals.chestTitle;
    });
    game.runGameRoutine([&] { chestOptIn = std::make_unique<mm6::ScopedChestMapLoadOptIn>(); });
    game.pressGuiButton("MainMenu_NewGame");
    game.tick(2);
    ASSERT_NE(pGUIWindow_CurrentMenu, nullptr);
    game.runGameRoutine([] {
        for (Character &character : pParty->pCharacters) {
            for (int order = 4; order <= 12 && character.GetSkillIdxByOrder(3) == SKILL_INVALID; ++order) {
                const Skill skill = character.GetSkillIdxByOrder(order);
                if (skill != SKILL_INVALID && !character.pActiveSkills[skill])
                    character.pActiveSkills[skill] = CombinedSkillValue::novice();
            }
        }
        for (int pass = 0; pass < 100 && CharacterCreation_GetUnspentAttributePointCount() > 0; ++pass)
            for (Character &character : pParty->pCharacters)
                for (Attribute stat : allStatAttributes())
                    character.IncreaseAttribute(stat);
    });
    game.pressGuiButton("PartyCreation_OK");
    game.skipLoadingScreen();
    game.tick();
    game.runGameRoutine([&] { chestOptIn.reset(); });
    mm6::testing::setNewGameSaveOverride({});
    ASSERT_EQ(engine->_currentLoadedMapId, pMapTable->GetMapInfo("oute3.odm"));
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
}

static void startNewGameThroughInput(EngineController &game) {
    // Each character takes its first three optional skills and up to three points in every attribute, then the party
    // enters the first map. Nothing stands in for the new game's own save.
    constexpr PlatformKey characterKeys[] = {PlatformKey::KEY_DIGIT_1, PlatformKey::KEY_DIGIT_2, PlatformKey::KEY_DIGIT_3,
                                             PlatformKey::KEY_DIGIT_4};
    game.pressGuiButton("MainMenu_NewGame");
    game.tick(2);
    for (PlatformKey key : characterKeys) {
        game.pressAndReleaseKey(key);
        game.tick();
        for (int skill = 0; skill < 3; skill++) {
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
    game.pressGuiButton("PartyCreation_OK");
    game.skipLoadingScreen();
    game.tick();
}

static void talkAt(EngineController &game, const Vec3f &spot) {
    // Teleport next to a street NPC and talk to her with Space.
    game.teleportTo(engine->_currentLoadedMapId, spot, 0);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
}

static void talkToMarianne(EngineController &game) {
    talkAt(game, kMarianneSpot);
}

static void leaveConversation(EngineController &game) {
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
}

static std::vector<std::string> dialogueOptionLabels() {
    // Labels of the conversation window's topics, empty while no conversation is open.
    std::vector<std::string> result;
    if (pDialogueWindow)
        for (GUIButton *button : pDialogueWindow->vButtons)
            if (button->id.starts_with("NpcDialogue_Option"))
                result.push_back(button->label);
    return result;
}

static std::vector<DialogueId> dialogueOptionTopics() {
    // Topics of the conversation window's buttons, empty while no conversation is open.
    std::vector<DialogueId> result;
    if (pDialogueWindow)
        for (GUIButton *button : pDialogueWindow->vButtons)
            if (button->id.starts_with("NpcDialogue_Option"))
                result.push_back(static_cast<DialogueId>(button->msg_param));
    return result;
}

static void clickNpcOption(EngineController &game, std::string_view label) {
    // Left click in the middle of the conversation topic with this label.
    ASSERT_NE(pDialogueWindow, nullptr);
    std::optional<Pointi> center;
    for (GUIButton *button : pDialogueWindow->vButtons)
        if (button->id.starts_with("NpcDialogue_Option") && button->label == label)
            center = button->rect.center();
    ASSERT_TRUE(center) << "No conversation topic " << label;
    game.pressAndReleaseButton(BUTTON_LEFT, *center);
    game.tick();
}

static void captureFrame(EngineController &game, std::string_view prefix, std::string_view name) {
    RgbaImage frame;
    game.runGameRoutine([&frame] {
        render->ClearTarget(Color(0, 0, 0));
        engine->drawWorld();
        engine->drawHUD();
        render->flushAndScale();
        engine->drawOverlay();
        frame = render->MakeFullScreenshot();
    });
    FileOutputStream output{Path(fmt::format("{}-{}.png", prefix, name))};
    output.write(png::encode(frame));
    output.close();
}

static std::vector<Recti> dialogueButtonRects() {
    // Hit boxes of the conversation window's exit and topic buttons, empty while no conversation is open.
    std::vector<Recti> result;
    if (pDialogueWindow)
        for (GUIButton *button : pDialogueWindow->vButtons)
            if (button->id.starts_with("NpcDialogue_"))
                result.push_back(button->rect);
    return result;
}

GAME_TEST(MM6NpcDialogue, StreetNpcPanel) {
    // Talking to a street NPC drew MM7's conversation frame over MM6's right panel, with placeholders for the MM7 art
    // that MM6 doesn't have, and its topics and exit sat where MM7 puts them.
    auto screenTape = tapes.screen();
    auto texturesTape = tapes.hudTextures();
    auto textTape = tapes.allGUIWindowsText();
    auto rectsTape = tapes.custom([] { return dialogueButtonRects(); });
    enterNewSorpigal(game);
    test.startTaping();
    talkAt(game, kMariaSpot);
    clickNpcOption(game, "Join");
    clickNpcOption(game, "More Information");
    game.pressAndReleaseButton(BUTTON_LEFT, 560, 325); // The exit picture.
    game.tick();

    EXPECT_EQ(screenTape, tape(SCREEN_GAME, SCREEN_NPC_DIALOGUE, SCREEN_GAME));
    EXPECT_EQ(rectsTape, tape(std::vector<Recti>(),
                              std::vector<Recti>{{526, 313, 75, 33}, {480, 169, 140, 16}, {480, 216, 140, 16}, {480, 263, 140, 16}},
                              std::vector<Recti>{{526, 313, 75, 33}, {480, 185, 140, 32}, {480, 248, 140, 16}},
                              std::vector<Recti>()));
    auto textures = texturesTape.flatten();
    EXPECT_CONTAINS(textures, "evpan019");
    EXPECT_CONTAINS(textures, "npc149");
    EXPECT_CONTAINS(textures, "buttesc1");
    EXPECT_MISSES(textures, "evt02");
    EXPECT_MISSES(textures, "evtnpc");
    EXPECT_MISSES(textures, "ib-bcu-a");
    auto text = textTape.flatten();
    EXPECT_CONTAINS(text, "Maria the Scholar");
    EXPECT_CONTAINS(text, "More Information");
    EXPECT_CONTAINS(text, "Hire");
    EXPECT_CONTAINS(text, "Unlimited item Identification and a 5 percent bonus\non all experience gained.  Maria takes 5 percent\nof all gold you find.");
}

GAME_TEST(MM6NpcDialogue, StreetNpcHire) {
    // Clicking Hire where MM6 draws it asks the party for the street NPC's fee.
    auto hirelingsTape = tapes.totalHirelings();
    auto goldTape = tapes.gold();
    auto screenTape = tapes.screen();
    auto statusTape = tapes.statusBar();
    enterNewSorpigal(game);
    test.startTaping();
    talkAt(game, kMariaSpot);
    clickNpcOption(game, "Join");
    clickNpcOption(game, "Hire");

    EXPECT_EQ(hirelingsTape, tape(0));
    EXPECT_EQ(goldTape, tape(200));
    EXPECT_EQ(screenTape, tape(SCREEN_GAME, SCREEN_NPC_DIALOGUE));
    EXPECT_EQ(statusTape, tape("Rest", "", "You don't have enough gold"));
}

GAME_TEST(MM6NpcDialogue, StreetNpcEscape) {
    // One Escape closes a street NPC's conversation.
    auto screenTape = tapes.screen();
    enterNewSorpigal(game);
    test.startTaping();
    talkToMarianne(game);
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();

    EXPECT_EQ(screenTape, tape(SCREEN_GAME, SCREEN_NPC_DIALOGUE, SCREEN_GAME));
}

GAME_TEST(MM6NpcDialogue, NewGameAfterHire) {
    // Hiring a street NPC and then starting a new game kept the hireling's name behind a NUL, and the new game
    // couldn't be saved.
    startNewGameThroughInput(game);
    game.runGameRoutine([] { pParty->SetGold(10000, true); });
    auto hirelingsTape = tapes.totalHirelings();
    auto statusTape = tapes.statusBar();
    test.startTaping();

    talkToMarianne(game); // This party finds Noreen there, who talks to it.
    clickNpcOption(game, "Join");
    clickNpcOption(game, "Hire");
    game.goToMainMenu();
    startNewGameThroughInput(game);

    EXPECT_EQ(hirelingsTape, tape(0, 1, 0));
    EXPECT_MISSES(statusTape, "This party can't be saved yet");
}

GAME_TEST(MM6OffscreenFrame, DISABLED_NpcDialogueCapture) {
    // Hidden capture of a street NPC's conversation, before and after its first topic, and of the HUD once she's hired.
    ASSERT_FALSE(window->isVisible());
    ASSERT_EQ(dynamic_cast<NullRenderer *>(render), nullptr);
    const char *capturePrefix = std::getenv("MM6_OFFSCREEN_CAPTURE_PATH");
    ASSERT_NE(capturePrefix, nullptr);
    ASSERT_TRUE(std::string_view(capturePrefix).starts_with("/private/tmp/"));

    enterNewSorpigal(game);
    talkAt(game, kMariaSpot);
    ASSERT_EQ(current_screen_type, SCREEN_NPC_DIALOGUE);
    captureFrame(game, capturePrefix, "talk");
    clickNpcOption(game, "Join");
    clickNpcOption(game, "More Information");
    captureFrame(game, capturePrefix, "topic");
    game.runGameRoutine([] { pParty->SetGold(10000, true); });
    clickNpcOption(game, "Hire");
    captureFrame(game, capturePrefix, "hired");
}

GAME_TEST(MM6NpcDialogue, StreetNpcGreetingName) {
    // The party's name in a street NPC's greeting swallowed the full stop and the space after it.
    auto textTape = tapes.allGUIWindowsText();
    enterNewSorpigal(game);
    test.startTaping();
    talkAt(game, kMariaSpot);

    EXPECT_CONTAINS(textTape.flatten(), "Zoltan.  Charmed, I'm sure.");
}

GAME_TEST(MM6NpcDialogue, StreetNpcGreetingDeeds) {
    // Edison the Negotiator's greeting names the deeds the party is known for, and drawing it hit an assert.
    auto screenTape = tapes.screen();
    auto textTape = tapes.allGUIWindowsText();
    enterNewSorpigal(game);
    test.startTaping();
    talkAt(game, kEdisonSpot);

    EXPECT_EQ(screenTape, tape(SCREEN_GAME, SCREEN_NPC_DIALOGUE));
    EXPECT_CONTAINS(textTape.flatten(), "Sorry, sir but it doesn't matter to me if you\nperformed daring deeds.  As a matter of fact, I\n"
                                        "consider your holier-than-thou attitude quite\ninsulting\x85" "sir.");
}

GAME_TEST(MM6NpcDialogue, StreetNpcBeg) {
    // Marianne the Teacher turns away a party without a good name, and offered nothing to change her mind. Begging
    // works on a scholar: it costs 10 reputation, she answers, and her usual topics replace Beg, Threat and Bribe.
    auto goldTape = tapes.gold();
    auto reputationTape = tapes.custom([] { return pParty->mm6Reputation; });
    auto dialogueTape = tapes.dialogueType();
    auto topicsTape = tapes.custom([] { return dialogueOptionTopics(); });
    auto textTape = tapes.allGUIWindowsText();
    auto portraitTape = charTapes.portrait(0);
    enterNewSorpigal(game);
    test.startTaping();
    talkToMarianne(game);
    clickNpcOption(game, "Beg");

    EXPECT_EQ(goldTape, tape(200));
    EXPECT_EQ(reputationTape, tape(0, -10));
    EXPECT_EQ(dialogueTape, tape(DIALOGUE_NULL, DIALOGUE_MAIN, DIALOGUE_BEG));
    EXPECT_EQ(topicsTape, tape(std::vector<DialogueId>(), std::vector<DialogueId>{DIALOGUE_BEG, DIALOGUE_THREAT, DIALOGUE_BRIBE},
                               std::vector<DialogueId>{DIALOGUE_WEEKDAY_TOPIC, DIALOGUE_13_hiring_related, DIALOGUE_NEWS}));
    auto text = textTape.flatten();
    EXPECT_CONTAINS(text, "People have been talking about how you like to\ncommit crimes, Zoltan.  I don't like how that\nsounds, so I don't want to talk to you.");
    EXPECT_CONTAINS(text, "Fine, but make it quick.");
    EXPECT_EQ(portraitTape, tape(PORTRAIT_NORMAL, PORTRAIT_SMILE, PORTRAIT_44)); // The greeting, then the beg.
}

GAME_TEST(MM6NpcDialogue, StreetNpcBegAgain) {
    // A street NPC who was begged once greets the party coldly next time, and begging again only gets that greeting.
    auto reputationTape = tapes.custom([] { return pParty->mm6Reputation; });
    auto dialogueTape = tapes.dialogueType();
    auto optionsTape = tapes.custom([] { return dialogueOptionLabels(); });
    auto textTape = tapes.allGUIWindowsText();
    enterNewSorpigal(game);
    test.startTaping();
    talkToMarianne(game);
    clickNpcOption(game, "Beg");
    leaveConversation(game);
    talkToMarianne(game);
    clickNpcOption(game, "Beg");

    EXPECT_EQ(reputationTape, tape(0, -10));
    EXPECT_EQ(dialogueTape, tape(DIALOGUE_NULL, DIALOGUE_MAIN, DIALOGUE_BEG, DIALOGUE_NULL, DIALOGUE_MAIN, DIALOGUE_NULL));
    EXPECT_EQ(optionsTape.back(), (std::vector<std::string>{"Beg", "Threat", "Bribe 50 Gold"}));
    EXPECT_CONTAINS(textTape.flatten(), "No.  I won't let you waste any more of my time. \nPerhaps tomorrow.");
}

GAME_TEST(MM6NpcDialogue, StreetNpcBegRefused) {
    // Edison the Negotiator won't listen to begging. He says so, and the party's reputation stays.
    auto reputationTape = tapes.custom([] { return pParty->mm6Reputation; });
    auto dialogueTape = tapes.dialogueType();
    auto optionsTape = tapes.custom([] { return dialogueOptionLabels(); });
    auto textTape = tapes.allGUIWindowsText();
    auto portraitTape = charTapes.portrait(0);
    enterNewSorpigal(game);
    test.startTaping();
    talkAt(game, kEdisonSpot);
    clickNpcOption(game, "Beg");

    EXPECT_EQ(reputationTape, tape(0));
    EXPECT_EQ(dialogueTape, tape(DIALOGUE_NULL, DIALOGUE_MAIN, DIALOGUE_BEG));
    EXPECT_EQ(optionsTape, tape(std::vector<std::string>(), std::vector<std::string>{"Beg", "Threat", "Bribe 50 Gold"}));
    EXPECT_CONTAINS(textTape.flatten(), "Beg all you like.  I still won't help you.");
    EXPECT_EQ(portraitTape, tape(PORTRAIT_NORMAL, PORTRAIT_SMILE, PORTRAIT_48)); // The greeting, then the failed beg.
}

GAME_TEST(MM6NpcDialogue, StreetNpcThreat) {
    // Threatening Marianne costs 50 reputation, and she talks from then on, with a frightened greeting.
    auto goldTape = tapes.gold();
    auto reputationTape = tapes.custom([] { return pParty->mm6Reputation; });
    auto dialogueTape = tapes.dialogueType();
    auto topicsTape = tapes.custom([] { return dialogueOptionTopics(); });
    auto textTape = tapes.allGUIWindowsText();
    auto portraitTape = charTapes.portrait(0);
    enterNewSorpigal(game);
    test.startTaping();
    talkToMarianne(game);
    clickNpcOption(game, "Threat");
    leaveConversation(game);
    talkToMarianne(game);

    EXPECT_EQ(goldTape, tape(200));
    EXPECT_EQ(reputationTape, tape(0, -50));
    EXPECT_EQ(dialogueTape, tape(DIALOGUE_NULL, DIALOGUE_MAIN, DIALOGUE_THREAT, DIALOGUE_NULL, DIALOGUE_MAIN));
    EXPECT_EQ(topicsTape, tape(std::vector<DialogueId>(), std::vector<DialogueId>{DIALOGUE_BEG, DIALOGUE_THREAT, DIALOGUE_BRIBE},
                               std::vector<DialogueId>{DIALOGUE_WEEKDAY_TOPIC, DIALOGUE_13_hiring_related, DIALOGUE_NEWS},
                               std::vector<DialogueId>(),
                               std::vector<DialogueId>{DIALOGUE_WEEKDAY_TOPIC, DIALOGUE_13_hiring_related, DIALOGUE_NEWS}));
    auto text = textTape.flatten();
    EXPECT_CONTAINS(text, "Understood sir.  I want to live.  What do you need?");
    EXPECT_CONTAINS(text, "Now what?  Have you returned to slay me?  I will\nsurely call out your name before you strike me\ndown!  Someone will come to save me!");
    EXPECT_EQ(portraitTape, tape(PORTRAIT_NORMAL, PORTRAIT_SMILE, PORTRAIT_WIDE_SMILE, PORTRAIT_53)); // The greeting, the threat, then the greeting below zero reputation.
}

GAME_TEST(MM6NpcDialogue, StreetNpcThreatRefused) {
    // Edison the Negotiator won't take a threat. He says so, and the party's reputation stays.
    auto reputationTape = tapes.custom([] { return pParty->mm6Reputation; });
    auto dialogueTape = tapes.dialogueType();
    auto optionsTape = tapes.custom([] { return dialogueOptionLabels(); });
    auto textTape = tapes.allGUIWindowsText();
    auto portraitTape = charTapes.portrait(0);
    enterNewSorpigal(game);
    test.startTaping();
    talkAt(game, kEdisonSpot);
    clickNpcOption(game, "Threat");

    EXPECT_EQ(reputationTape, tape(0));
    EXPECT_EQ(dialogueTape, tape(DIALOGUE_NULL, DIALOGUE_MAIN, DIALOGUE_THREAT));
    EXPECT_EQ(optionsTape, tape(std::vector<std::string>(), std::vector<std::string>{"Beg", "Threat", "Bribe 50 Gold"}));
    EXPECT_CONTAINS(textTape.flatten(), "A threat?  You had best be gone before I set the\nwatch on you!");
    EXPECT_EQ(portraitTape, tape(PORTRAIT_NORMAL, PORTRAIT_SMILE, PORTRAIT_42)); // The greeting, then the failed threat.
}

GAME_TEST(MM6NpcDialogue, StreetNpcBribe) {
    // Bribing Edison the Negotiator takes 50 gold and 20 reputation, and he talks.
    auto goldTape = tapes.gold();
    auto reputationTape = tapes.custom([] { return pParty->mm6Reputation; });
    auto dialogueTape = tapes.dialogueType();
    auto topicsTape = tapes.custom([] { return dialogueOptionTopics(); });
    auto textTape = tapes.allGUIWindowsText();
    auto portraitTape = charTapes.portrait(0);
    enterNewSorpigal(game);
    test.startTaping();
    talkAt(game, kEdisonSpot);
    clickNpcOption(game, "Bribe 50 Gold");

    EXPECT_EQ(goldTape, tape(200, 150));
    EXPECT_EQ(reputationTape, tape(0, -20));
    EXPECT_EQ(dialogueTape, tape(DIALOGUE_NULL, DIALOGUE_MAIN, DIALOGUE_BRIBE));
    EXPECT_EQ(topicsTape, tape(std::vector<DialogueId>(), std::vector<DialogueId>{DIALOGUE_BEG, DIALOGUE_THREAT, DIALOGUE_BRIBE},
                               std::vector<DialogueId>{DIALOGUE_WEEKDAY_TOPIC, DIALOGUE_13_hiring_related, DIALOGUE_NEWS}));
    EXPECT_CONTAINS(textTape.flatten(), "Thank you.  How may I be of service?");
    EXPECT_EQ(portraitTape, tape(PORTRAIT_NORMAL, PORTRAIT_SMILE, PORTRAIT_47)); // The greeting, then the bribe.
}

GAME_TEST(MM6NpcDialogue, StreetNpcBribeAgain) {
    // A bribed NPC wants a bribe every time, and every bribe the party pays makes the next one cost 50 gold more.
    // When the party can't pay, the NPC greets it again and the gold stays.
    auto goldTape = tapes.gold();
    auto reputationTape = tapes.custom([] { return pParty->mm6Reputation; });
    auto dialogueTape = tapes.dialogueType();
    auto optionsTape = tapes.custom([] { return dialogueOptionLabels(); });
    auto statusTape = tapes.statusBar();
    auto textTape = tapes.allGUIWindowsText();
    enterNewSorpigal(game);
    test.startTaping();
    talkAt(game, kEdisonSpot);
    clickNpcOption(game, "Bribe 50 Gold");
    leaveConversation(game);
    talkAt(game, kEdisonSpot);
    clickNpcOption(game, "Bribe 100 Gold");
    leaveConversation(game);
    talkAt(game, kEdisonSpot);
    clickNpcOption(game, "Bribe 150 Gold");

    EXPECT_EQ(goldTape, tape(200, 150, 50));
    EXPECT_EQ(reputationTape, tape(0, -20, -40));
    EXPECT_EQ(dialogueTape.back(), DIALOGUE_NULL);
    EXPECT_EQ(optionsTape.back(), (std::vector<std::string>{"Beg", "Threat", "Bribe 150 Gold"}));
    EXPECT_EQ(statusTape.back(), "You don't have enough gold");
    auto text = textTape.flatten();
    EXPECT_CONTAINS(text, "I'm afraid I will require a second payment, sir. \n100 now, or the conversation ends here.");
    EXPECT_CONTAINS(text, "I'm afraid I will require a second payment, sir. \n150 now, or the conversation ends here.");
}

GAME_TEST(MM6NpcDialogue, StreetNpcBribeRefused) {
    // Marianne the Teacher can't be bought. She says so, and the party keeps its gold and reputation.
    auto goldTape = tapes.gold();
    auto reputationTape = tapes.custom([] { return pParty->mm6Reputation; });
    auto dialogueTape = tapes.dialogueType();
    auto optionsTape = tapes.custom([] { return dialogueOptionLabels(); });
    auto textTape = tapes.allGUIWindowsText();
    auto portraitTape = charTapes.portrait(0);
    enterNewSorpigal(game);
    test.startTaping();
    talkToMarianne(game);
    clickNpcOption(game, "Bribe 50 Gold");

    EXPECT_EQ(goldTape, tape(200));
    EXPECT_EQ(reputationTape, tape(0));
    EXPECT_EQ(dialogueTape, tape(DIALOGUE_NULL, DIALOGUE_MAIN, DIALOGUE_BRIBE));
    EXPECT_EQ(optionsTape, tape(std::vector<std::string>(), std::vector<std::string>{"Beg", "Threat", "Bribe 50 Gold"}));
    EXPECT_CONTAINS(textTape.flatten(), "Money means little.  Knowledge is power to me.  ");
    EXPECT_EQ(portraitTape, tape(PORTRAIT_NORMAL, PORTRAIT_SMILE, PORTRAIT_48)); // The greeting, then the failed bribe.
}


GAME_TEST(MM6NpcDialogue, StreetNpcBribeDiplomacy) {
    // The diplomacy of the character who talks lowers the price of a bribe and the reputation it costs.
    auto goldTape = tapes.gold();
    auto reputationTape = tapes.custom([] { return pParty->mm6Reputation; });
    auto dialogueTape = tapes.dialogueType();
    enterNewSorpigal(game);
    game.runGameRoutine([] { pParty->pCharacters[0].pActiveSkills[SKILL_DIPLOMACY] = CombinedSkillValue(2, MASTERY_EXPERT); });
    test.startTaping();
    talkAt(game, kEdisonSpot);
    clickNpcOption(game, "Bribe 47 Gold");

    EXPECT_EQ(goldTape, tape(200, 153));
    EXPECT_EQ(reputationTape, tape(0, -14));
    EXPECT_EQ(dialogueTape, tape(DIALOGUE_NULL, DIALOGUE_MAIN, DIALOGUE_BRIBE));
}

GAME_TEST(MM6NpcDialogue, StreetNpcNewsAfterNewGame) {
    // A New Sorpigal villager of a new game told Castle Ironfist's news, the news a villager of the last game had
    // picked when the party talked to her there.
    auto mapTape = tapes.map();
    auto optionsTape = tapes.custom([] { return dialogueOptionLabels(); });
    enterNewSorpigal(game);
    test.startTaping();
    game.teleportTo(pMapTable->GetMapInfo("outd3.odm"), kCastleIronfistVillagerSpot, 0);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE); // Cora, the villager there.
    game.tick();
    leaveConversation(game);
    game.goToMainMenu();
    enterNewSorpigal(game);
    talkToMarianne(game); // Philomena stands there in this game.

    EXPECT_EQ(mapTape, tape(pMapTable->GetMapInfo("oute3.odm"), pMapTable->GetMapInfo("outd3.odm"), pMapTable->GetMapInfo("oute3.odm")));
    EXPECT_EQ(optionsTape, tape(std::vector<std::string>(), std::vector<std::string>{"Beg", "Threat", "Bribe 50 Gold"}, std::vector<std::string>(),
                                std::vector<std::string>{"Magic Weapons", "Join", "Abandoned Temple"}));
}

GAME_TEST(MM6NpcDialogue, GreetingSpeech) {
    // The character who starts a conversation greeted by the hour of the day. MM6 greets by the party's reputation,
    // with the other greeting at any hour once it drops below zero.
    auto speechTape = tapes.speeches();
    auto reputationTape = tapes.custom([] { return pParty->mm6Reputation; });
    auto screenTape = tapes.screen();
    auto activeCharacterTape = tapes.activeCharacterIndex();
    auto talkerPortraitTape = charTapes.portrait(1);
    auto otherPortraitTape = charTapes.portrait(0);
    enterNewSorpigal(game);
    test.startTaping();
    game.pressAndReleaseKey(PlatformKey::KEY_DIGIT_2);
    game.tick();
    for (int reputation : {0, -1}) {
        game.runGameRoutine([reputation] { pParty->mm6Reputation = reputation; });
        talkToMarianne(game);
        leaveConversation(game);
    }

    EXPECT_EQ(speechTape.flatten(), tape(SPEECH_GOOD_DAY, SPEECH_GOOD_EVENING));
    EXPECT_EQ(reputationTape, tape(0, -1));
    EXPECT_EQ(screenTape, tape(SCREEN_GAME, SCREEN_NPC_DIALOGUE, SCREEN_GAME, SCREEN_NPC_DIALOGUE, SCREEN_GAME));
    EXPECT_EQ(activeCharacterTape, tape(1));
    EXPECT_EQ(talkerPortraitTape, tape(PORTRAIT_NORMAL, PORTRAIT_BLINK, PORTRAIT_WIDE_SMILE, PORTRAIT_53)); // A blink, then the two greetings.
    EXPECT_EQ(otherPortraitTape, tape(PORTRAIT_NORMAL));
}

GAME_TEST(MM6NpcDialogue, StreetNpcVoices) {
    // Characters spoke MM6 speeches with MM7's voice lines, and begging had no voice.
    auto speechTape = tapes.speeches();
    auto soundsTape = tapes.sounds();
    auto activeCharacterTape = tapes.activeCharacterIndex();
    auto reputationTape = tapes.custom([] { return pParty->mm6Reputation; });
    enterNewSorpigal(game);
    test.startTaping();
    talkToMarianne(game);
    game.pressAndReleaseKey(PlatformKey::KEY_DIGIT_2); // The first character is still saying the greeting, so the second one begs.
    game.tick();
    clickNpcOption(game, "Beg");
    leaveConversation(game);
    game.pressAndReleaseKey(PlatformKey::KEY_DIGIT_3);
    game.tick();
    talkToMarianne(game);

    auto voices = soundsTape.flatten().filter([](SoundId sound) { return sound >= static_cast<SoundId>(5000) && sound < static_cast<SoundId>(6200); });
    EXPECT_EQ(speechTape.flatten(), tape(SPEECH_GOOD_DAY, SPEECH_BEG, SPEECH_GOOD_EVENING));
    EXPECT_EQ(voices, tape(static_cast<SoundId>(5062), static_cast<SoundId>(5182), static_cast<SoundId>(5808))); // MaleA31a, MaleB41a, GirlA04a.
    EXPECT_EQ(activeCharacterTape, tape(0, 1, 2));
    EXPECT_EQ(reputationTape, tape(0, -10));
}
