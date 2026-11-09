#include "MM6CharacterScreen.h"

#include <array>
#include <string_view>

#include "Engine/AssetsManager.h"
#include "Engine/Graphics/Renderer/Renderer.h"
#include "Engine/Localization.h"

#include "GUI/GUIButton.h"
#include "GUI/GUIWindow.h"

#include "Io/InputEnums.h"

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
static constexpr Sizei MM6_CHARACTER_SCREEN_BUTTON_SIZE = {75, 33};

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

void drawMM6CharacterScreenButtons(WindowType activeTab) {
    for (const MM6CharacterScreenButton &entry : mm6CharacterScreenButtons) {
        Pointi position = (*entry.button)->rect.topLeft();
        render->DrawQuad2D(assets->getIcon(entry.normalIcon), position);
        if (entry.tab == activeTab)
            render->DrawQuad2D(assets->getIcon(entry.activeIcon), position);
    }
}

std::vector<GraphicsImage *> mm6CharacterScreenExitTextures() {
    const MM6CharacterScreenButton &exit = mm6CharacterScreenButtons.back();
    return {assets->getIcon(exit.normalIcon), assets->getIcon(exit.pressedIcon)};
}
