#pragma once

#include <cstdint>
#include <functional>

#include "GUI/GUIWindow.h"

class GUIWindow_MainMenu : public GUIWindow {
 public:
    GUIWindow_MainMenu();
    virtual ~GUIWindow_MainMenu();

    virtual void Update() override;

    void processMessage(UIMessageType messageType);

 protected:
    GUIButton *pBtnExit = nullptr;
    GUIButton *pBtnCredits = nullptr;
    GUIButton *pBtnLoad = nullptr;
    GUIButton *pBtnNew = nullptr;

    GraphicsImage *main_menu_background = nullptr;

    GraphicsImage *ui_mainmenu_new = nullptr;
    GraphicsImage *ui_mainmenu_load = nullptr;
    GraphicsImage *ui_mainmenu_credits = nullptr;
    GraphicsImage *ui_mainmenu_exit = nullptr;

 private:
    int _hoverFrame = 0;
    int _hoverStep = 1;
    int64_t _nextHoverStepTime = 0;
};
