#include "GUI/UI/UIMainMenu.h"

#include <string>

#include "GUI/UI/MainMenuResources.h"

#include "Engine/EngineGlobals.h"
#include "Engine/Localization.h"
#include "Engine/Graphics/Renderer/Renderer.h"
#include "Engine/Graphics/Image.h"
#include "Engine/AssetsManager.h"
#include "Engine/Engine.h"

#include "GUI/GUIButton.h"
#include "GUI/GUIFont.h"
#include "GUI/GUIMessageQueue.h"

#include "Io/Mouse.h"

#include "Library/Platform/Interface/Platform.h"

static std::string hoverIcon(const MainMenuResources &resources, const MainMenuButtonResource &button, int frame) {
    if (resources.hoverFrameCount == 1)
        return std::string(button.icon);
    return fmt::format("start{:02}{}", frame, button.icon.back());
}

GUIWindow_MainMenu::GUIWindow_MainMenu() :
    GUIWindow(WINDOW_MAIN_MENU, {0, 0}, render->GetRenderDimensions()) {
    const MainMenuResources resources = mainMenuResources(engine->gameTitle());
    main_menu_background = assets->getIcon(resources.background);

    ui_mainmenu_new = assets->getIcon(resources.buttons[0].pressedIcon);
    ui_mainmenu_load = assets->getIcon(resources.buttons[1].pressedIcon);
    ui_mainmenu_credits = assets->getIcon(resources.buttons[2].pressedIcon);
    ui_mainmenu_exit = assets->getIcon(resources.buttons[3].pressedIcon);

    auto buttonSize = [&](int button) {
        Sizei size = resources.buttonSize.isEmpty() ? assets->getIcon(resources.buttons[button].icon)->size() : resources.buttonSize;
        return Sizei(size.w - 1, size.h - 1);
    };
    InputAction exitAction = resources.exitHasHotkey ? INPUT_ACTION_EXIT_GAME : INPUT_ACTION_INVALID;
    pBtnNew = CreateButton("MainMenu_NewGame", resources.buttons[0].position, buttonSize(0), BUTTON_TYPE_NORMAL, 0,
                           UIMSG_MainMenu_ShowPartyCreationWnd, 0, INPUT_ACTION_NEW_GAME, "", {ui_mainmenu_new});
    pBtnLoad = CreateButton("MainMenu_LoadGame", resources.buttons[1].position, buttonSize(1), BUTTON_TYPE_NORMAL, 0,
                            UIMSG_MainMenu_ShowLoadWindow, 1, INPUT_ACTION_LOAD_GAME, "", {ui_mainmenu_load});
    pBtnCredits = CreateButton("MainMenu_Credits", resources.buttons[2].position, buttonSize(2), BUTTON_TYPE_NORMAL, 0,
                               UIMSG_ShowCredits, 2, INPUT_ACTION_SHOW_CREDITS, "", {ui_mainmenu_credits});
    pBtnExit = CreateButton("MainMenu_ExitGame", resources.buttons[3].position, buttonSize(3), BUTTON_TYPE_NORMAL, 0,
                            UIMSG_ExitToWindows, 3, exitAction, "", {ui_mainmenu_exit});

    _nextHoverStepTime = platform->tickCount() + 500;
}

GUIWindow_MainMenu::~GUIWindow_MainMenu() {
    assets->releaseImage(main_menu_background);
}

void GUIWindow_MainMenu::Update() {
    render->DrawQuad2D(main_menu_background, {0, 0});

    Pointi pt = mouse->position();
    const MainMenuResources resources = mainMenuResources(engine->gameTitle());
    for (GUIButton *pButton : vButtons) {
        if (pButton->Contains(pt.x, pt.y) && pButton->msg_param < resources.buttons.size()) {
            const MainMenuButtonResource &button = resources.buttons[pButton->msg_param];
            render->DrawQuad2D(assets->getIcon(hoverIcon(resources, button, _hoverFrame)), button.position);
        }
    }

    // MM6 steps the hover animation at most once a frame, 500 ms after the menu opens and every 50 ms after that.
    if (resources.hoverFrameCount > 1 && platform->tickCount() >= _nextHoverStepTime) {
        _nextHoverStepTime = platform->tickCount() + 50;
        _hoverFrame += _hoverStep;
        if (_hoverFrame < 1) {
            _hoverStep = 1;
        } else if (_hoverFrame > resources.hoverFrameCount - 2) {
            _hoverStep = -1;
        }
    }
}

void GUIWindow_MainMenu::processMessage(UIMessageType message) {
    const MainMenuResources resources = mainMenuResources(engine->gameTitle());
    // Play the sound and change visual connected to the related button
    switch (message) {
    case UIMSG_MainMenu_ShowPartyCreationWnd:
        new OnButtonClick(resources.buttons[0].position, {0, 0}, pBtnNew);
        break;
    case UIMSG_MainMenu_ShowLoadWindow:
        new OnButtonClick(resources.buttons[1].position, {0, 0}, pBtnLoad);
        break;
    case UIMSG_ShowCredits:
        new OnButtonClick(resources.buttons[2].position, {0, 0}, pBtnCredits);
        break;
    case UIMSG_ExitToWindows:
        new OnButtonClick(resources.buttons[3].position, {0, 0}, pBtnExit);
        break;
    default:
        break;
    }
}
