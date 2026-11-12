#include "UIStatusBar.h"

#include <string>

#include "Engine/AssetsManager.h"
#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Localization.h"
#include "Engine/mm7_data.h"

#include "Engine/Graphics/Renderer/Renderer.h"

#include "GUI/GUIFont.h"
#include "GUI/MM6HudLayout.h"

#include "GUI/UI/UIGame.h"

const std::string &StatusBar::get() {
    if (_eventStatusExpireTime) {
        return _eventStatusString;
    } else {
        return _statusString;
    }
}

void StatusBar::draw() {
    render->DrawQuad2D(game_ui_statusbar, {0, 352});

    const std::string &status = get();
    if (status.empty())
        return;

    const int x = assets->pFontLucida->AlignText_Center(450, status) + 11;
    if (engine->gameTitle() == GameTitle::MM6) {
        Color color = _eventStatusExpireTime ? colorTable.PaleCanary : colorTable.White;
        GUIWindow::DrawText(assets->pFontLucida.get(), {x, mm6HudStatusTextY}, color, status, pPrimaryWindow->frameRect, 0, colorTable.Black);
    } else {
        GUIWindow::DrawText(assets->pFontLucida.get(), {x, 357}, uGameUIFontMain, status, pPrimaryWindow->frameRect, 0, uGameUIFontShadow);
    }
}

void StatusBar::drawForced(std::string_view str, Color color) {
    render->DrawQuad2D(game_ui_statusbar, {0, 352});
    const int y = engine->gameTitle() == GameTitle::MM6 ? mm6HudStatusTextY : 357;
    GUIWindow::DrawText(assets->pFontLucida.get(), { assets->pFontLucida->AlignText_Center(450, str) + 11, y}, color, str, pPrimaryWindow->frameRect);
}

void StatusBar::update() {
    // Was also checking that event timer is not stopped
    if (_eventStatusExpireTime && platform->tickCount() >= _eventStatusExpireTime) {
        _eventStatusExpireTime = 0;
    }
}

void StatusBar::setPermanent(std::string_view str) {
    if (str.length() > 0) {
        if (_eventStatusExpireTime == 0) {
            _statusString = str;
        }
    }
}

void StatusBar::clearPermanent() {
    _statusString.clear();
}

void StatusBar::clearAll() {
    _statusString.clear();
    clearEvent();
}

void StatusBar::setEvent(std::string_view str) {
    _eventStatusString = str;
    _eventStatusExpireTime = platform->tickCount() + EVENT_DURATION;
}

void StatusBar::setEventShort(std::string_view str) {
    _eventStatusString = str;
    _eventStatusExpireTime = platform->tickCount() + EVENT_DURATION_SHORT;
}

void StatusBar::clearEvent() {
    _eventStatusExpireTime = 0;
}

void StatusBar::nothingHere() {
    if (_eventStatusExpireTime == 0) {
        setEvent(LSTR_NOTHING_HERE);
    }
}
