#include "MM6NpcDialogue.h"

#include <algorithm>
#include <string>
#include <utility>

#include "Engine/AssetsManager.h"
#include "Engine/Graphics/Renderer/Renderer.h"
#include "Engine/Localization.h"

#include "GUI/GUIButton.h"
#include "GUI/GUIFont.h"
#include "GUI/GUIWindow.h"

static constexpr int OPTION_AREA_TOP = 138;
static constexpr int OPTION_AREA_HEIGHT = 174;
static constexpr int OPTION_MAX_SPACING = 32;
static const Recti optionTextRect(483, 0, 148, 480);
static const Pointi exitPosition(526, 313);

static int textHeight(GUIFont *font, std::string_view text, int width) {
    // MM6 counts every line as font height - 3, CalcTextHeight counts the first one as font height - 6.
    return text.empty() ? 0 : font->CalcTextHeight(text, width, 0) + 3;
}

GUIButton *mm6::createNpcDialogueExitButton(GUIWindow *window) {
    return window->CreateButton("NpcDialogue_Exit", exitPosition, {74, 32}, BUTTON_TYPE_NORMAL, 0, UIMSG_Escape, 0, INPUT_ACTION_INVALID,
                                localization->str(LSTR_EXIT_DIALOGUE), {assets->getIcon("buttesc2")});
}

GUIButton *mm6::createNpcDialogueOptionButton(GUIWindow *window, int index, DialogueId topic, int y, int height) {
    return window->CreateButton(fmt::format("NpcDialogue_Option{}", index), {480, y}, {139, height - 1}, BUTTON_TYPE_NORMAL, 0,
                                UIMSG_SelectNPCDialogueOption, std::to_underlying(topic), INPUT_ACTION_INVALID, "");
}

void mm6::drawNpcDialoguePanel(GraphicsImage *portrait, std::string_view name) {
    render->DrawQuad2D(assets->getIcon("evpan019"), {481, 0}); // MM6 has no conversation background, it shows the panel of house 19.
    render->DrawQuad2D(portrait, {525, 34});
    GUIWindow::DrawTitleText(assets->pFontArrus.get(), 483, 110, ui_game_dialogue_npc_name_color, name, 3, Recti(0, 0, 630, 480));
}

void mm6::drawNpcDialogueOptions(GUIWindow *window) {
    GUIFont *font = assets->pFontArrus.get();
    int first = window->pStartingPosActiveItem;
    int count = 0;
    int totalHeight = 0;
    for (; count < window->pNumPresenceButton; ++count) {
        GUIButton *button = window->GetControl(first + count);
        if (!button)
            break;
        totalHeight += textHeight(font, button->label, optionTextRect.w);
    }

    if (count > 0) {
        int spacing = std::min((OPTION_AREA_HEIGHT - totalHeight) / count, OPTION_MAX_SPACING);
        int y = (OPTION_AREA_HEIGHT - count * spacing - totalHeight) / 2 - spacing / 2 + OPTION_AREA_TOP;
        for (int i = first; i < first + count; ++i) {
            GUIButton *button = window->GetControl(i);
            int height = textHeight(font, button->label, optionTextRect.w);
            button->rect.y = y + spacing;
            button->rect.h = height;
            Color color = window->pCurrentPosActiveItem == i ? ui_game_dialogue_option_highlight_color : ui_game_dialogue_option_normal_color;
            GUIWindow::DrawTitleText(font, 0, button->rect.y, color, button->label, 3, optionTextRect);
            y = button->rect.y + height - 1;
        }
    }

    render->DrawQuad2D(assets->getIcon("buttesc1"), exitPosition);
}
