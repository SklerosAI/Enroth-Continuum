#include <algorithm>
#include <cctype>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/RenderEntities.h"
#include "Engine/Graphics/Renderer/Renderer.h"
#include "Engine/MM6/MM6ChestMapLoadRoute.h"
#include "Engine/MM6/MM6NativeActorNpcPolicy.h"
#include "Engine/MM6/MM6ScopedNewGameSave.h"
#include "Engine/MM6/MM6StreetNpcPortraits.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/CharacterEnumFunctions.h"
#include "Engine/Party.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/NPCTable.h"

#include "GUI/GUIWindow.h"

static AccessibleVector<RenderBillboardD3D::OpacityType> drawnBlendModes(ObjectType type) {
    // Blend modes of this frame's billboards for objects of the given type, each mode once.
    AccessibleVector<RenderBillboardD3D::OpacityType> result;
    for (int i = 0; i < static_cast<int>(render->uNumBillboardsToDraw); ++i) {
        const RenderBillboardD3D &billboard = render->pBillboardRenderListD3D[i];
        if (billboard.sParentBillboardID >= 0 && pBillboardRenderList[billboard.sParentBillboardID].object_pid.type() == type &&
            !std::ranges::contains(result, billboard.opacity))
            result.push_back(billboard.opacity);
    }
    return result;
}

static std::vector<int> drawnNpcPortraits(const TestMultiTape<std::string> &texturesTape) {
    // Numbers of the NPC portraits drawn on the taped frames, in the order they were first drawn.
    std::vector<int> result;
    for (const std::string &texture : texturesTape.flatten()) {
        if (texture.size() != 6 || !texture.starts_with("npc") || !std::isdigit(static_cast<unsigned char>(texture[3])))
            continue;
        const int portrait = std::stoi(texture.substr(3));
        if (!std::ranges::contains(result, portrait))
            result.push_back(portrait);
    }
    return result;
}

static int nearestActor(Vec3f pos, int monster) {
    // The map's actor of this MM6 monster ID nearest to the given point.
    int result = -1;
    float bestDistance = std::numeric_limits<float>::max();
    for (const Actor &actor : pActors) {
        const float distance = (actor.pos - pos).length();
        if (actor.monsterInfo.id == static_cast<MonsterId>(monster) && distance < bestDistance) {
            bestDistance = distance;
            result = actor.id;
        }
    }
    return result;
}

static void completeEditableMM6Party() {
    // Four skills for every character and no unspent attribute points, so that the creation screen accepts the party.
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

GAME_TEST(MM6StreetNpcs, Outc2LoadsWithItsStreetNpcs) {
    // Entering outc2 threw a generated NPC pool overflow, because MM7's peasant ID ranges picked well over a hundred
    // of its monsters for a street NPC.
    auto mapTape = tapes.map();
    auto screenTape = tapes.screen();
    auto streetPeasantsTape = tapes.custom([] {
        return std::ranges::count_if(pActors, [](const Actor &actor) { return mm6::nativeStreetNpcEligible(actor.monsterInfo.id); });
    });
    auto streetLinksTape = tapes.custom([] {
        return std::ranges::count_if(pActors, [](const Actor &actor) { return actor.npcId >= 5000 && actor.npcId < 5000 + pNPCStats->uNewlNPCBufPos; });
    });
    auto otherLinksTape = tapes.custom([] {
        return std::ranges::count_if(pActors, [](const Actor &actor) { return actor.npcId != 0 && actor.npcId < 5000; });
    });
    auto poolTape = tapes.custom([] { return pNPCStats->uNewlNPCBufPos; });
    enterFirstMapFromNewGame(game);

    std::unique_ptr<mm6::ScopedChestMapLoadOptIn> chestOptIn;
    game.runGameRoutine([&] { chestOptIn = std::make_unique<mm6::ScopedChestMapLoadOptIn>(); });
    game.teleportTo(pMapTable->GetMapInfo("outc2.odm"), Vec3f(11407, 18074, 1), 0); // Where the d08 exit puts the party.
    game.runGameRoutine([&] { chestOptIn.reset(); });
    test.startTaping();
    game.tick();

    EXPECT_EQ(mapTape, tape(pMapTable->GetMapInfo("outc2.odm")));
    EXPECT_EQ(screenTape, tape(SCREEN_GAME));
    EXPECT_EQ(streetPeasantsTape, tape(58));
    EXPECT_EQ(streetLinksTape, tape(58));
    EXPECT_EQ(poolTape, tape(58));
    EXPECT_EQ(otherLinksTape, tape(0));
}

GAME_TEST(MM6StreetNpcs, VillagersDrawOpaque) {
    // Villagers and monsters outdoors were drawn with additive blending, so they looked washed out and see-through.
    enterFirstMapFromNewGame(game);
    auto actorsTape = tapes.custom([] { return drawnBlendModes(OBJECT_Actor); });
    auto decorationsTape = tapes.custom([] { return drawnBlendModes(OBJECT_Decoration); });
    game.teleportTo(engine->_currentLoadedMapId, Vec3f(-10217, -7978, 161), 90); // Down the street from Rosemary the Guard.
    test.startTaping();
    game.tick();

    EXPECT_EQ(actorsTape, tape(std::vector{RenderBillboardD3D::Transparent})); // Alpha blended, the opaque modes are additive.
    EXPECT_EQ(decorationsTape, tape(std::vector{RenderBillboardD3D::Transparent}));
}

GAME_TEST(MM6StreetNpcs, StreetNpcPortraitMatchesSex) {
    // The male peasants of New Sorpigal talked with a woman's portrait and name, because MM7's peasant types disagree
    // with MM6's on sex. MM6 gives the Peasant F1 and F2 families women and the Peasant M1 family men.
    enterFirstMapFromNewGame(game);
    const int man = nearestActor(pParty->pos, 134);
    const int woman = nearestActor(pParty->pos, 121);
    auto screenTape = tapes.screen();
    auto texturesTape = tapes.hudTextures();
    test.startTaping();

    for (int peasant : {man, woman}) {
        game.teleportTo(engine->_currentLoadedMapId, pActors[peasant].pos - Vec3f(150, 0, 0), 0); // West of the peasant, facing east.
        game.pointMouseAtActor(peasant);
        game.pressAndReleaseButton(BUTTON_LEFT);
        game.tick();
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
    }

    EXPECT_EQ(screenTape, tape(SCREEN_GAME, SCREEN_NPC_DIALOGUE, SCREEN_GAME, SCREEN_NPC_DIALOGUE, SCREEN_GAME));
    const std::vector<int> portraits = drawnNpcPortraits(texturesTape);
    ASSERT_EQ(portraits.size(), 2);
    EXPECT_TRUE(std::ranges::contains(mm6StreetNpcMalePortraits, portraits[0])) << portraits[0];
    EXPECT_TRUE(std::ranges::contains(mm6StreetNpcFemalePortraits, portraits[1])) << portraits[1];
}
