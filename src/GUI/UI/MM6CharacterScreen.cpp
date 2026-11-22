#include "MM6CharacterScreen.h"

#include <array>
#include <tuple>
#include <utility>
#include <string>
#include <string_view>
#include <vector>

#include "Application/GameConfig.h"

#include "Engine/AssetsManager.h"
#include "Engine/Engine.h"
#include "Engine/Graphics/Renderer/Renderer.h"
#include "Engine/Localization.h"
#include "Engine/MM6/MM6ActualStatCondition.h"
#include "Engine/Objects/Character.h"
#include "Engine/Spells/Spells.h"

#include "GUI/GUIButton.h"
#include "GUI/GUIFont.h"
#include "GUI/GUIWindow.h"

#include "Io/InputEnums.h"

#include "Library/Color/ColorTable.h"

struct MM6CharacterScreenButton {
    GUIButton **button;
    std::string_view id;
    WindowType tab;
    int x;
    UIMessageType message;
    InputAction action;
    LstrId label;
    std::string_view normalIcon;
    std::string_view pressedIcon;
    std::string_view activeIcon;
};

static constexpr int MM6_CHARACTER_SCREEN_BUTTON_Y = 318;
static constexpr Sizei MM6_CHARACTER_SCREEN_BUTTON_SIZE = {74, 32}; // CreateButton adds a pixel, MM6's hit box is 75x33.

static const std::array<MM6CharacterScreenButton, 5> mm6CharacterScreenButtons = {{
    {&pCharacterScreen_StatsBtn, "CharacterWindow_Stats", WINDOW_CHARACTER_STATS, 22, UIMSG_ClickStatsBtn,
     INPUT_ACTION_OPEN_STATS, LSTR_STATS, "buttsta1", "buttsta2", "stat_hot"},
    {&pCharacterScreen_SkillsBtn, "CharacterWindow_Skills", WINDOW_CHARACTER_SKILLS, 115, UIMSG_ClickSkillsBtn,
     INPUT_ACTION_OPEN_SKILLS, LSTR_SKILLS, "buttski1", "buttski2", "skil_hot"},
    {&pCharacterScreen_InventoryBtn, "CharacterWindow_Inventory", WINDOW_CHARACTER_INVENTORY, 208, UIMSG_ClickInventoryBtn,
     INPUT_ACTION_OPEN_INVENTORY, LSTR_INVENTORY, "buttinv1", "buttinv2", "equi_hot"},
    {&pCharacterScreen_AwardsBtn, "CharacterWindow_Awards", WINDOW_CHARACTER_AWARDS, 301, UIMSG_ClickAwardsBtn,
     INPUT_ACTION_OPEN_AWARDS, LSTR_AWARDS, "buttawa1", "buttawa2", "awar_hot"},
    {&pCharacterScreen_ExitBtn, "CharacterWindow_Exit", WINDOW_NULL, 394, UIMSG_ClickExitCharacterWindowBtn,
     INPUT_ACTION_INVALID, LSTR_EXIT_DIALOGUE, "buttexi1", "buttexi2", {}},
}};

void createMM6CharacterScreenButtons(GUIWindow *window) {
    for (const MM6CharacterScreenButton &entry : mm6CharacterScreenButtons) {
        *entry.button = window->CreateButton(std::string(entry.id), {entry.x, MM6_CHARACTER_SCREEN_BUTTON_Y}, MM6_CHARACTER_SCREEN_BUTTON_SIZE,
                                             BUTTON_TYPE_NORMAL, 0, entry.message, 0, entry.action, localization->str(entry.label),
                                             {assets->getIcon(entry.normalIcon), assets->getIcon(entry.pressedIcon)});
    }
}

void drawMM6CharacterScreenUnderlay() {
    render->DrawQuad2D(assets->getIcon("leather"), {8, 8});
    drawMM6CharacterScreenBorders();
    for (const MM6CharacterScreenButton &entry : mm6CharacterScreenButtons)
        render->DrawQuad2D(assets->getIcon(entry.normalIcon), (*entry.button)->rect.topLeft());
}

void drawMM6CharacterScreenActiveTab(WindowType activeTab) {
    for (const MM6CharacterScreenButton &entry : mm6CharacterScreenButtons)
        if (entry.tab == activeTab)
            render->DrawQuad2D(assets->getIcon(entry.activeIcon), (*entry.button)->rect.topLeft());
}

void drawMM6CharacterScreenBorders() {
    render->DrawQuad2D(assets->getIcon("border5"), {7, 8});
    render->DrawQuad2D(assets->getIcon("border6"), {461, 8});
}

void drawMM6StatsTab(GUIWindow *window, Character *character) {
    render->DrawQuad2D(assets->getIcon("fr_stats"), MM6_CHARACTER_TAB_BACKGROUND_POSITION);
    GUIFont *font = assets->pFontArrus.get();
    const int rowStep = font->GetHeight() - 2;
    auto draw = [&](int x, int y, std::string_view text) {
        GUIWindow::DrawText(font, {x, y}, colorTable.White, text, window->frameRect);
    };

    draw(26, 18, fmt::format("{::}{}\f00000\r180{}: {::}{}\f00000\n\n\n", ui_character_header_text_color.tag(),
                             NameAndTitle(character->name, character->classType), localization->str(LSTR_SKILL_POINTS),
                             character->uSkillPoints ? ui_character_bonus_text_color.tag() : ui_character_default_text_color.tag(),
                             character->uSkillPoints));

    auto leftRow = [](LstrId label, int current, int base) {
        Color color = UI_GetHealthManaAndOtherQualitiesStringColor(current, base);
        if (base >= 1000)
            return fmt::format("{}{::}\r388{}\f00000 / {}\n", localization->str(label), color.tag(), current, base);
        return fmt::format("{}{::}\r424{}\f00000 /\t185{}\n", localization->str(label), color.tag(), current, base);
    };
    int y = 57;
    for (auto [label, current, base] : std::array<std::tuple<LstrId, int, int>, 7>{{
             {LSTR_MIGHT, character->GetActualMight(), character->GetBaseMight()},
             {LSTR_INTELLECT, character->GetActualIntelligence(), character->GetBaseIntelligence()},
             {LSTR_PERSONALITY, character->GetActualPersonality(), character->GetBasePersonality()},
             {LSTR_ENDURANCE, character->GetActualEndurance(), character->GetBaseEndurance()},
             {LSTR_ACCURACY, character->GetActualAccuracy(), character->GetBaseAccuracy()},
             {LSTR_SPEED, character->GetActualSpeed(), character->GetBaseSpeed()},
             {LSTR_LUCK, character->GetActualLuck(), character->GetBaseLuck()}}}) {
        draw(26, y, leftRow(label, current, base));
        y += rowStep;
    }

    y = 198;
    draw(26, y, leftRow(LSTR_HIT_POINTS, character->health, character->GetMaxHealth()));
    draw(26, y += rowStep, leftRow(LSTR_SPELL_POINTS, character->mana, character->GetMaxMana()));
    draw(26, y += rowStep, leftRow(LSTR_ARMOR_CLASS, character->GetActualAC(), character->GetBaseAC()));

    Condition condition = character->GetMajorConditionIdx();
    if (engine->config->gameplay.FixStatsScreenCondition.value()) // MM6.exe's stats screen reads the stat order (0x4145C0).
        condition = mm6::actualStatCondition(GameTitle::MM6, [&](Condition candidate) { return character->conditions.has(candidate); });
    window->DrawTextInRect(font, {26, 268}, colorTable.White,
                           fmt::format("{}: {::}{}\n", localization->str(LSTR_CONDITION), GetConditionDrawColor(condition).tag(),
                                       localization->characterConditionName(condition)), 226, 0);
    std::string quickSpell = character->uQuickSpell != SPELL_NONE ? pSpellStats->pInfos[character->uQuickSpell].pShortName
                                                                  : localization->str(LSTR_NONE);
    window->DrawTextInRect(font, {26, 286}, colorTable.White, fmt::format("{}: {}", localization->str(LSTR_QUICK_SPELL), quickSpell), 226, 0);

    auto rightRow = [](LstrId label, int current, int base, bool wide) {
        Color color = UI_GetHealthManaAndOtherQualitiesStringColor(current, base);
        if (wide)
            return fmt::format("{}{::}\r180{}\f00000 / {}\n", localization->str(label), color.tag(), current, base);
        return fmt::format("{}{::}\t100{}\f00000 / {}\n", localization->str(label), color.tag(), current, base);
    };
    draw(273, 54, rightRow(LSTR_AGE, character->GetActualAge(), character->GetBaseAge(), false));
    draw(273, 71, rightRow(LSTR_LEVEL, character->GetActualLevel(), character->GetBaseLevel(), character->GetBaseLevel() > 99));
    draw(273, 88, fmt::format("{}\r180{::}{}\f00000\n\n", localization->str(character->experience <= 9999999 ? LSTR_EXPERIENCE : LSTR_EXP),
                              character->GetExperienceDisplayColor().tag(), character->experience));
    draw(273, 126, fmt::format("{}\t100{:+}\n", localization->str(LSTR_ATTACK), character->GetActualAttack(false)));
    draw(273, 143, fmt::format("{}\t100 {}\n", localization->str(LSTR_DAMAGE), character->GetMeleeDamageString()));
    draw(273, 160, fmt::format("{}\t100{:+}\n", localization->str(LSTR_SHOOT), character->GetRangedAttack()));
    draw(273, 177, fmt::format("{}\t100 {}\n\n", localization->str(LSTR_DAMAGE), character->GetRangedDamageString()));

    auto resistanceRow = [](LstrId label, int current, int base) {
        Color color = UI_GetHealthManaAndOtherQualitiesStringColor(current, base);
        if (current > 99 || base > 99)
            return fmt::format("{}{::}\r180{}\f00000 / {}\n", localization->str(label), color.tag(), current, base);
        return fmt::format("{}{::}\t110{}\f00000 / {}\n", localization->str(label), color.tag(), current, base);
    };
    y = 216;
    // The shared runtime keeps MM6 Electricity, Cold and Poison in its Air, Water and Body fields.
    for (auto [label, attribute] : std::array<std::pair<LstrId, Attribute>, 4>{{
             {LSTR_FIRE, ATTRIBUTE_RESIST_FIRE}, {LSTR_ELECTRICITY, ATTRIBUTE_RESIST_AIR},
             {LSTR_COLD, ATTRIBUTE_RESIST_WATER}, {LSTR_POISON, ATTRIBUTE_RESIST_BODY}}}) {
        draw(273, y, resistanceRow(label, character->GetActualResistance(attribute), character->GetBaseResistance(attribute)));
        y += rowStep;
    }
    draw(273, y, resistanceRow(LSTR_MAGIC, character->sResMagicBase + character->sResMagicBonus, character->sResMagicBase));
}

std::vector<GraphicsImage *> mm6CharacterScreenExitTextures() {
    const MM6CharacterScreenButton &exit = mm6CharacterScreenButtons.back();
    return {assets->getIcon(exit.normalIcon), assets->getIcon(exit.pressedIcon)};
}
