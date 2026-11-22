#include <initializer_list>
#include <optional>
#include <regex>
#include <string>
#include <string_view>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineCallObserver.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/CombinedSkillValue.h"
#include "Engine/Party.h"

#include "GUI/GUIButton.h"
#include "GUI/GUIWindow.h"

static void startNewGameThroughInput(EngineController &game) {
    // Each character takes its first three optional skills and up to three points in every attribute, then the party
    // enters the first map. The party is a Knight, a Cleric, a Sorcerer and a Druid with nothing equipped.
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

static void wearFromBackpack(EngineController &game, ItemId item, Pointi paperdoll = {520, 150}) {
    // The knight picks the item up in the backpack and drops it on the paperdoll. A dagger dropped on its right half
    // goes to the off hand.
    game.goToInventory(0);
    InventoryEntry entry = pParty->pCharacters[0].inventory.find(item);
    ASSERT_TRUE(entry);
    game.pressAndReleaseButton(BUTTON_LEFT, Pointi(14, 17) + entry.geometry().topLeft() * 32 + Pointi(16, 16));
    game.tick();
    game.pressAndReleaseButton(BUTTON_LEFT, paperdoll);
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
}

static void viewStats(EngineController &game) {
    // Opens the knight's stats tab and closes the character screen again.
    game.goToInventory(0);
    game.pressGuiButton("CharacterWindow_Stats");
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
}

static void talkToStreetNpc(EngineController &game, int actorId) {
    // Stand next to the street NPC, facing east towards her, and click on her.
    game.teleportTo(engine->_currentLoadedMapId, pActors[actorId].pos - Vec3f(150, 0, 0), 0);
    game.pointMouseAtActor(actorId);
    game.pressAndReleaseButton(BUTTON_LEFT);
    game.tick();
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

static void hireArmsMaster(EngineController &game) {
    game.runGameRoutine([] { pParty->SetGold(10000, true); });
    talkToStreetNpc(game, 8); // Deloris the Arms Master.
    clickNpcOption(game, "Beg"); // A Paladin talks to a party of no reputation only once it begs.
    clickNpcOption(game, "Join");
    clickNpcOption(game, "Hire");
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
}

static std::string shownRows(const AccessibleVector<DrawnText> &frame, std::initializer_list<std::string_view> labels) {
    // The stats tab rows with these labels drawn in a frame, without colors and tab stops, in drawing order.
    static const std::regex colors("\f[0-9]{5}");
    static const std::regex stops("[\t\r][0-9]{3}");
    static const std::regex spaces("\\s+");
    std::string result;
    for (const DrawnText &text : frame) {
        for (std::string_view label : labels) {
            if (!text.text.starts_with(label))
                continue;
            std::string row = std::regex_replace(std::regex_replace(text.text, colors, ""), stops, " ");
            row = std::regex_replace(row, spaces, " ");
            while (row.ends_with(' '))
                row.pop_back();
            result += result.empty() ? row : ", " + row;
        }
    }
    return result;
}

static auto statsRows(TestMultiTape<DrawnText> &textTape, std::initializer_list<std::string_view> labels) {
    return textTape.map([labels](const AccessibleVector<DrawnText> &frame) { return shownRows(frame, labels); })
        .filter([](const std::string &rows) { return !rows.empty(); }).unique();
}

static void learnSkill(Skill skill, Mastery mastery) {
    pParty->pCharacters[0].setSkillValue(skill, CombinedSkillValue(4, mastery));
}

GAME_TEST(MM6SkillBonus, SpearDamageAndArmorClassFollowMastery) {
    // An Expert spear added its skill to damage and Expert spear gave no armor class, as in MM7. MM6 adds the spear
    // skill to armor class from Expert and to damage at Master.
    startNewGameThroughInput(game);
    learnSkill(SKILL_SPEAR, MASTERY_NOVICE);
    pParty->pCharacters[0].inventory.add(Item(static_cast<ItemId>(31))); // Spear.
    wearFromBackpack(game, static_cast<ItemId>(31));
    auto textTape = tapes.drawnTexts();
    auto masteryTape = charTapes.custom(0, [](const Character &character) { return character.getSkillValue(SKILL_SPEAR).mastery(); });
    test.startTaping();

    for (Mastery mastery : {MASTERY_NOVICE, MASTERY_EXPERT, MASTERY_MASTER}) {
        learnSkill(SKILL_SPEAR, mastery);
        viewStats(game);
    }

    EXPECT_EQ(masteryTape, tape(MASTERY_NOVICE, MASTERY_EXPERT, MASTERY_MASTER));
    EXPECT_EQ(statsRows(textTape, {"Armor Class", "Attack", "Damage"}),
              tape("Armor Class 0 / 0, Attack +4, Damage 4 - 20, Damage N/A",
                   "Armor Class 4 / 4, Attack +4, Damage 4 - 20, Damage N/A",
                   "Armor Class 4 / 4, Attack +4, Damage 8 - 24, Damage N/A"));
}

GAME_TEST(MM6SkillBonus, ShieldArmorClassFollowsMastery) {
    // The shield skill added to armor class once at Novice and Expert and twice at Master, as in MM7. MM6 adds it once,
    // twice and three times.
    startNewGameThroughInput(game);
    learnSkill(SKILL_SHIELD, MASTERY_NOVICE);
    pParty->pCharacters[0].inventory.add(Item(static_cast<ItemId>(85))); // Bronze Shield.
    wearFromBackpack(game, static_cast<ItemId>(85));
    auto textTape = tapes.drawnTexts();
    auto masteryTape = charTapes.custom(0, [](const Character &character) { return character.getSkillValue(SKILL_SHIELD).mastery(); });
    test.startTaping();

    for (Mastery mastery : {MASTERY_NOVICE, MASTERY_EXPERT, MASTERY_MASTER}) {
        learnSkill(SKILL_SHIELD, mastery);
        viewStats(game);
    }

    EXPECT_EQ(masteryTape, tape(MASTERY_NOVICE, MASTERY_EXPERT, MASTERY_MASTER));
    EXPECT_EQ(statsRows(textTape, {"Armor Class"}), tape("Armor Class 10 / 10", "Armor Class 14 / 14", "Armor Class 18 / 18"));
}

GAME_TEST(MM6SkillBonus, LeatherArmorClassIgnoresMastery) {
    // Master leather added its skill to armor class twice, as in MM7. MM6 adds it once at every mastery.
    startNewGameThroughInput(game);
    learnSkill(SKILL_LEATHER, MASTERY_NOVICE);
    pParty->pCharacters[0].inventory.add(Item(static_cast<ItemId>(66))); // Leather Armor.
    wearFromBackpack(game, static_cast<ItemId>(66));
    auto textTape = tapes.drawnTexts();
    auto masteryTape = charTapes.custom(0, [](const Character &character) { return character.getSkillValue(SKILL_LEATHER).mastery(); });
    test.startTaping();

    for (Mastery mastery : {MASTERY_NOVICE, MASTERY_EXPERT, MASTERY_MASTER}) {
        learnSkill(SKILL_LEATHER, mastery);
        viewStats(game);
    }

    EXPECT_EQ(masteryTape, tape(MASTERY_NOVICE, MASTERY_EXPERT, MASTERY_MASTER));
    EXPECT_EQ(statsRows(textTape, {"Armor Class"}), tape("Armor Class 8 / 8"));
}

GAME_TEST(MM6SkillBonus, BlasterAttackFollowsMastery) {
    // The blaster skill added to attack once, twice and three times by mastery, as in MM7. MM6.exe adds it once at
    // Novice and Expert and three times at Master, and a hired Arms Master's two points are multiplied with it.
    engine->config->gameplay.FixExpertBlaster.setValue(false);
    startNewGameThroughInput(game);
    learnSkill(SKILL_BLASTER, MASTERY_NOVICE);
    pParty->pCharacters[0].inventory.add(Item(static_cast<ItemId>(64))); // Blaster.
    wearFromBackpack(game, static_cast<ItemId>(64));
    auto textTape = tapes.drawnTexts();
    auto masteryTape = charTapes.custom(0, [](const Character &character) { return character.getSkillValue(SKILL_BLASTER).mastery(); });
    auto hirelingsTape = tapes.totalHirelings();
    test.startTaping();

    for (bool hired : {false, true}) {
        if (hired)
            hireArmsMaster(game);
        for (Mastery mastery : {MASTERY_NOVICE, MASTERY_EXPERT, MASTERY_MASTER}) {
            learnSkill(SKILL_BLASTER, mastery);
            viewStats(game);
        }
    }

    EXPECT_EQ(hirelingsTape, tape(0, 1));
    EXPECT_EQ(masteryTape, tape(MASTERY_NOVICE, MASTERY_EXPERT, MASTERY_MASTER, MASTERY_NOVICE, MASTERY_EXPERT, MASTERY_MASTER));
    EXPECT_EQ(statsRows(textTape, {"Attack", "Shoot"}),
              tape("Attack +12, Shoot +12", "Attack +20, Shoot +20", "Attack +14, Shoot +14", "Attack +26, Shoot +26"));
}

GAME_TEST(MM6SkillBonus, ExpertBlasterDoublesAttack) {
    // An Expert blaster added its skill to attack once, where its description promises double effect. With
    // fix_expert_blaster it adds it twice, while Novice and Master stay at once and three times.
    for (bool fix : {true, false}) {
        SCOPED_TRACE(fmt::format("fix={}", fix));
        test.prepareForNextTest();
        engine->config->gameplay.FixExpertBlaster.setValue(fix);
        startNewGameThroughInput(game);
        learnSkill(SKILL_BLASTER, MASTERY_NOVICE);
        pParty->pCharacters[0].inventory.add(Item(static_cast<ItemId>(64))); // Blaster.
        wearFromBackpack(game, static_cast<ItemId>(64));
        auto textTape = tapes.drawnTexts();
        auto masteryTape = charTapes.custom(0, [](const Character &character) { return character.getSkillValue(SKILL_BLASTER).mastery(); });
        test.startTaping();

        for (Mastery mastery : {MASTERY_NOVICE, MASTERY_EXPERT, MASTERY_MASTER}) {
            learnSkill(SKILL_BLASTER, mastery);
            viewStats(game);
        }

        EXPECT_EQ(masteryTape, tape(MASTERY_NOVICE, MASTERY_EXPERT, MASTERY_MASTER));
        if (fix) {
            EXPECT_EQ(statsRows(textTape, {"Attack", "Shoot"}), tape("Attack +12, Shoot +12", "Attack +16, Shoot +16", "Attack +20, Shoot +20"));
        } else {
            EXPECT_EQ(statsRows(textTape, {"Attack", "Shoot"}), tape("Attack +12, Shoot +12", "Attack +20, Shoot +20"));
        }
    }
}

GAME_TEST(MM6SkillBonus, OffHandDaggerLeavesAxeDamage) {
    // A dagger in the off hand took away the damage bonus of a Master axe in the main hand. MM6 passes over every
    // weapon but an axe, spear or mace for the damage bonus, while the attack bonus comes from the off hand weapon.
    startNewGameThroughInput(game);
    pParty->pCharacters[0].setSkillValue(SKILL_AXE, CombinedSkillValue(4, MASTERY_MASTER));
    pParty->pCharacters[0].setSkillValue(SKILL_DAGGER, CombinedSkillValue(3, MASTERY_EXPERT));
    pParty->pCharacters[0].inventory.add(Item(static_cast<ItemId>(23))); // Hand Axe.
    pParty->pCharacters[0].inventory.add(Item(static_cast<ItemId>(15))); // Dagger.
    auto textTape = tapes.drawnTexts();
    auto offHandTape = charTapes.custom(0, [](const Character &character) {
        InventoryConstEntry entry = character.inventory.entry(ITEM_SLOT_OFF_HAND);
        return entry ? entry->itemId : ITEM_NULL;
    });
    test.startTaping();

    wearFromBackpack(game, static_cast<ItemId>(23));
    viewStats(game);
    wearFromBackpack(game, static_cast<ItemId>(15), {580, 150});
    viewStats(game);

    EXPECT_EQ(offHandTape, tape(ITEM_NULL, static_cast<ItemId>(15)));
    EXPECT_EQ(statsRows(textTape, {"Attack", "Damage"}), tape("Attack +4, Damage 10 - 14, Damage N/A", "Attack +3, Damage 12 - 18, Damage N/A"));
}
