#include <cstdint>
#include <map>
#include <regex>
#include <string>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Localization.h"
#include "Engine/MM6/MM6ChestTitleState.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Objects/CombinedSkillValue.h"
#include "Engine/Party.h"
#include "Engine/Tables/ChestTable.h"
#include "Engine/Tables/MapTable.h"

#include "Utility/Math/TrigLut.h"

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

static void takeFromHiveChest4(EngineController &game, ItemId item, int characterIndex) {
    // The party opens the hive's chest 4 and puts the item into a character's backpack.
    game.teleportTo(pMapTable->GetMapInfo("hive.blv"), Vec3f(-3000, 4182, -1777), 0, -20); // West of chest 4, looking down at it.
    game.pointMouseAtFace(2960);
    game.pressAndReleaseButton(BUTTON_LEFT);
    game.tick();
    InventoryEntry entry = vChests[4].inventory.find(item);
    ASSERT_TRUE(entry);
    game.pressAndReleaseButton(BUTTON_LEFT, chestTable[vChests[4].chestTypeId].inventoryOffset + entry.geometry().topLeft() * 32 + Pointi(16, 16));
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    game.pressGuiButton(fmt::format("Game_Character{}", characterIndex + 1));
    game.tick();
}

static void wearFromBackpack(EngineController &game, int characterIndex, ItemId item) {
    // The character picks the item up in the backpack and drops it on the paperdoll.
    game.goToInventory(characterIndex);
    InventoryEntry entry = pParty->pCharacters[characterIndex].inventory.find(item);
    ASSERT_TRUE(entry);
    game.pressAndReleaseButton(BUTTON_LEFT, Pointi(14, 17) + entry.geometry().topLeft() * 32 + Pointi(16, 16));
    game.tick();
    game.pressAndReleaseButton(BUTTON_LEFT, {520, 150}); // The paperdoll.
    game.tick();
}

static void showStats(EngineController &game, int characterIndex) {
    // Opens the character screen of the character on its stats tab.
    game.goToInventory(characterIndex);
    game.pressGuiButton("CharacterWindow_Stats");
    game.tick();
}

using ShownStats = std::map<std::string, std::pair<int, int>>;

static ShownStats shownStats(const std::vector<std::string> &frame) {
    // The rows of the stats tab drawn in a frame, each with its current and its base or maximum value.
    static const std::regex colors("\f[0-9]{5}");
    static const std::regex row("^([A-Za-z ]+)[\r\t][0-9]{3}(-?[0-9]+) /(?:\t[0-9]{3}| )(-?[0-9]+)\n?$");
    ShownStats result;
    for (const std::string &line : frame) {
        const std::string plain = std::regex_replace(line, colors, "");
        std::smatch match;
        if (std::regex_match(plain, match, row))
            result[match[1]] = {std::stoi(match[2]), std::stoi(match[3])};
    }
    if (!result.contains(localization->str(LSTR_MIGHT)))
        return {};
    return result;
}

struct ArtifactCase {
    std::uint32_t luck; // Chest luck that rolls the item into the hive's chest 4.
    int item;
    int character;
    Skill skill; // Skill the wearer needs and lacks, or SKILL_INVALID.
    std::vector<std::pair<LstrId, int>> changes; // How much each stats tab row moves, both of its values.
    std::vector<LstrId> derived; // Rows that also move through a changed stat or the item's armor class.
};

static void checkArtifactCases(EngineController &game, TestController &test, CommonTapeRecorder &tapes, const std::vector<ArtifactCase> &cases) {
    // For each chest luck, a new party takes its items from the chest, and each wearer looks at the stats tab before
    // and after putting the item on.
    for (size_t first = 0; first < cases.size();) {
        size_t last = first;
        while (last < cases.size() && cases[last].luck == cases[first].luck)
            last++;
        test.prepareForNextTest();
        auto textTape = tapes.allGUIWindowsText();
        startNewGameThroughInput(game);
        test.startTaping();

        mm6::chestTitleState().lcgState = cases[first].luck;
        for (size_t i = first; i < last; i++) {
            if (cases[i].skill != SKILL_INVALID)
                pParty->pCharacters[cases[i].character].setSkillValue(cases[i].skill, CombinedSkillValue::novice());
            takeFromHiveChest4(game, static_cast<ItemId>(cases[i].item), cases[i].character);
        }
        for (size_t i = first; i < last; i++) {
            showStats(game, cases[i].character);
            wearFromBackpack(game, cases[i].character, static_cast<ItemId>(cases[i].item));
            game.pressGuiButton("CharacterWindow_Stats");
            game.tick();
            game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
            game.tick();
        }

        auto shown = textTape.map(shownStats).filter([](const ShownStats &stats) { return !stats.empty(); });
        ASSERT_EQ(shown.size(), 2 * (last - first));
        for (size_t i = first; i < last; i++) {
            const ArtifactCase &c = cases[i];
            SCOPED_TRACE(fmt::format("item {}", c.item));
            const ShownStats &before = shown[2 * (i - first)];
            ShownStats expected = before;
            for (auto [row, delta] : c.changes) {
                auto &[current, base] = expected[localization->str(row)];
                if (row != LSTR_HIT_POINTS && row != LSTR_SPELL_POINTS)
                    current += delta; // The current hit and spell points stay as they are.
                base += delta;
            }
            ShownStats after = shown[2 * (i - first) + 1];
            for (LstrId row : c.derived)
                after[localization->str(row)] = expected[localization->str(row)];
            EXPECT_EQ(after, expected);
        }
        first = last;
    }
}

GAME_TEST(MM6ArtifactBonus, ArtifactsRaiseStats) {
    // Artifacts gave none of their stat, hit point, spell point or resistance bonuses. A character who puts one on
    // sees it on the stats tab.
    const std::vector<ArtifactCase> cases = {
        {687, 401, 1, SKILL_INVALID, {{LSTR_MIGHT, 20}}, {}}, // Thor.
        {353, 402, 0, SKILL_INVALID, {{LSTR_MIGHT, 20}}, {}}, // Conan.
        {257, 403, 0, SKILL_INVALID, {{LSTR_MIGHT, 30}}, {}}, // Excalibur.
        {808, 404, 2, SKILL_INVALID, {{LSTR_SPELL_POINTS, 40}}, {}}, // Merlin.
        {485, 406, 0, SKILL_CHAIN, {{LSTR_HIT_POINTS, 25}, {LSTR_FIRE, 10}, {LSTR_ELECTRICITY, 10}, {LSTR_COLD, 10}, {LSTR_POISON, 10}},
         {LSTR_ARMOR_CLASS}}, // Galahad.
        {907, 407, 0, SKILL_PLATE, {{LSTR_ENDURANCE, 30}}, {LSTR_HIT_POINTS, LSTR_ARMOR_CLASS}}, // Pellinore.
        {36, 408, 1, SKILL_INVALID, {{LSTR_ACCURACY, 30}}, {LSTR_ARMOR_CLASS}}, // Valeria.
        {552, 409, 1, SKILL_INVALID, {{LSTR_MIGHT, 10}, {LSTR_INTELLECT, 10}, {LSTR_PERSONALITY, 10}, {LSTR_ENDURANCE, 10},
                                      {LSTR_ACCURACY, 10}, {LSTR_SPEED, 10}, {LSTR_LUCK, 10}},
         {LSTR_HIT_POINTS, LSTR_SPELL_POINTS, LSTR_ARMOR_CLASS}}, // Arthur.
        {117, 410, 0, SKILL_INVALID, {{LSTR_LUCK, 30}}, {LSTR_ARMOR_CLASS}}, // Pendragon.
        {22, 411, 0, SKILL_INVALID, {{LSTR_SPEED, 30}}, {LSTR_ARMOR_CLASS}}, // Lucius.
        {107, 412, 2, SKILL_INVALID, {{LSTR_SPELL_POINTS, 30}}, {LSTR_ARMOR_CLASS}}, // Guinevere.
        {232, 413, 1, SKILL_INVALID, {{LSTR_SPELL_POINTS, 25}}, {LSTR_ARMOR_CLASS}}, // Igraine.
        {84, 414, 3, SKILL_INVALID, {{LSTR_SPELL_POINTS, 20}}, {LSTR_ARMOR_CLASS}}, // Morgan.
    };
    checkArtifactCases(game, test, tapes, cases);
}

GAME_TEST(MM6ArtifactBonus, RelicsRaiseStatsAndTakeTheirToll) {
    // Relics gave neither their bonuses nor their drawbacks. A character who puts one on sees both on the stats tab.
    const std::vector<ArtifactCase> cases = {
        {4, 415, 0, SKILL_INVALID, {{LSTR_LUCK, 20}}, {}}, // Hades.
        {353, 416, 1, SKILL_INVALID, {{LSTR_FIRE, 25}}, {}}, // Ares.
        {10, 417, 0, SKILL_SPEAR, {{LSTR_MIGHT, 20}, {LSTR_ENDURANCE, 20}, {LSTR_ACCURACY, 20}, {LSTR_SPEED, -10}},
         {LSTR_HIT_POINTS, LSTR_ARMOR_CLASS}}, // Poseidon.
        {27, 418, 0, SKILL_INVALID, {{LSTR_HIT_POINTS, 100}, {LSTR_LUCK, -50}}, {}}, // Cronos.
        {157, 419, 0, SKILL_INVALID, {{LSTR_MIGHT, 50}, {LSTR_ENDURANCE, 20}, {LSTR_INTELLECT, -30}}, {LSTR_HIT_POINTS}}, // Hercules.
        {9, 420, 0, SKILL_BOW, {{LSTR_FIRE, -10}, {LSTR_ELECTRICITY, -10}, {LSTR_COLD, -10}, {LSTR_POISON, -10}}, {}}, // Artemis.
        {808, 421, 0, SKILL_CHAIN, {{LSTR_ENDURANCE, -30}, {LSTR_LUCK, 20}, {LSTR_FIRE, 20}, {LSTR_ELECTRICITY, 20}, {LSTR_COLD, 20},
                                    {LSTR_POISON, 20}},
         {LSTR_HIT_POINTS, LSTR_ARMOR_CLASS}}, // Apollo.
        {907, 422, 1, SKILL_PLATE, {{LSTR_HIT_POINTS, 50}, {LSTR_SPELL_POINTS, 50}, {LSTR_LUCK, 50}, {LSTR_INTELLECT, -50}}, {LSTR_ARMOR_CLASS}}, // Zeus.
        {84, 423, 1, SKILL_INVALID, {{LSTR_LUCK, 20}, {LSTR_SPEED, -20}}, {LSTR_ARMOR_CLASS}}, // Aegis.
        {265, 424, 0, SKILL_INVALID, {{LSTR_SPEED, -40}, {LSTR_FIRE, 50}, {LSTR_ELECTRICITY, 50}, {LSTR_COLD, 50}, {LSTR_POISON, 50}},
         {LSTR_ARMOR_CLASS}}, // Odin.
        {875, 425, 0, SKILL_INVALID, {{LSTR_MIGHT, 100}, {LSTR_SPEED, -40}}, {LSTR_ARMOR_CLASS}}, // Atlas.
        {875, 426, 1, SKILL_INVALID, {{LSTR_SPEED, 100}, {LSTR_ACCURACY, -40}}, {LSTR_ARMOR_CLASS}}, // Hermes.
        {875, 427, 2, SKILL_INVALID, {{LSTR_PERSONALITY, 100}, {LSTR_LUCK, -40}}, {LSTR_ARMOR_CLASS}}, // Aphrodite.
        {1038, 428, 2, SKILL_INVALID, {{LSTR_INTELLECT, 100}, {LSTR_MIGHT, -40}}, {LSTR_SPELL_POINTS, LSTR_ARMOR_CLASS}}, // Athena.
        {1020, 429, 3, SKILL_INVALID, {{LSTR_HIT_POINTS, 50}, {LSTR_LUCK, 50}, {LSTR_PERSONALITY, -50}}, {LSTR_SPELL_POINTS, LSTR_ARMOR_CLASS}}, // Hera.
    };
    checkArtifactCases(game, test, tapes, cases);
}

GAME_TEST(MM6ArtifactBonus, SwiftArtifactsShortenRecovery) {
    // Merlin and Percival recovered as slowly as a plain staff and a plain bow. They shorten the recovery as much as
    // the Swift enchantment does.
    struct Case {
        int weapon;
        ItemSlot slot;
        Skill skill;
        int distance; // How far ahead the goblin stands, in reach of the staff or out of it for the bow.
        int recovery;
    };
    const Case cases[] = {
        {61, ITEM_SLOT_MAIN_HAND, SKILL_STAFF, 150, 206}, // Staff.
        {404, ITEM_SLOT_MAIN_HAND, SKILL_STAFF, 150, 163}, // Merlin.
        {42, ITEM_SLOT_BOW, SKILL_BOW, 1000, 206}, // Longbow.
        {405, ITEM_SLOT_BOW, SKILL_BOW, 1000, 163}, // Percival.
    };
    for (const Case &c : cases) {
        SCOPED_TRACE(fmt::format("weapon={}", c.weapon));
        test.prepareForNextTest();
        startNewGameThroughInput(game);
        Character &knight = pParty->pCharacters[0];
        knight.inventory.equip(c.slot, Item(static_cast<ItemId>(c.weapon)));
        knight.setSkillValue(c.skill, CombinedSkillValue::novice());
        const Vec3f ahead(TrigLUT.cos(pParty->_viewYaw), TrigLUT.sin(pParty->_viewYaw), 0);
        const int goblin = game.spawnMonster(pParty->pos + ahead * c.distance, static_cast<MonsterId>(76), SPAWN_DUMMY)->id;
        auto recoveryTape = charTapes.custom(0, [](const Character &character) { return character.timeToRecovery.ticks(); });
        test.startTaping();

        game.pointMouseAtActor(goblin);
        game.pressAndReleaseKey(PlatformKey::KEY_A);
        game.tick();

        EXPECT_EQ(recoveryTape.max(), c.recovery);
    }
}
