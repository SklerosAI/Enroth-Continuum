#include "MM6Spellbook.h"

#include <algorithm>
#include <string>
#include <utility>

#include "Engine/AssetsManager.h"
#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Renderer/Renderer.h"
#include "Engine/Localization.h"
#include "Engine/Objects/CharacterEnumFunctions.h"
#include "Engine/Party.h"
#include "Engine/Spells/MM6SpellRules.h"
#include "Engine/Spells/SpellEnumFunctions.h"
#include "Engine/Spells/Spells.h"
#include "Engine/Timer.h"

#include "GUI/GUIButton.h"
#include "GUI/GUIFont.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/UIStatusBar.h"
#include "GUI/UI/UISpellbook.h"

#include "Io/Mouse.h"

#include "Media/Audio/AudioPlayer.h"

#include "Library/Color/ColorTable.h"

static constexpr IndexedArray<const char *, MAGIC_SCHOOL_FIRST, MAGIC_SCHOOL_LAST> mm6SchoolArtNames = {
    {MAGIC_SCHOOL_FIRE,   "fire"},
    {MAGIC_SCHOOL_AIR,    "air"},
    {MAGIC_SCHOOL_WATER,  "wtr"},
    {MAGIC_SCHOOL_EARTH,  "earth"},
    {MAGIC_SCHOOL_SPIRIT, "sprt"},
    {MAGIC_SCHOOL_MIND,   "mind"},
    {MAGIC_SCHOOL_BODY,   "body"},
    {MAGIC_SCHOOL_LIGHT,  "lite"},
    {MAGIC_SCHOOL_DARK,   "dark"}
};

/**
 * Top left corners of the page pictures, the school header at index 0 and the spells after it. Spells 1 and 2 sit
 * on the top row next to the header, the rest in three rows of three.
 */
static constexpr IndexedArray<std::array<Pointi, 12>, MAGIC_SCHOOL_FIRST, MAGIC_SCHOOL_LAST> mm6SpellIconPositions = {{
    {MAGIC_SCHOOL_FIRE,   {{{48, 18}, {198, 32}, {313, 32}, {68, 107}, {183, 107}, {328, 107}, {68, 182}, {198, 182}, {312, 182}, {56, 254}, {198, 257}, {328, 257}}}},
    {MAGIC_SCHOOL_AIR,    {{{48, 18}, {198, 32}, {328, 31}, {67, 104}, {198, 103}, {328, 107}, {68, 182}, {185, 182}, {300, 182}, {66, 255}, {181, 252}, {328, 255}}}},
    {MAGIC_SCHOOL_WATER,  {{{48, 18}, {197, 32}, {297, 31}, {64, 107}, {193, 105}, {325, 107}, {50, 182}, {194, 182}, {325, 182}, {68, 257}, {188, 257}, {325, 253}}}},
    {MAGIC_SCHOOL_EARTH,  {{{48, 18}, {198, 31}, {302, 32}, {46, 104}, {162, 104}, {328, 107}, {58, 182}, {198, 181}, {327, 182}, {68, 256}, {197, 257}, {308, 257}}}},
    {MAGIC_SCHOOL_SPIRIT, {{{48, 18}, {195, 27}, {328, 32}, {64, 103}, {198, 107}, {328, 107}, {68, 180}, {198, 182}, {328, 182}, {68, 257}, {181, 253}, {328, 257}}}},
    {MAGIC_SCHOOL_MIND,   {{{48, 18}, {198, 32}, {328, 32}, {68, 107}, {186, 106}, {328, 107}, {68, 182}, {180, 182}, {315, 179}, {68, 257}, {197, 258}, {316, 257}}}},
    {MAGIC_SCHOOL_BODY,   {{{48, 18}, {191, 31}, {328, 32}, {68, 107}, {198, 107}, {328, 107}, {68, 182}, {195, 179}, {315, 182}, {68, 257}, {197, 257}, {315, 250}}}},
    {MAGIC_SCHOOL_LIGHT,  {{{48, 18}, {193, 32}, {321, 32}, {68, 107}, {198, 107}, {322, 107}, {61, 180}, {188, 182}, {327, 182}, {68, 257}, {198, 257}, {328, 257}}}},
    {MAGIC_SCHOOL_DARK,   {{{48, 18}, {198, 32}, {314, 32}, {61, 107}, {183, 107}, {328, 107}, {68, 182}, {195, 182}, {324, 182}, {48, 255}, {188, 257}, {328, 257}}}}
}};

/**
 * Spell name anchors, indexed like `mm6SpellIconPositions` with index 0 unused. The x anchors are the same for every
 * school.
 */
static constexpr std::array<int, 12> mm6SpellNameX = {46, 176, 307, 46, 176, 307, 46, 176, 307, 46, 176, 307};
static constexpr IndexedArray<std::array<int, 12>, MAGIC_SCHOOL_FIRST, MAGIC_SCHOOL_LAST> mm6SpellNameY = {{
    {MAGIC_SCHOOL_FIRE,   {89, 89, 89, 161, 164, 164, 240, 240, 240, 314, 314, 314}},
    {MAGIC_SCHOOL_AIR,    {89, 89, 89, 161, 164, 164, 240, 240, 240, 314, 314, 314}},
    {MAGIC_SCHOOL_WATER,  {89, 89, 89, 161, 164, 164, 240, 240, 240, 314, 314, 314}},
    {MAGIC_SCHOOL_EARTH,  {89, 89, 89, 161, 164, 164, 240, 240, 240, 314, 314, 314}},
    {MAGIC_SCHOOL_SPIRIT, {89, 89, 89, 164, 164, 164, 240, 240, 240, 314, 314, 314}},
    {MAGIC_SCHOOL_MIND,   {89, 89, 89, 164, 164, 164, 240, 240, 240, 314, 314, 314}},
    {MAGIC_SCHOOL_BODY,   {89, 89, 89, 161, 164, 164, 240, 240, 240, 314, 314, 314}},
    {MAGIC_SCHOOL_LIGHT,  {89, 89, 89, 164, 164, 164, 240, 240, 240, 314, 314, 310}},
    {MAGIC_SCHOOL_DARK,   {89, 89, 89, 164, 164, 164, 236, 240, 240, 314, 314, 310}}
}};

static constexpr Pointi MM6_SPELLBOOK_POSITION = {8, 8}; // `book` and `pagemask`.
static constexpr Pointi MM6_SET_QUICK_SPELL_POSITION = {301, 332};
static constexpr Pointi MM6_EXIT_POSITION = {360, 332};
static constexpr Sizei MM6_BOTTOM_TAB_SIZE = {54, 16}; // CreateButton adds a pixel, MM6's hit box is 55x17.
static constexpr Sizei MM6_SPELL_SIZE = {99, 59}; // 100x60 in MM6.
static constexpr Sizei MM6_HEADER_SIZE = {125, 81}; // 126x82 in MM6.
static constexpr Sizei MM6_SCHOOL_TAB_SIZE = {49, 33}; // 50x34 in MM6.
static constexpr int MM6_SCHOOL_TAB_BUTTON_X = 410;
static constexpr int MM6_ACTIVE_SCHOOL_TAB_X = 414;
static constexpr int MM6_INACTIVE_SCHOOL_TAB_X = 421;
static constexpr int MM6_SCHOOL_TAB_Y = 13;
static constexpr int MM6_SCHOOL_TAB_STEP = 35;
static constexpr int MM6_SPELL_NAME_BOX_WIDTH = 100;
static constexpr int MM6_SPELL_POPUP_WIDTH = 460;
static constexpr Pointi MM6_SPELL_POPUP_PICTURE_OFFSET = {17, 17};

static bool hasMagicSchool(const Character &character, MagicSchool school) {
    return character.pActiveSkills[skillForMagicSchool(school)] || engine->config->debug.AllMagic.value();
}

static bool knowsSpell(const Character &character, SpellId spell) {
    return character.bHaveSpell[spell] || engine->config->debug.AllMagic.value();
}

static int schoolTabY(MagicSchool school) {
    return MM6_SCHOOL_TAB_Y + MM6_SCHOOL_TAB_STEP * std::to_underlying(school);
}

static Color spellNameColor(SpellId spell, SpellId quickSpell) {
    const bool selected = spell == spellbookSelectedSpell;
    const bool quick = spell == quickSpell;
    if (selected && quick)
        return Color(235, 15, 255);
    if (selected)
        return Color(255, 15, 15);
    if (quick)
        return Color(15, 15, 255);
    return Color::fromC16(1);
}

std::array<GraphicsImage *, 12> loadMM6SpellbookPage(MagicSchool school) {
    std::array<GraphicsImage *, 12> result;
    for (int i = 0; i < 12; ++i)
        result[i] = assets->getIcon(fmt::format("{}{:03}", mm6SchoolArtNames[school], i));
    return result;
}

void createMM6SpellbookButtons(GUIWindow *window) {
    const Character &character = pParty->activeCharacter();
    const MagicSchool page = character.lastOpenedSpellbookPage;

    for (SpellId spell : spellsForMagicSchool(page)) {
        int index = spellIndexInMagicSchool(spell);
        window->CreateButton(fmt::format("SpellBook_Spell{}", index), mm6SpellIconPositions[page][index + 1], MM6_SPELL_SIZE, BUTTON_TYPE_NORMAL,
                             UIMSG_Spellbook_ShowHightlightedSpellInfo, UIMSG_SelectSpell, std::to_underlying(spell));
    }

    window->CreateButton({0, 0}, {0, 0}, BUTTON_TYPE_NORMAL, 0, UIMSG_SpellBook_PressTab, 0, INPUT_ACTION_NEXT_CHAR);
    window->setKeyboardControlGroup(spellsForMagicSchool(page).size(), false, 0, 0);

    for (MagicSchool school : allMagicSchools())
        if (hasMagicSchool(character, school))
            window->CreateButton(fmt::format("SpellBook_School{}", std::to_underlying(school)), {MM6_SCHOOL_TAB_BUTTON_X, schoolTabY(school)},
                                 MM6_SCHOOL_TAB_SIZE, BUTTON_TYPE_NORMAL, 0, UIMSG_OpenSpellbookPage, std::to_underlying(school),
                                 INPUT_ACTION_INVALID, localization->spellSchoolName(school));

    pBtn_InstallRemoveSpell = window->CreateButton("SpellBook_SetQuickSpell", MM6_SET_QUICK_SPELL_POSITION, MM6_BOTTOM_TAB_SIZE, BUTTON_TYPE_NORMAL,
                                                   UIMSG_HintSelectRemoveQuickSpellBtn, UIMSG_ClickInstallRemoveQuickSpellBtn,
                                                   std::to_underlying(MM6_QUICK_SPELL_ACTION_SET));
    window->CreateButton("SpellBook_RemoveQuickSpell", mm6SpellIconPositions[page][0], MM6_HEADER_SIZE, BUTTON_TYPE_NORMAL,
                         UIMSG_HintSelectRemoveQuickSpellBtn, UIMSG_ClickInstallRemoveQuickSpellBtn, std::to_underlying(MM6_QUICK_SPELL_ACTION_REMOVE));
    pBtn_CloseBook = window->CreateButton("SpellBook_Exit", MM6_EXIT_POSITION, MM6_BOTTOM_TAB_SIZE, BUTTON_TYPE_NORMAL, 0, UIMSG_Escape, 0,
                                          INPUT_ACTION_INVALID, localization->str(LSTR_EXIT_DIALOGUE));
}

void drawMM6Spellbook(const std::array<GraphicsImage *, 12> &page) {
    const Character &character = pParty->activeCharacter();
    const MagicSchool school = character.lastOpenedSpellbookPage;

    render->DrawQuad2D(assets->getIcon("book"), MM6_SPELLBOOK_POSITION);
    render->DrawQuad2D(assets->getIcon("pagemask"), MM6_SPELLBOOK_POSITION);
    render->DrawQuad2D(assets->getIcon("tabspell"), MM6_SET_QUICK_SPELL_POSITION);
    render->DrawQuad2D(assets->getIcon("tabexit"), MM6_EXIT_POSITION);
    render->DrawQuad2D(assets->getIcon("border5"), {7, 8});
    render->DrawQuad2D(assets->getIcon("border6"), {461, 8});
    render->DrawQuad2D(page[0], mm6SpellIconPositions[school][0]);

    for (SpellId spell : spellsForMagicSchool(school)) {
        if (!knowsSpell(character, spell))
            continue;
        int i = spellIndexInMagicSchool(spell) + 1;
        render->DrawQuad2D(page[i], mm6SpellIconPositions[school][i]);
        Recti nameBox(mm6SpellNameX[i] + 6, mm6SpellNameY[school][i] - 5, MM6_SPELL_NAME_BOX_WIDTH, 344);
        GUIWindow::DrawTitleText(assets->pFontBookLloyds.get(), 0, 0, spellNameColor(spell, character.uQuickSpell), pSpellStats->pInfos[spell].name,
                                 i <= 8 ? 5 : 7, nameBox);
    }

    for (MagicSchool tab : allMagicSchools()) {
        if (!hasMagicSchool(character, tab))
            continue;
        if (tab == school) {
            render->DrawQuad2D(assets->getIcon(fmt::format("tab{}b", std::to_underlying(tab) + 1)), {MM6_ACTIVE_SCHOOL_TAB_X, schoolTabY(tab)});
        } else {
            render->DrawQuad2D(assets->getIcon(fmt::format("tab{}a", std::to_underlying(tab) + 1)), {MM6_INACTIVE_SCHOOL_TAB_X, schoolTabY(tab)});
        }
    }
}

void drawMM6SpellPopup(SpellId spell) {
    const Character &character = pParty->activeCharacter();
    const SpellInfo &info = pSpellStats->pInfos[spell];
    const int mouseY = mouse->position().y;
    Recti frame(90, mouseY <= 250 ? mouseY + 30 : 30, 328, 68);

    const int labelWidth = std::max({assets->pFontSmallnum->GetLineWidth(localization->str(LSTR_NORMAL)),
                                     assets->pFontSmallnum->GetLineWidth(localization->str(LSTR_EXPERT)),
                                     assets->pFontSmallnum->GetLineWidth(localization->str(LSTR_MASTER))});
    const std::string text = fmt::format("{}\n\n{}\t{:03}:\t{:03}{}\t000\n{}\t{:03}:\t{:03}{}\t000\n{}\t{:03}:\t{:03}{}", info.pDescription,
                                         localization->str(LSTR_NORMAL), labelWidth + 3, labelWidth + 10, info.pBasicSkillDesc,
                                         localization->str(LSTR_EXPERT), labelWidth + 3, labelWidth + 10, info.pExpertSkillDesc,
                                         localization->str(LSTR_MASTER), labelWidth + 3, labelWidth + 10, info.pMasterSkillDesc);
    frame.h = std::max(frame.h + assets->pFontSmallnum->CalcTextHeight(text, frame.w, 0), 150);
    frame.w = MM6_SPELL_POPUP_WIDTH;
    GUIWindow::DrawMessageBox(frame, "");
    render->DrawQuad2D(loadMM6SpellbookPage(magicSchoolForSpell(spell))[spellIndexInMagicSchool(spell) + 1], frame.topLeft() + MM6_SPELL_POPUP_PICTURE_OFFSET);
    frame.w -= 12;
    frame.h -= 12;
    GUIWindow::DrawTitleText(assets->pFontArrus.get(), 120, 12, colorTable.PaleCanary, info.name, 3, frame);
    GUIWindow::DrawText(assets->pFontSmallnum.get(), {120, 44}, colorTable.White, text, frame);

    frame.w = 108;
    const Skill skill = skillForMagicSchool(character.lastOpenedSpellbookPage);
    GUIWindow::DrawTitleText(assets->pFontComic.get(), 12, 75, colorTable.White, localization->skillName(skill), 3, frame);
    const Mastery mastery = std::max(character.getSkillValue(skill).mastery(), MASTERY_NOVICE); // MM6.exe reads a missing skill as Normal.
    const int cost = mm6::spellManaRecovery(spell, mastery, mm6::SPELL_COST_FROM_SPELLS_TXT).mana; // MM6.exe shows spells.txt's cost, also where the cast charges the executable's.
    GUIWindow::DrawTitleText(assets->pFontComic.get(), 12, frame.h - assets->pFontComic->GetHeight() - 16, colorTable.White,
                             fmt::format("{}\n{}", localization->str(LSTR_SP_COST), cost), 3, frame);
}

void clickMM6QuickSpellButton(MM6QuickSpellAction action) {
    if (!pParty->hasActiveCharacter())
        return;
    Character &character = pParty->activeCharacter();

    if (action == MM6_QUICK_SPELL_ACTION_REMOVE) {
        character.uQuickSpell = SPELL_NONE;
        spellbookSelectedSpell = SPELL_NONE;
        pAudioPlayer->playUISound(SOUND_fizzle);
        return;
    }

    if (spellbookSelectedSpell == SPELL_NONE) {
        pAudioPlayer->playUISound(SOUND_fizzle);
        return;
    }

    character.uQuickSpell = spellbookSelectedSpell;
    pGUIWindow_CurrentMenu = nullptr;
    gameTimer->setPaused(false);
    current_screen_type = SCREEN_GAME;
    character.playReaction(SPEECH_SET_QUICK_SPELL);
}

void hintMM6QuickSpellButton(MM6QuickSpellAction action) {
    if (action == MM6_QUICK_SPELL_ACTION_REMOVE) {
        engine->_statusBar->setPermanent(LSTR_CLICK_HERE_TO_REMOVE_YOUR_QUICK_SPELL);
    } else if (spellbookSelectedSpell != SPELL_NONE) {
        engine->_statusBar->setPermanent(LSTR_SET_S_AS_THE_READY_SPELL, pSpellStats->pInfos[spellbookSelectedSpell].name);
    } else {
        engine->_statusBar->setPermanent(LSTR_SELECT_A_SPELL_THEN_CLICK_HERE_TO_SET_A);
    }
}
