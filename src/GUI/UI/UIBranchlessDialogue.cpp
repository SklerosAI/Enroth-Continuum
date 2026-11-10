#include <cassert>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

#include "UIBranchlessDialogue.h"

#include "Engine/Engine.h"
#include "Engine/AssetsManager.h"
#include "Engine/Evt/MM6EventRules.h"
#include "Engine/Evt/Processor.h"
#include "Engine/Graphics/Renderer/Renderer.h"
#include "Engine/Party.h"
#include "Engine/mm7_data.h"
#include "Engine/Graphics/Viewport.h"

#include "GUI/GUIFont.h"
#include "GUI/GUIMessageQueue.h"
#include "GUI/UI/UIHouses.h"
#include "GUI/UI/UIGame.h"

#include "Io/KeyboardInputHandler.h"

GUIWindow_BranchlessDialogue::GUIWindow_BranchlessDialogue(EvtOpcode opcode, std::optional<EvtContinuation> continuation)
    : GUIWindow(WINDOW_GREETING_NPC, {0, 0}, render->GetRenderDimensions()), _opcode(opcode), _continuation(continuation) {
    prev_screen_type = current_screen_type;
    keyboardInputHandler->StartTextInput(TEXT_INPUT_TEXT, 15, this);
    current_screen_type = SCREEN_BRANCHLESS_NPC_DIALOG;

    CreateCharacterButtons();
}

GUIWindow_BranchlessDialogue::~GUIWindow_BranchlessDialogue() {
    current_screen_type = prev_screen_type;
    keyboardInputHandler->EndTextInput(this);
}

void GUIWindow_BranchlessDialogue::setMM6Question(MM6QuestionDialogue question) {
    _mm6Question = std::move(question);
}

void GUIWindow_BranchlessDialogue::Update() {
    if (_mm6Question) {
        DrawDialoguePanel(_mm6Question->question);
        render->DrawQuad2D(game_ui_statusbar, {0, 352});
        if (keyboard_input_status != WINDOW_INPUT_IN_PROGRESS) {
            if (keyboard_input_status == WINDOW_INPUT_CONFIRMED) {
                const bool correct = mm6QuestionMatches(keyboardInputHandler->GetTextInput(),
                                                         _mm6Question->answer1, _mm6Question->answer2);
                _continuation = correct ? _mm6Question->success : _mm6Question->incorrect;
            } else {
                // Escape cancels the prompt and stops the current event.
                _continuation.reset();
            }
            keyboardInputHandler->EndTextInput(this);
            engine->_messageQueue->addMessageCurrentFrame(UIMSG_Escape, 0, 0);
            return;
        }
        GUIFont *font = assets->pFontLucida.get();
        const std::string &input = keyboardInputHandler->GetTextInput();
        DrawText(font, {13, 357}, colorTable.White, input, frameRect);
        DrawFlashingInputCursor(font->GetLineWidth(input) + 13, 357, font, frameRect);
        return;
    }

    if (current_npc_text.length() > 0 && branchless_dialogue_str.empty())
        branchless_dialogue_str = current_npc_text;

    pGUIWindow_BranchlessDialogue->DrawDialoguePanel(branchless_dialogue_str);
    render->DrawQuad2D(game_ui_statusbar, {0, 352});

    // TODO(Nik-RE-dev): this code related to text input in MM6/MM8, revisit
    // this functionality when it's time to support it.
#if 0
    if (pGUIWindow_BranchlessDialogue->keyboard_input_status != WINDOW_INPUT_IN_PROGRESS) {
        if (pGUIWindow_BranchlessDialogue->keyboard_input_status == WINDOW_INPUT_CONFIRMED) {
            pGUIWindow_BranchlessDialogue->keyboard_input_status = WINDOW_INPUT_NONE;
            GameUI_StatusBar_OnInput(keyboardInputHandler->GetTextInput());
        } else {
            GameUI_StatusBar_ClearInputString();
        }
        releaseBranchlessDialogue();
        return;
    }

    if (pGUIWindow_BranchlessDialogue->opcode() == EVENT_InputString) {
        auto str = fmt::format("{} {}", GameUI_StatusBar_GetInput(), keyboardInputHandler->GetTextInput());
        pGUIWindow_BranchlessDialogue->DrawText(pFontLucida, {13, 357}, colorTable.White, str);
        pGUIWindow_BranchlessDialogue->DrawFlashingInputCursor(pFontLucida->GetLineWidth(str) + 13, 357, pFontLucida);
        return;
    }
#endif

    // Close branchless dialog on any keypress
    if (!keyboardInputHandler->GetTextInput().empty()) {
        keyboardInputHandler->EndTextInput();
        engine->_messageQueue->addMessageCurrentFrame(UIMSG_Escape, 0, 0);
        return;
    }

    // Also close branchless dialog on enter
    if (pGUIWindow_BranchlessDialogue->keyboard_input_status != WINDOW_INPUT_IN_PROGRESS) {
        engine->_messageQueue->addMessageCurrentFrame(UIMSG_Escape, 0, 0);
        return;
    }
}

void startBranchlessDialogue(EvtOpcode opcode, std::optional<EvtContinuation> continuation) {
    if (!pGUIWindow_BranchlessDialogue) {
        animTimer->setPaused(true);
        gameTimer->setPaused(true);
        pGUIWindow_BranchlessDialogue = std::make_unique<GUIWindow_BranchlessDialogue>(opcode, continuation);
    }
}

void startMM6QuestionDialogue(MM6QuestionDialogue question) {
    if (pGUIWindow_BranchlessDialogue)
        throw std::logic_error("MM6 question dialogue is already open");
    startBranchlessDialogue(EVENT_InputString, std::nullopt);
    pGUIWindow_BranchlessDialogue->setMM6Question(std::move(question));
}

void releaseBranchlessDialogue() {
    assert(pGUIWindow_BranchlessDialogue);
    std::optional<EvtContinuation> continuation = pGUIWindow_BranchlessDialogue->continuation();
    pGUIWindow_BranchlessDialogue.reset();
    DialogueEnding();
    current_screen_type = SCREEN_GAME;
    if (continuation)
        runEvent(continuation->context, continuation->step); // Can open another dialogue, so nothing may run after it.
}

