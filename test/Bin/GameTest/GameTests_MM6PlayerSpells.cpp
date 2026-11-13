#include <algorithm>
#include <cctype>
#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineCallObserver.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/MM6/MM6ChestMapLoadRoute.h"
#include "Engine/MM6/MM6CombatRules.h"
#include "Engine/MM6/MM6PlayerSpell.h"
#include "Engine/MM6/MM6ScopedNewGameSave.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/CharacterEnumFunctions.h"
#include "Engine/Objects/Monsters.h"
#include "Engine/Party.h"
#include "Engine/SaveLoad.h"
#include "Engine/Spells/MM6SpellRules.h"
#include "Engine/Spells/SpellEnumFunctions.h"
#include "Engine/Spells/Spells.h"
#include "Engine/Tables/MapTable.h"

#include "Utility/Math/TrigLut.h"
#include "Utility/String/Format.h"

#include "GUI/GUIWindow.h"

static void completeEditableMM6Party() {
    for (Character &character : pParty->pCharacters) {
        for (int order = 4; order <= 12 && character.GetSkillIdxByOrder(3) == SKILL_INVALID; ++order) {
            const Skill skill = character.GetSkillIdxByOrder(order);
            if (skill != SKILL_INVALID && !character.pActiveSkills[skill])
                character.pActiveSkills[skill] = CombinedSkillValue::novice();
        }
        if (character.GetSkillIdxByOrder(3) == SKILL_INVALID)
            throw std::runtime_error("MM6 creation party lacks four skills");
    }
    for (int pass = 0; pass < 100 && CharacterCreation_GetUnspentAttributePointCount() > 0; ++pass)
        for (Character &character : pParty->pCharacters)
            for (Attribute stat : allStatAttributes())
                character.IncreaseAttribute(stat);
    if (CharacterCreation_GetUnspentAttributePointCount() != 0)
        throw std::runtime_error("MM6 creation party could not spend attribute points");
}

static void enterFirstMapFromNewGame(EngineController &game) {
    // Default party through the creation screen, then the first map in the real game loop. The test-only new-game
    // callback seeds the map cache from the loaded archives and writes no save.
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
    ASSERT_EQ(pGUIWindow_CurrentMenu->eWindowType, WINDOW_PARTY_CREATION);
    game.runGameRoutine(completeEditableMM6Party);
    game.pressGuiButton("PartyCreation_OK");
    game.skipLoadingScreen();
    game.tick();
    game.runGameRoutine([&] { chestOptIn.reset(); });
    mm6::testing::setNewGameSaveOverride({});
    ASSERT_EQ(engine->_currentLoadedMapId, pMapTable->GetMapInfo("oute3.odm"));
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
}

static constexpr int SORCERER = 2;

static PlatformKey characterKey(int characterIndex) {
    return static_cast<PlatformKey>(std::to_underlying(PlatformKey::KEY_DIGIT_1) + characterIndex);
}

static void teachSorcerer(SpellId spell, CombinedSkillValue value) {
    Character &sorcerer = pParty->pCharacters[SORCERER];
    sorcerer.pActiveSkills[skillForSpell(spell)] = value;
    sorcerer.bHaveSpell[spell] = true;
    sorcerer.mana = 100;
}

static void selectInSpellbook(EngineController &game, SpellId spell) {
    game.pressAndReleaseKey(characterKey(SORCERER));
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_C);
    game.tick();
    game.pressGuiButton(fmt::format("SpellBook_School{}", std::to_underlying(magicSchoolForSpell(spell))));
    game.tick();
    game.pressGuiButton(fmt::format("SpellBook_Spell{}", spellIndexInMagicSchool(spell)));
    game.tick();
}

static void castFromSpellbook(EngineController &game, SpellId spell) {
    selectInSpellbook(game, spell);
    game.pressGuiButton(fmt::format("SpellBook_Spell{}", spellIndexInMagicSchool(spell))); // A second click casts it.
    game.tick(2); // The book closes, the spell takes effect or opens the target picker on the next frame.
}

static void castAndRecover(EngineController &game, SpellId spell) {
    castFromSpellbook(game, spell);
    for (int frame = 0; frame < 100 && pParty->pCharacters[SORCERER].timeToRecovery > 0_ticks; frame++)
        game.tick(); // The sorcerer recovers from the cast.
}

static std::vector<PartyBuff> activePartyBuffs() {
    std::vector<PartyBuff> result;
    for (PartyBuff buff : pParty->pPartyBuffs.indices())
        if (pParty->pPartyBuffs[buff].Active())
            result.push_back(buff);
    return result;
}

static std::vector<CharacterBuff> activeCharacterBuffs(const Character &character) {
    std::vector<CharacterBuff> result;
    for (CharacterBuff buff : character.pCharacterBuffs.indices())
        if (character.pCharacterBuffs[buff].Active())
            result.push_back(buff);
    return result;
}

static void setQuickSpellFromSpellbook(EngineController &game, SpellId spell) {
    selectInSpellbook(game, spell);
    game.pressGuiButton("SpellBook_SetQuickSpell");
    game.tick();
}

static MonsterId sturdyMonster() {
    for (MonsterId id = static_cast<MonsterId>(1); std::to_underlying(id) < pMonsterStats->infos.size(); id = static_cast<MonsterId>(std::to_underlying(id) + 1))
        if (pMonsterStats->infos[id].hp >= 100 && pMonsterStats->infos[id].ac <= 20)
            return id;
    throw std::runtime_error("MM6 has no monster with 100 hit points and armor class 20 or less");
}

// A spell takes effect before its frame advances the game clock, so the tape counts a buff's time from the previous
// frame.
static TestTape<Duration> buffDurationTape(CommonTapeRecorder &tapes, std::function<const SpellBuff &()> buff) {
    return tapes.custom([buff = std::move(buff), frameStart = Time()]() mutable {
        const Duration result = buff().Active() ? buff().expireTime - frameStart : Duration();
        frameStart = pParty->GetPlayingTime();
        return result;
    });
}

static TestTape<Duration> partyBuffDurationTape(CommonTapeRecorder &tapes, PartyBuff buff) {
    return buffDurationTape(tapes, [buff]() -> const SpellBuff & { return pParty->pPartyBuffs[buff]; });
}

static std::string withoutAnimationFrame(const DrawnQuad &quad) {
    // Buff sprite frames are numbered, i2_fire1 and on.
    std::string result = quad.texture;
    while (!result.empty() && std::isdigit(static_cast<unsigned char>(result.back())))
        result.pop_back();
    return result;
}

static Vec3f waterAhead() {
    const Vec3f ahead(TrigLUT.cos(pParty->_viewYaw), TrigLUT.sin(pParty->_viewYaw), 0);
    for (int distance = 0; distance < 20000; distance += 32) {
        bool onWater = false;
        int faceId = -1;
        const Vec3f position = pParty->pos + ahead * distance;
        ODM_GetFloorLevel(position, &onWater, &faceId);
        if (onWater)
            return position;
    }
    throw std::runtime_error("No water ahead of the party");
}

GAME_TEST(MM6PlayerSpells, Fly) {
    // MM6 Fly was turned down as not available, and the shared Fly lasted an hour per level at every mastery.
    auto mpTape = charTapes.mp(SORCERER);
    auto flyTape = partyBuffDurationTape(tapes, PARTY_BUFF_FLY);
    auto flyingTape = tapes.custom([] { return pParty->bFlying; });
    auto heightTape = tapes.custom([] { return pParty->pos.z; });
    enterFirstMapFromNewGame(game);
    teachSorcerer(mm6::SPELL_FLY, CombinedSkillValue(4, MASTERY_NOVICE));
    test.startTaping();

    castFromSpellbook(game, mm6::SPELL_FLY);
    game.pressKey(PlatformKey::KEY_PAGEUP);
    game.tick(10); // The party rises clear of the ground.
    game.releaseKey(PlatformKey::KEY_PAGEUP);

    EXPECT_EQ(flyTape.max(), Duration::fromMinutes(20));
    EXPECT_LT(mpTape.back(), mpTape.front());
    EXPECT_EQ(flyingTape, tape(false, true));
    EXPECT_GT(heightTape.back(), heightTape.front());
}

GAME_TEST(MM6PlayerSpells, FlyEndsWhenTheCasterRunsDry) {
    // Fly drains a spell point every five minutes, and MM6 drops the party at the first drain that finds the caster with
    // none left to give.
    auto flightTape = tapes.custom([] {
        const int64_t drains = pParty->GetPlayingTime().ticks() / Duration::fromMinutes(5).ticks();
        return std::tuple(pParty->pCharacters[SORCERER].mana, pParty->bFlying, drains);
    });
    enterFirstMapFromNewGame(game);
    teachSorcerer(mm6::SPELL_FLY, CombinedSkillValue(4, MASTERY_NOVICE));
    const int mana = mm6::spellManaRecovery(mm6::SPELL_FLY, MASTERY_NOVICE).mana + 1;
    pParty->pCharacters[SORCERER].mana = mana;
    test.startTaping();

    castFromSpellbook(game, mm6::SPELL_FLY);
    game.pressKey(PlatformKey::KEY_PAGEUP);
    game.tick(10); // The party rises clear of the ground.
    game.releaseKey(PlatformKey::KEY_PAGEUP);
    game.tick(200); // Ten game minutes, two drains. The first takes the last spell point, the second finds none.

    const int64_t start = std::get<2>(flightTape.front());
    EXPECT_EQ(flightTape, tape(std::tuple(mana, false, start), std::tuple(1, false, start), std::tuple(1, true, start), std::tuple(1, true, start + 1),
                               std::tuple(0, true, start + 1), std::tuple(0, true, start + 2), std::tuple(0, false, start + 2)));
}

GAME_TEST(MM6PlayerSpells, Jump) {
    // MM6 Jump has the ID of MM7 Invisibility, and was turned down as not available.
    auto mpTape = charTapes.mp(SORCERER);
    auto heightTape = tapes.custom([] { return pParty->pos.z; });
    auto invisibleTape = tapes.custom([] { return pParty->pPartyBuffs[PARTY_BUFF_INVISIBILITY].Active(); });
    enterFirstMapFromNewGame(game);
    teachSorcerer(mm6::SPELL_JUMP, CombinedSkillValue(4, MASTERY_NOVICE));
    test.startTaping();

    castFromSpellbook(game, mm6::SPELL_JUMP);

    EXPECT_LT(mpTape.back(), mpTape.front());
    EXPECT_GT(heightTape.max(), heightTape.front());
    EXPECT_EQ(invisibleTape, tape(false));
}

GAME_TEST(MM6PlayerSpells, FeatherFall) {
    // MM6 Feather Fall has the ID of MM7 Jump, and was turned down as not available.
    auto mpTape = charTapes.mp(SORCERER);
    auto featherFallTape = partyBuffDurationTape(tapes, PARTY_BUFF_FEATHER_FALL);
    auto powerTape = tapes.custom([] { return pParty->pPartyBuffs[PARTY_BUFF_FEATHER_FALL].power; });
    auto heightTape = tapes.custom([] { return pParty->pos.z; });
    auto quadsTape = tapes.hudQuads();
    enterFirstMapFromNewGame(game);
    teachSorcerer(mm6::SPELL_FEATHER_FALL, CombinedSkillValue(4, MASTERY_EXPERT));
    test.startTaping();

    castFromSpellbook(game, mm6::SPELL_FEATHER_FALL);

    EXPECT_EQ(featherFallTape.max(), Duration::fromMinutes(40));
    EXPECT_EQ(powerTape, tape(0, 8));
    EXPECT_LT(mpTape.back(), mpTape.front());
    EXPECT_EQ(heightTape.size(), 1); // The party stays on the ground.
    EXPECT_CONTAINS(quadsTape.flatten().map(withoutAnimationFrame), "i1_ffal");
}

GAME_TEST(MM6PlayerSpells, WaterWalk) {
    // MM6 Water Walk was turned down as not available. Without it the party sinks into the water instead.
    for (bool waterWalk : {true, false}) {
        SCOPED_TRACE(fmt::format("waterWalk={}", waterWalk));
        test.prepareForNextTest();
        auto mpTape = charTapes.mp(SORCERER);
        auto waterWalkTape = partyBuffDurationTape(tapes, PARTY_BUFF_WATER_WALK);
        auto onWaterTape = tapes.custom([] { return static_cast<bool>(pParty->uFlags & PARTY_FLAG_STANDING_ON_WATER); });
        auto drowningTape = tapes.custom([] { return static_cast<bool>(pParty->uFlags & PARTY_FLAG_WATER_DAMAGE); });
        enterFirstMapFromNewGame(game);
        teachSorcerer(mm6::SPELL_WATER_WALK, CombinedSkillValue(4, MASTERY_MASTER));
        test.startTaping();

        if (waterWalk)
            castFromSpellbook(game, mm6::SPELL_WATER_WALK);
        game.runGameRoutine([] { pParty->pos = waterAhead(); });
        game.pressKey(PlatformKey::KEY_UP);
        game.tick(6); // The party drops from shore height to the water and walks out over it.
        game.releaseKey(PlatformKey::KEY_UP);

        if (waterWalk) {
            EXPECT_EQ(waterWalkTape.max(), Duration::fromHours(4));
            EXPECT_LT(mpTape.back(), mpTape.front());
        } else {
            EXPECT_EQ(waterWalkTape, tape(Duration()));
        }
        EXPECT_EQ(onWaterTape.back(), waterWalk);
        EXPECT_EQ(drowningTape.back(), !waterWalk);
    }
}

GAME_TEST(MM6PlayerSpells, AttackSpells) {
    // MM6 attack spells were turned down as not available. Five of them share their ID with an MM7 spell that isn't an
    // attack, and the rest rolled MM7 damage.
    struct AttackSpell {
        SpellId spell;
        int minDamage;
        int maxDamage;
    };
    const std::vector<AttackSpell> spells = {
        {mm6::SPELL_FLAME_ARROW, 1, 8},
        {mm6::SPELL_FIRE_BOLT, 10, 40},
        {mm6::SPELL_STATIC_CHARGE, 2, 6},
        {mm6::SPELL_SPARKS, 12, 12},
        {mm6::SPELL_LIGHTNING_BOLT, 10, 80},
        {mm6::SPELL_ICE_BOLT, 10, 70},
        {mm6::SPELL_MAGIC_ARROW, 3, 8},
        {mm6::SPELL_SPIRIT_ARROW, 1, 6},
    };
    for (const AttackSpell &attack : spells) {
        SCOPED_TRACE(fmt::format("spell={}", std::to_underlying(attack.spell)));
        test.prepareForNextTest();
        enterFirstMapFromNewGame(game);
        teachSorcerer(attack.spell, CombinedSkillValue(10, MASTERY_NOVICE));
        const Vec3f ahead(TrigLUT.cos(pParty->_viewYaw), TrigLUT.sin(pParty->_viewYaw), 0);
        const int monster = game.spawnMonster(pParty->pos + ahead * 1000, sturdyMonster(), SPAWN_DUMMY)->id;
        auto hpTape = actorTapes.hp(monster);
        auto mpTape = charTapes.mp(SORCERER);
        auto spritesTape = tapes.sprites();
        test.startTaping();

        setQuickSpellFromSpellbook(game, attack.spell);
        game.pointMouseAtActor(monster);
        game.pressAndReleaseKey(PlatformKey::KEY_S);
        for (int frame = 0; frame < 20 && hpTape.size() == 1; frame++)
            game.tick(); // The spell flies to the monster.

        EXPECT_LT(mpTape.back(), mpTape.front());
        EXPECT_CONTAINS(spritesTape.flatten(), SpellSpriteMapping[attack.spell]);
        ASSERT_EQ(hpTape.size(), 2);
        EXPECT_GE(hpTape.front() - hpTape.back(), attack.minDamage);
        EXPECT_LE(hpTape.front() - hpTape.back(), attack.maxDamage);
    }
}

GAME_TEST(MM6PlayerSpells, PartyBuffs) {
    // MM6 Torch Light, Wizard Eye and the protection spells were turned down as not available, and MM6 Protection from
    // Magic has the ID of MM7 Protection from Earth.
    struct BuffSpell {
        SpellId spell;
        PartyBuff buff;
        int power;
        std::string sprite; // MM6 draws no torch light sprite.
    };
    const std::vector<BuffSpell> spells = {
        {static_cast<SpellId>(1), PARTY_BUFF_TORCHLIGHT, 3, ""},
        {static_cast<SpellId>(12), PARTY_BUFF_WIZARD_EYE, 0, "wizeye"},
        {static_cast<SpellId>(3), PARTY_BUFF_RESIST_FIRE, 8, "i2_fire"},
        {static_cast<SpellId>(14), PARTY_BUFF_RESIST_AIR, 8, "i3_ltng"},
        {static_cast<SpellId>(25), PARTY_BUFF_RESIST_WATER, 8, "i4_cold"},
        {mm6::SPELL_PROTECTION_FROM_MAGIC, PARTY_BUFF_PROTECTION_FROM_MAGIC, 8, "i6_magc"},
        {static_cast<SpellId>(69), PARTY_BUFF_RESIST_BODY, 8, "i5_pois"},
    };
    for (const BuffSpell &buffSpell : spells) {
        SCOPED_TRACE(fmt::format("spell={}", std::to_underlying(buffSpell.spell)));
        test.prepareForNextTest();
        enterFirstMapFromNewGame(game);
        teachSorcerer(buffSpell.spell, CombinedSkillValue(4, MASTERY_EXPERT));
        auto timeLeftTape = partyBuffDurationTape(tapes, buffSpell.buff);
        auto powerTape = tapes.custom([&] { return pParty->pPartyBuffs[buffSpell.buff].power; });
        auto mpTape = charTapes.mp(SORCERER);
        auto quadsTape = tapes.hudQuads();
        test.startTaping();

        castFromSpellbook(game, buffSpell.spell);

        EXPECT_EQ(timeLeftTape.max(), Duration::fromHours(4));
        EXPECT_EQ(powerTape.back(), buffSpell.power);
        EXPECT_LT(mpTape.back(), mpTape.front());
        if (!buffSpell.sprite.empty())
            EXPECT_CONTAINS(quadsTape.flatten().map(withoutAnimationFrame), buffSpell.sprite);
    }
}

GAME_TEST(MM6PlayerSpells, ProtectionsRaiseResistances) {
    // The MM6 protection spells raise the resistance the character damage roll uses, MM6 Protection from Magic used to
    // raise Earth resistance instead.
    struct Protection {
        SpellId spell;
        DamageType damage;
    };
    const std::vector<Protection> spells = {
        {static_cast<SpellId>(3), DAMAGE_FIRE},
        {static_cast<SpellId>(14), DAMAGE_AIR},
        {static_cast<SpellId>(25), DAMAGE_WATER},
        {mm6::SPELL_PROTECTION_FROM_MAGIC, DAMAGE_MAGIC},
        {static_cast<SpellId>(69), DAMAGE_EARTH},
    };
    for (const Protection &protection : spells) {
        SCOPED_TRACE(fmt::format("spell={}", std::to_underlying(protection.spell)));
        test.prepareForNextTest();
        enterFirstMapFromNewGame(game);
        teachSorcerer(protection.spell, CombinedSkillValue(4, MASTERY_MASTER));
        auto resistanceTape = tapes.custom([&] { return mm6::characterResistance(pParty->pCharacters[0], protection.damage, *pParty, 0); });
        test.startTaping();

        castFromSpellbook(game, protection.spell);

        EXPECT_EQ(resistanceTape.delta(), 12);
    }
}

GAME_TEST(MM6PlayerSpells, ProtectionFromMagicLetsConditionsThrough) {
    // MM6 Protection from Magic took MM7's power to block a condition and used itself up doing it.
    auto conditionTape = charTapes.condition(0);
    auto powerTape = tapes.custom([] { return pParty->pPartyBuffs[PARTY_BUFF_PROTECTION_FROM_MAGIC].power; });
    enterFirstMapFromNewGame(game);
    teachSorcerer(mm6::SPELL_PROTECTION_FROM_MAGIC, CombinedSkillValue(4, MASTERY_EXPERT));
    test.startTaping();

    castFromSpellbook(game, mm6::SPELL_PROTECTION_FROM_MAGIC);
    game.runGameRoutine([] { pParty->pCharacters[0].SetCondition(CONDITION_POISON_WEAK, true); }); // As a monster's poison attack does.
    game.tick();

    EXPECT_EQ(conditionTape, tape(CONDITION_GOOD, CONDITION_POISON_WEAK));
    EXPECT_EQ(powerTape, tape(0, 8));
}

GAME_TEST(MM6PlayerSpells, BlessAndHeroism) {
    // MM6 Bless and Heroism were turned down as not available. The shared Bless lasted an hour plus 5 minutes per level
    // instead of MM6's 64 minutes.
    for (CharacterBuff buff : {CHARACTER_BUFF_BLESS, CHARACTER_BUFF_HEROISM}) {
        SCOPED_TRACE(fmt::format("buff={}", std::to_underlying(buff)));
        test.prepareForNextTest();
        const SpellId spell = buff == CHARACTER_BUFF_BLESS ? mm6::SPELL_BLESS : mm6::SPELL_HEROISM;
        auto timeLeftTape = buffDurationTape(tapes, [buff]() -> const SpellBuff & { return pParty->pCharacters[0].pCharacterBuffs[buff]; });
        auto powerTape = charTapes.custom(0, [buff](const Character &character) { return character.pCharacterBuffs[buff].power; });
        auto otherBuffTape = charTapes.hasBuff(3, buff);
        enterFirstMapFromNewGame(game);
        teachSorcerer(spell, CombinedSkillValue(4, MASTERY_NOVICE));
        test.startTaping();

        castFromSpellbook(game, spell);
        game.pressAndReleaseKey(characterKey(0));
        game.tick();

        EXPECT_EQ(timeLeftTape.max(), Duration::fromMinutes(84));
        EXPECT_EQ(powerTape, tape(0, 9));
        EXPECT_EQ(otherBuffTape, tape(false));
    }
}

GAME_TEST(MM6PlayerSpells, HealingSpells) {
    // MM6 Healing Touch has the ID of MM7 Fate and Cure Wounds the ID of MM7 Regeneration, both were turned down as not
    // available.
    struct HealingSpell {
        SpellId spell;
        int minHeal;
        int maxHeal;
    };
    const std::vector<HealingSpell> spells = {
        {mm6::SPELL_HEALING_TOUCH, 3, 7},
        {mm6::SPELL_CURE_WOUNDS, 13, 13},
    };
    for (const HealingSpell &healing : spells) {
        SCOPED_TRACE(fmt::format("spell={}", std::to_underlying(healing.spell)));
        test.prepareForNextTest();
        enterFirstMapFromNewGame(game);
        teachSorcerer(healing.spell, CombinedSkillValue(4, MASTERY_NOVICE));
        pParty->pCharacters[0].health = 1;
        auto hpTape = charTapes.hp(0);
        auto regenerationTape = charTapes.hasBuff(0, CHARACTER_BUFF_REGENERATION);
        test.startTaping();

        castFromSpellbook(game, healing.spell);
        game.pressAndReleaseKey(characterKey(0));
        game.tick();

        EXPECT_GE(hpTape.delta(), healing.minHeal);
        EXPECT_LE(hpTape.delta(), healing.maxHeal);
        EXPECT_EQ(regenerationTape, tape(false));
    }
}

GAME_TEST(MM6PlayerSpells, FirstAidShortensTargetRecovery) {
    // MM6 First Aid also takes a tick per spell level off the target's recovery, it only healed.
    auto recoveryTape = tapes.custom([] {
        return pParty->pCharacters[3].timeToRecovery - pParty->pCharacters[0].timeToRecovery;
    });
    enterFirstMapFromNewGame(game);
    teachSorcerer(static_cast<SpellId>(68), CombinedSkillValue(4, MASTERY_NOVICE));
    pParty->pCharacters[0].timeToRecovery = Duration::fromTicks(1000);
    pParty->pCharacters[3].timeToRecovery = Duration::fromTicks(1000);
    test.startTaping();

    castFromSpellbook(game, static_cast<SpellId>(68));
    game.pressAndReleaseKey(characterKey(0));
    game.tick();

    EXPECT_EQ(recoveryTape, tape(Duration(), Duration::fromTicks(4)));
}

GAME_TEST(MM6PlayerSpells, MonsterDispelMagicEndsEveryBuff) {
    // A monster's Dispel Magic left Protection from Magic on the party, with its icon on the buff shelf.
    auto partyBuffsTape = tapes.custom([] { return activePartyBuffs(); });
    auto characterBuffsTape = charTapes.custom([](const Character &character) { return activeCharacterBuffs(character); });
    auto quadsTape = tapes.hudQuads();
    enterFirstMapFromNewGame(game);
    for (SpellId spell : {mm6::SPELL_BLESS, mm6::SPELL_HEROISM, static_cast<SpellId>(3), mm6::SPELL_FLY, mm6::SPELL_PROTECTION_FROM_MAGIC}) {
        teachSorcerer(spell, CombinedSkillValue(4, MASTERY_EXPERT));
        castAndRecover(game, spell);
    }
    test.startTaping();

    Actor *lich = game.spawnMonster(pParty->pos + Vec3f(0, 1500, 0), static_cast<MonsterId>(94), SPAWN_STATIONARY);
    lich->monsterInfo.spell1UseChance = 100;
    for (int frame = 0; frame < 100 && pParty->pPartyBuffs[PARTY_BUFF_RESIST_FIRE].Active(); frame++)
        game.tick(); // The lich turns to the party and casts.

    EXPECT_EQ(partyBuffsTape, tape(std::vector{PARTY_BUFF_RESIST_FIRE, PARTY_BUFF_FLY, PARTY_BUFF_PROTECTION_FROM_MAGIC}, std::vector<PartyBuff>()));
    const std::vector blessed = {CHARACTER_BUFF_BLESS, CHARACTER_BUFF_HEROISM};
    const std::vector<CharacterBuff> none;
    EXPECT_EQ(characterBuffsTape, tape({blessed, blessed, blessed, blessed}, {none, none, none, none})); // The new party fails every save.
    EXPECT_CONTAINS(quadsTape.front().map(withoutAnimationFrame), "i6_magc");
    EXPECT_FALSE(std::ranges::contains(quadsTape.back().map(withoutAnimationFrame), "i6_magc"));
}

GAME_TEST(MM6PlayerSpells, ShieldHalvesArrows) {
    // A Shield buff didn't halve a monster's arrow.
    for (bool shield : {false, true}) {
        SCOPED_TRACE(fmt::format("shield={}", shield));
        test.prepareForNextTest();
        auto hpTape = charTapes.hps();
        auto spritesTape = tapes.sprites();
        enterFirstMapFromNewGame(game);
        if (shield)
            for (Character &character : pParty->pCharacters)
                character.pCharacterBuffs[CHARACTER_BUFF_SHIELD].Apply(pParty->GetPlayingTime() + Duration::fromHours(1), MASTERY_EXPERT, 0, 0, -1);
        test.startTaping();

        game.spawnMonster(pParty->pos + Vec3f(0, 1500, 0), static_cast<MonsterId>(1), SPAWN_STATIONARY);
        for (int frame = 0; frame < 100 && hpTape.size() == 1; frame++)
            game.tick(); // The archer shoots until an arrow lands.

        EXPECT_CONTAINS(spritesTape.flatten(), static_cast<SpriteId>(500));
        EXPECT_EQ(hpTape, tape({38, 22, 22, 20}, {38, 22, shield ? 20 : 18, 20}));
    }
}
