#include "GUI/UI/UIGameOver.h"

#include <memory>

#include "GUI/GUIMessageQueue.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Localization.h"
#include "Engine/Timer.h"
#include "Engine/AssetsManager.h"
#include "Engine/Graphics/Renderer/Renderer.h"

#include "Library/Platform/Interface/Platform.h"

#include "Application/GameOver.h"

GUIWindow_GameOver::GUIWindow_GameOver(UIMessageType releaseEvent) : GUIWindow_GameOver(releaseEvent, &CreateWinnerCertificate) {
    this->sHint = fmt::format(
        "{}\n \n{}\n \n{}",
        localization->str(LSTR_CONGRATULATIONS_ADVENTURER),
        localization->str(LSTR_WE_HOPE_THAT_YOUVE_ENJOYED_PLAYING_MIGHT),
        localization->str(LSTR_THE_MIGHT_AND_MAGIC_VII_DEVELOPMENT_TEAM));
}

void GUIWindow_GameOver::Update() {
    // draw winners certificate background
    assert(_winnerCert);
    render->DrawQuad2D(_winnerCert.get(), {0, 0});

    // draw pop up box
    if (_showPopUp) {
        Recti popupRect(120, 140, 400, 100);
        DrawMessageBox(popupRect, fmt::format("{}\n \n{}", sHint, localization->str(LSTR_PRESS_ESCAPE)));
    }
}

GUIWindow_GameOver::~GUIWindow_GameOver() {
    engine->_messageQueue->addMessageCurrentFrame(_releaseEvent, 0, 0);

    current_screen_type = prev_screen_type;
    GameOverNoSound = false;
    gameTimer->setPaused(false);
}

bool GUIWindow_GameOver::toggleAndTestFinished() {
    if (_showPopUp) return true;

    _showPopUp = true;
    return false;
}

GUIWindow_GameOver::GUIWindow_GameOver(UIMessageType releaseEvent, std::unique_ptr<GraphicsImage> (*createScreen)())
    : GUIWindow(WINDOW_GAME_OVER, {0, 0}, render->GetRenderDimensions()), _releaseEvent(releaseEvent) {
    gameTimer->setPaused(true);
    prev_screen_type = current_screen_type;
    current_screen_type = SCREEN_GAMEOVER_WINDOW;
    GameOver_Setup();
    _winnerCert = createScreen();
}

GUIWindow_MM6GameOver::GUIWindow_MM6GameOver(mm6::FinaleEnding ending)
    : GUIWindow_GameOver(UIMSG_OnGameOverWindowClose, ending == mm6::FINALE_ENDING_WIN ? &CreateMM6WinScreen : &CreateMM6LoseScreen),
      _ending(ending), _acceptsInputAt(platform->tickCount() + 5000) {
    this->sHint = fmt::format("{}\n \n{}\n \n{}", localization->str(static_cast<LstrId>(151)), localization->str(static_cast<LstrId>(118)),
                              localization->str(static_cast<LstrId>(167)));
}

void GUIWindow_MM6GameOver::Update() {
    render->DrawQuad2D(_winnerCert.get(), {0, 0});
    if (_showPopUp) {
        Recti popupRect(120, 140, 400, 100);
        DrawMessageBox(popupRect, sHint);
    }
}

bool GUIWindow_MM6GameOver::toggleAndTestFinished() {
    if (platform->tickCount() < _acceptsInputAt)
        return false;
    if (_ending == mm6::FINALE_ENDING_LOSE || _showPopUp)
        return true;
    _showPopUp = true;
    return false;
}
