#include "GUI/UI/UIInventory.h"

#include "Engine/AssetsManager.h"
#include "Engine/Engine.h"
#include "Engine/Localization.h"

#include "Engine/Graphics/Renderer/Renderer.h"
#include "Engine/Party.h"

#include "GUI/GUIButton.h"
#include "GUI/UI/MM6CharacterScreen.h"

#include "Io/KeyboardInputHandler.h"

GUIWindow_Inventory_CastSpell::GUIWindow_Inventory_CastSpell(Pointi position, Sizei dimensions, CastSpellInfo *spellInfo, std::string_view hint) :
    TargetedSpellUI(WINDOW_CAST_SPELL_IN_INVENTORY, position, dimensions, spellInfo, hint) {
    if (engine->gameTitle() == GameTitle::MM6) {
        pBtn_ExitCancel = CreateButton("CastSpell_Cancel", {392, 318}, {74, 32}, BUTTON_TYPE_NORMAL, 0, UIMSG_Escape, 0, INPUT_ACTION_INVALID,
                                       localization->str(LSTR_CANCEL), {assets->getIcon("buttesc2")});
    } else {
        pBtn_ExitCancel = CreateButton({392, 318}, {75, 33}, BUTTON_TYPE_NORMAL, 0, UIMSG_Escape, 0, INPUT_ACTION_INVALID, localization->str(LSTR_CANCEL), {ui_buttdesc2});
    }
    current_character_screen_window = WINDOW_CHARACTER_INVENTORY;
    current_screen_type = SCREEN_CASTING;
}

void GUIWindow_Inventory_CastSpell::Update() {
    draw_leather();
    CharacterUI_InventoryTab_Draw(&pParty->activeCharacter(), true);
    if (engine->gameTitle() == GameTitle::MM6) {
        drawMM6CharacterScreenBorders();
        render->DrawQuad2D(assets->getIcon("buttesc1"), pBtn_ExitCancel->rect.topLeft());
        return;
    }
    CharacterUI_DrawPaperdoll(&pParty->activeCharacter());
    render->DrawQuad2D(dialogue_ui_x_x_u, pBtn_ExitCancel->rect.topLeft());
}
