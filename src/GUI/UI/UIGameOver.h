#pragma once

#include <cstdint>
#include <memory>

#include "Engine/MM6/MM6Finale.h"

#include "GUI/GUIWindow.h"

class GUIWindow_GameOver : public GUIWindow {
 public:
    explicit GUIWindow_GameOver(UIMessageType releaseEvent = UIMSG_OnGameOverWindowClose);
    virtual ~GUIWindow_GameOver();

    virtual void Update() override;

    virtual bool toggleAndTestFinished();

 protected:
    /**
     * Pauses the game and opens the window over the screen that `createScreen` draws.
     *
     * @param releaseEvent              Message posted when the window closes.
     * @param createScreen              Draws the window's background and returns it.
     */
    GUIWindow_GameOver(UIMessageType releaseEvent, std::unique_ptr<GraphicsImage> (*createScreen)());

    UIMessageType _releaseEvent = UIMSG_0;
    bool _showPopUp = false;
    std::unique_ptr<GraphicsImage> _winnerCert;
};

/**
 * MM6 ending screen. It takes a key or a click once it has been up for 5 seconds. The win screen then shows the
 * farewell box, which closes on the next one, the losing screen closes right away.
 */
class GUIWindow_MM6GameOver : public GUIWindow_GameOver {
 public:
    explicit GUIWindow_MM6GameOver(mm6::FinaleEnding ending);

    virtual void Update() override;

    virtual bool toggleAndTestFinished() override;

 private:
    mm6::FinaleEnding _ending;
    int64_t _acceptsInputAt = 0;
};
