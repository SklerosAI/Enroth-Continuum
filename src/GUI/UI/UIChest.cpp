#include "UIChest.h"

#include <algorithm>
#include <string>

#include "Engine/Graphics/Renderer/Renderer.h"
#include "Engine/Graphics/Image.h"
#include "Engine/Localization.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Timer.h"
#include "Engine/Party.h"
#include "Engine/AssetsManager.h"
#include "Engine/Engine.h"
#include "Engine/Tables/ChestTable.h"

#include "GUI/GUIButton.h"
#include "GUI/GUIFont.h"
#include "GUI/UI/ItemGrid.h"
#include "GUI/UI/MM6CharacterScreen.h"

GUIWindow_Chest::GUIWindow_Chest(int chestId) : GUIWindow(WINDOW_CHEST, {0, 0}, render->GetRenderDimensions()), _chestId(chestId) {
    CreateCharacterButtons();

    if (engine->gameTitle() == GameTitle::MM6) {
        pBtn_ExitCancel = CreateButton("Chest_Exit", {392, 318}, {74, 32}, BUTTON_TYPE_NORMAL, 0, UIMSG_Escape, 0, INPUT_ACTION_INVALID,
                                       localization->str(LSTR_EXIT_DIALOGUE), {assets->getIcon("buttesc2")});
    } else {
        pBtn_ExitCancel = CreateButton({471, 445}, {169, 35}, BUTTON_TYPE_NORMAL, 0, UIMSG_Escape, 0, INPUT_ACTION_INVALID,
                                       localization->str(LSTR_EXIT_DIALOGUE), {ui_exit_cancel_button_background});
    }
    if (engine->gameTitle() == GameTitle::MM7 && Chest::canReroll(chestId))
        CreateButton("Chest_Reroll", {337, 326}, {104, 23}, BUTTON_TYPE_NORMAL, 0,
                     UIMSG_CHEST_Reroll, 0, INPUT_ACTION_INVALID, "Reroll chest loot");
    // The chest click area includes its lower wooden rim. Give the inset button first refusal.
    CreateButton({7, 8}, {460, 343}, BUTTON_TYPE_NORMAL, 0, UIMSG_CHEST_ClickItem, 0);
    current_screen_type = SCREEN_CHEST;
    gameTimer->setPaused(true);
}

void GUIWindow_Chest::drawExitButton() {
    if (engine->gameTitle() == GameTitle::MM6) {
        drawMM6CharacterScreenBorders();
        render->DrawQuad2D(assets->getIcon("buttesc1"), pBtn_ExitCancel->rect.topLeft());
        return;
    }
    render->DrawQuad2D(ui_exit_cancel_button_background, pBtn_ExitCancel->rect.topLeft());
}

void GUIWindow_Chest::Update() {
    if (engine->gameTitle() == GameTitle::MM6) {
        updatePageButton();
        updateRerollButton();
    }
    if (current_screen_type == SCREEN_CHEST_INVENTORY && pParty->hasActiveCharacter()) {
        draw_leather();
        CharacterUI_InventoryTab_Draw(&pParty->activeCharacter(), true);
        drawExitButton();
    } else if (current_screen_type == SCREEN_CHEST) {
        int uChestID = _chestId;
        int chestDescId = vChests[uChestID].chestTypeId;
        int chest_offs_x = chestTable[chestDescId].inventoryOffset.x;
        int chest_offs_y = chestTable[chestDescId].inventoryOffset.y;
        int chestWidthCells = chestTable[chestDescId].size.w;
        int chestHeghtCells = chestTable[chestDescId].size.h;

        GraphicsImage *chest_background = assets->getIcon(chestTable[chestDescId].textureName);
        render->DrawQuad2D(chest_background, {8, 8});

        render->SetUIClipRect({ chest_offs_x, chest_offs_y, 32 * chestWidthCells, 32 * chestHeghtCells });
        CharacterUI_DrawPickedItemUnderlay({ chest_offs_x, chest_offs_y });
        render->ResetUIClipRect();

        auto drawItem = [&](const Item &item, Pointi cell) {
            auto item_texture = assets->getIcon(item.GetIconName());
            int X_offset = itemOffset(item_texture->width());
            int Y_offset = itemOffset(item_texture->height());
            int itemPixelPosX = chest_offs_x + 32 * cell.x + X_offset;
            int itemPixelPosY = chest_offs_y + 32 * cell.y + Y_offset;

            assert(0 < itemPixelPosX && itemPixelPosX < 640);
            assert(0 < itemPixelPosY && itemPixelPosY < 480);
            render->DrawQuad2D(item_texture, {itemPixelPosX, itemPixelPosY});
        };

        if (_page == 0) {
            for (InventoryEntry entry : vChests[uChestID].inventory.entries())
                if (entry.zone() == INVENTORY_ZONE_GRID)
                    drawItem(*entry, entry.geometry().topLeft());
        } else {
            for (const mm6::ChestPageItem &item : vChests[uChestID].mm6Pages)
                if (InventoryEntry entry = vChests[uChestID].inventory.entry(item.slot); item.page == _page && entry)
                    drawItem(*entry, {item.cell % chestWidthCells, item.cell / chestWidthCells});
        }

        drawExitButton();
        if (_pageButton) {
            GUIFont *font = assets->pFontArrus.get();
            GraphicsImage *arrow = assets->getIcon("con_arrr");
            const std::string text = fmt::format("{}/{}", _page + 1, pageCount());
            const int textWidth = font->GetLineWidth(text);
            const Recti &rect = _pageButton->rect;
            const int x = rect.x + (rect.w - textWidth - 4 - arrow->width()) / 2;
            GUIWindow::DrawText(font, {x, rect.y + (rect.h - font->GetHeight()) / 2}, colorTable.White, text, frameRect);
            render->DrawQuad2D(arrow, {x + textWidth + 4, rect.y + (rect.h - arrow->height()) / 2});
        }
        if (_rerollButton) {
            const Recti &rect = _rerollButton->rect;
            GUIFont *font = assets->pFontArrus.get();
            render->FillRect(rect, colorTable.Tacha);
            render->FillRect({rect.x + 2, rect.y + 2, rect.w - 4, rect.h - 4}, colorTable.CarnabyTan);
            render->FillRect({rect.x + 4, rect.y + 4, rect.w - 8, rect.h - 8}, colorTable.Cioccolato);
            DrawText(font, {rect.x + font->AlignText_Center(rect.w, "Reroll"), rect.y + (rect.h - font->GetHeight()) / 2},
                     colorTable.PaleCanary, "Reroll", frameRect);
        }
        if (engine->gameTitle() == GameTitle::MM7 && Chest::canReroll(_chestId)) {
            render->FillRect({337, 326, 105, 24}, colorTable.Tacha);
            render->FillRect({339, 328, 101, 20}, colorTable.CarnabyTan);
            render->FillRect({341, 330, 97, 16}, colorTable.Cioccolato);
            DrawText(assets->pFontLucida.get(),
                     {341 + assets->pFontLucida->AlignText_Center(97, "Reroll"), 331},
                     colorTable.PaleCanary, "Reroll", frameRect);
        }
    }
}

void GUIWindow_Chest::showNextPage() {
    _page = (_page + 1) % pageCount();
}

int GUIWindow_Chest::pageCount() const {
    int result = 1;
    for (const mm6::ChestPageItem &item : vChests[_chestId].mm6Pages)
        result = std::max(result, item.page + 1);
    return result;
}

void GUIWindow_Chest::updateRerollButton() {
    const bool shown = current_screen_type == SCREEN_CHEST && Chest::canReroll(_chestId);
    if (shown && !_rerollButton) {
        _rerollButton = CreateButton("Chest_Reroll", {396, 254}, {66, 24}, BUTTON_TYPE_NORMAL, 0, UIMSG_CHEST_Reroll, 0,
                                     INPUT_ACTION_INVALID, "Reroll chest loot");
        std::rotate(vButtons.begin(), vButtons.end() - 1, vButtons.end()); // Ahead of the chest grid's button, which covers it.
    } else if (!shown && _rerollButton) {
        _rerollButton->Release();
        _rerollButton = nullptr;
    }
}

void GUIWindow_Chest::updatePageButton() {
    const int pages = pageCount();
    _page = std::min(_page, pages - 1);
    const bool shown = current_screen_type == SCREEN_CHEST && pages > 1;
    if (shown && !_pageButton) {
        _pageButton = CreateButton("Chest_NextPage", {400, 284}, {58, 26}, BUTTON_TYPE_NORMAL, 0, UIMSG_CHEST_NextPage, 0);
        std::rotate(vButtons.begin(), vButtons.end() - 1, vButtons.end()); // Ahead of the chest grid's button, which covers it.
    } else if (!shown && _pageButton) {
        _pageButton->Release();
        _pageButton = nullptr;
    }
}
