#include <memory>

#include "Engine/AssetsManager.h"
#include "Engine/Engine.h"
#include "Engine/Localization.h"
#include "Engine/Party.h"

#include "Engine/Graphics/Renderer/Renderer.h"
#include "Engine/Graphics/Viewport.h"
#include "Engine/Graphics/Image.h"
#include "Engine/mm7_data.h"

#include "Engine/Tables/QuestTable.h"

#include "GUI/GUIButton.h"
#include "GUI/UI/UIGame.h"
#include "GUI/UI/Books/MM6BookLayout.h"
#include "GUI/UI/Books/QuestBook.h"

#include "Media/Audio/AudioPlayer.h"

GraphicsImage *ui_book_quests_background = nullptr;

GUIWindow_QuestBook::GUIWindow_QuestBook() {
    this->eWindowType = WINDOW_QUEST_BOOK;

    pChildBooksOverlay = std::make_unique<GUIWindow_BooksButtonOverlay>(Pointi{493, 355}, Sizei{0, 0}, pBtn_Quests);
    bFlashQuestBook = false;

    if (engine->gameTitle() == GameTitle::MM6) {
        ui_book_quests_background = assets->getIcon("quest_bg");
        ui_book_quest_div_bar = assets->getIcon("divbar");

        ui_book_button1_on = assets->getIcon("tab--on");
        ui_book_button2_on = assets->getIcon("tab+on");
        ui_book_button1_off = assets->getIcon("tab--off");
        ui_book_button2_off = assets->getIcon("tab+off");

        pBtn_Book_1 = createMM6Tab(mm6::books::kPrevPageTabY, BOOK_PREV_PAGE, INPUT_ACTION_DIALOG_LEFT, localization->str(LSTR_SCROLL_UP), ui_book_button1_on);
        pBtn_Book_2 = createMM6Tab(mm6::books::kNextPageTabY, BOOK_NEXT_PAGE, INPUT_ACTION_DIALOG_RIGHT, localization->str(LSTR_SCROLL_DOWN), ui_book_button2_on);
    } else {
        ui_book_quests_background = assets->getIcon("sbquiknot");
        ui_book_quest_div_bar = assets->getIcon("divbar");

        ui_book_button1_on = assets->getIcon("tab-an-6b");
        ui_book_button2_on = assets->getIcon("tab-an-7b");
        ui_book_button1_off = assets->getIcon("tab-an-6a");
        ui_book_button2_off = assets->getIcon("tab-an-7a");

        pBtn_Book_1 = CreateButton(pViewport.topLeft() + Pointi(398, 1), ui_book_button1_on->size(), BUTTON_TYPE_NORMAL, 0,
                                   UIMSG_ClickBooksBtn, std::to_underlying(BOOK_PREV_PAGE), INPUT_ACTION_DIALOG_LEFT, localization->str(LSTR_SCROLL_UP), {ui_book_button1_on});
        pBtn_Book_2 = CreateButton(pViewport.topLeft() + Pointi(398, 38), ui_book_button2_on->size(), BUTTON_TYPE_NORMAL, 0,
                                   UIMSG_ClickBooksBtn, std::to_underlying(BOOK_NEXT_PAGE), INPUT_ACTION_DIALOG_RIGHT, localization->str(LSTR_SCROLL_DOWN), {ui_book_button2_on});
    }

    for (auto i : pQuestTable.indices()) {
        if (pParty->_questBits[i] && !pQuestTable[i].empty()) {
            _activeQuestsIdx.push_back(i);
        }
    }
}

void GUIWindow_QuestBook::Update() {
    drawExitButton();

    int pTextHeight;
    const bool isMM6 = engine->gameTitle() == GameTitle::MM6;
    if (isMM6) {
        render->DrawQuad2D(ui_book_quests_background, mm6::books::kPagePosition);
        drawMM6Tab(mm6::books::kPrevPageTabY, ui_book_button1_on, ui_book_button1_off, isButtonPressed(BOOK_PREV_PAGE) || !_startingQuestIdx);
        drawMM6Tab(mm6::books::kNextPageTabY, ui_book_button2_on, ui_book_button2_off,
                   isButtonPressed(BOOK_NEXT_PAGE) || (_startingQuestIdx + _currentPageQuests) >= _activeQuestsIdx.size());
    } else {
        render->DrawQuad2D(ui_book_quests_background, pViewport.topLeft());

        if ((_bookButtonClicked && _bookButtonAction == BOOK_PREV_PAGE) || !_startingQuestIdx) {
            render->DrawQuad2D(ui_book_button1_off, pViewport.topLeft() + Pointi(407, 2));
        } else {
            render->DrawQuad2D(ui_book_button1_on, pViewport.topLeft() + Pointi(398, 1));
        }

        if ((_bookButtonClicked && _bookButtonAction == BOOK_NEXT_PAGE) || (_startingQuestIdx + _currentPageQuests) >= _activeQuestsIdx.size()) {
            render->DrawQuad2D(ui_book_button2_off, pViewport.topLeft() + Pointi(407, 38));
        } else {
            render->DrawQuad2D(ui_book_button2_on, pViewport.topLeft() + Pointi(398, 38));
        }
    }

    // for title
    DrawTitleText(assets->pFontBookTitle.get(), 0, 22, ui_book_quests_title_color, localization->str(LSTR_CURRENT_QUESTS), 3, isMM6 ? mm6::books::kTitleFrame : pViewport);

    // for other text
    Recti questbook_window(48, 70, 360, 264);

    if (_bookButtonClicked == BOOK_BUTTON_PRESSED_FRAMES && _bookButtonAction == BOOK_NEXT_PAGE && (_startingQuestIdx + _currentPageQuests) < _activeQuestsIdx.size()) {
        pAudioPlayer->playUISound(SOUND_openbook);
        _startingQuestIdx += _currentPageQuests;
        _questsPerPage[_currentPage] = _currentPageQuests;
        _currentPage++;
    }

    if (_bookButtonClicked == BOOK_BUTTON_PRESSED_FRAMES && _bookButtonAction == BOOK_PREV_PAGE && _startingQuestIdx) {
        pAudioPlayer->playUISound(SOUND_openbook);
        _currentPage--;
        _startingQuestIdx -= _questsPerPage[_currentPage];
    }

    if (_bookButtonClicked)
        _bookButtonClicked--;

    _currentPageQuests = 0;

    for (int i = _startingQuestIdx; i < _activeQuestsIdx.size(); ++i) {
        _currentPageQuests++;

        DrawText(assets->pFontBookOnlyShadow.get(), {1, 0}, ui_book_quests_text_color, pQuestTable[_activeQuestsIdx[i]], questbook_window);
        pTextHeight = assets->pFontBookOnlyShadow->CalcTextHeight(pQuestTable[_activeQuestsIdx[i]], questbook_window.w, 1);
        if ((questbook_window.y + pTextHeight) > questbook_window.h) {
            break;
        }

        render->DrawQuad2D(ui_book_quest_div_bar, {100, (questbook_window.y + pTextHeight) + 12});
        questbook_window.y = (questbook_window.y + pTextHeight) + 24;
    }
}
