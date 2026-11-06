#include "GUI/UI/UIMainMenu.h"
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

GUIWindow_MainMenu::GUIWindow_MainMenu() :
    GUIWindow(WINDOW_MAIN_MENU, {0, 0}, render->GetRenderDimensions()) {
    const MainMenuResources resources = mainMenuResources(engine->gameTitle());
    main_menu_background = assets->getIcon(resources.background);

    ui_mainmenu_new = assets->getIcon(resources.buttons[0].icon);
    ui_mainmenu_load = assets->getIcon(resources.buttons[1].icon);
    ui_mainmenu_credits = assets->getIcon(resources.buttons[2].icon);
    ui_mainmenu_exit = assets->getIcon(resources.buttons[3].icon);

    pBtnNew = CreateButton("MainMenu_NewGame", resources.buttons[0].position, ui_mainmenu_new->size(), BUTTON_TYPE_NORMAL, 0,
                           UIMSG_MainMenu_ShowPartyCreationWnd, 0, INPUT_ACTION_NEW_GAME, "", {ui_mainmenu_new});
    pBtnLoad = CreateButton("MainMenu_LoadGame", resources.buttons[1].position, ui_mainmenu_load->size(), BUTTON_TYPE_NORMAL, 0,
                            UIMSG_MainMenu_ShowLoadWindow, 1, INPUT_ACTION_LOAD_GAME, "", {ui_mainmenu_load});
    pBtnCredits = CreateButton("MainMenu_Credits", resources.buttons[2].position, ui_mainmenu_credits->size(), BUTTON_TYPE_NORMAL, 0,
                               UIMSG_ShowCredits, 2, INPUT_ACTION_SHOW_CREDITS, "", {ui_mainmenu_credits});
    pBtnExit = CreateButton("MainMenu_ExitGame", resources.buttons[3].position, ui_mainmenu_exit->size(), BUTTON_TYPE_NORMAL, 0,
                            UIMSG_ExitToWindows, 3, INPUT_ACTION_EXIT_GAME, "", {ui_mainmenu_exit});
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
            render->DrawQuad2D(assets->getIcon(button.icon), button.position);
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
