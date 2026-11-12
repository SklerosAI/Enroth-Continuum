#include <algorithm>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "GameOver.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/AssetsManager.h"
#include "Engine/Resources/EngineFileSystem.h"
#include "Engine/Resources/ResourceManager.h"
#include "Engine/Graphics/Renderer/Renderer.h"
#include "Engine/Graphics/Image.h"
#include "Engine/Localization.h"
#include "Engine/Party.h"
#include "Engine/mm7_data.h"

#include "GUI/GUIFont.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/UIHouses.h"

#include "Media/Audio/AudioPlayer.h"

#include "Core/Time/Time.h"

#include "Library/Image/Pcx.h"


//----- (004BF91E) --------------------------------------------------------
void GameOver_Setup() {
    engineFlags &= ~ENGINE_ESCAPE_ENABLED;
    GameOverNoSound = true;
    pAudioPlayer->stopSounds();

    // break out of house and dialogue
    window_SpeakInHouse = nullptr;
    pDialogueWindow = nullptr;
}

std::unique_ptr<GraphicsImage> CreateWinnerCertificate() {
    render->Present();
    render->BeginScene2D();
    GraphicsImage *background = assets->getIcon("winbg.pcx");
    render->DrawQuad2D(background, {0, 0});

    std::unique_ptr<GUIWindow> tempwindow_SpeakInHouse = std::make_unique<GUIWindow>(WINDOW_WINNER_CERTIFICATE, Pointi{ 0, 0 }, render->GetRenderDimensions());
    Recti frameRect(75, 60, 469, 338);
    std::unique_ptr<GUIFont> pFont = GUIFont::LoadFont("endgame.fnt");

    std::string pInString;
    if (pParty->isPartyGood()) {
        pInString = localization->str(LSTR_GOOD_ENDING);
    } else if (pParty->isPartyEvil()) {
        pInString = localization->str(LSTR_EVIL_ENDING);
    } else {
        assert(false);
    }

    Duration play_time = pParty->GetPlayingTime() - gameStartingTime;

    int totalDays = play_time.days();
    if (!totalDays) totalDays = 1;

    LongCivilDuration duration = play_time.toLongCivilDuration();
    int years = duration.years;
    int months = duration.months;
    int days = duration.days;

    GUIWindow::DrawTitleText(
        pFont.get(), 1, 0x23, colorTable.Black, localization->str(LSTR_CONGRATULATIONS), 3, frameRect
    );
    uint64_t v23 = 0ull;
    int v20 = 0;
    for (int i = 0; i < 4; i++) {
        GUIWindow::DrawTitleText(
            pFont.get(), 1,
            i * (pFont->GetHeight() - 2) + pFont->GetHeight() + 46,
            colorTable.Black,
            localization->format(
                LSTR_S_THE_LEVEL_U_S,
                pParty->pCharacters[i].name,
                pParty->pCharacters[i].GetBaseLevel(),
                localization->className(pParty->pCharacters[i].classType)),
            3, frameRect);
        v23 += pParty->pCharacters[i].experience;
    }
    v23 = (int64_t)v23 / totalDays;
    std::string v6 = pFont->WrapText(pInString, frameRect.w, 12);
    GUIWindow::DrawTitleText(pFont.get(), 1, 5 * (pFont->GetHeight() + 11), colorTable.Black, v6, 0, frameRect);

    std::string v7 = localization->str(LSTR_DAY_CAPITALIZED);
    if (days != 1) v7 = localization->str(LSTR_DAYS);

    std::string v8 = localization->str(LSTR_MONTH);
    if (months != 1) v8 = localization->str(LSTR_MONTHS);

    std::string v9 = localization->str(LSTR_YEAR);
    if (years != 1) v9 = localization->str(LSTR_YEARS);

    GUIWindow::DrawTitleText(
        pFont.get(), 1, frameRect.h - 2 * pFont->GetHeight() - 5, colorTable.Black,
        fmt::format("{} {} {}, {} {}, {} {} ", localization->str(LSTR_TOTAL_TIME), years, v9, months, v8, days, v7), 3, frameRect);

    GUIWindow::DrawTitleText(pFont.get(), 1, frameRect.h, colorTable.Black,
        localization->format(LSTR_YOUR_SCORE_LU, v23), 3, frameRect);
    engineFlags |= ENGINE_ESCAPE_ENABLED;

    // flush draw buffer so cert is drawn
    render->DrawTwodVerts();
    render->EndLines2D();
    render->EndTextNew();

    RgbaImage pixels = render->MakeFullScreenshot();
    ufs->write("MM7_Win.Pcx", pcx::encode(pixels));
    std::unique_ptr<GraphicsImage> result = GraphicsImage::Create(std::move(pixels));

    assets->releaseImage(background);
    background = nullptr;

    return result;
}

/**
 * @param font                          Font the text is drawn in.
 * @param fileName                      Text file in the events archive.
 * @param frameRect                     Frame the text is drawn in.
 * @return                              The file's text, wrapped to the frame.
 */
static std::string mm6EndingText(GUIFont *font, std::string_view fileName, const Recti &frameRect) {
    std::string text(engine->resources()->eventsData(fileName).str());
    std::ranges::replace(text, '\0', '\n');
    return font->WrapText(text, frameRect.w, 12);
}

/**
 * @param background                    Background icon the screen was drawn over, released here.
 * @return                              Everything drawn since `BeginScene2D`, as an image.
 */
static std::unique_ptr<GraphicsImage> captureMM6EndingScreen(GraphicsImage *background) {
    engineFlags |= ENGINE_ESCAPE_ENABLED; // GameOver_Setup turned it off, and party creation needs it.
    render->DrawTwodVerts();
    render->EndLines2D();
    render->EndTextNew();
    RgbaImage pixels = render->MakeFullScreenshot();
    assets->releaseImage(background);
    return GraphicsImage::Create(std::move(pixels));
}

std::unique_ptr<GraphicsImage> CreateMM6WinScreen() {
    render->Present();
    render->BeginScene2D();
    GraphicsImage *background = assets->getIcon("winbg.pcx");
    render->DrawQuad2D(background, {0, 0});
    const Recti frameRect(75, 60, 495, 338);
    std::unique_ptr<GUIFont> font = GUIFont::LoadFont("endgame.fnt");

    const Duration playTime = pParty->GetPlayingTime() - gameStartingTime;
    const int totalDays = std::max(1, playTime.days());
    const LongCivilDuration duration = playTime.toLongCivilDuration();

    GUIWindow::DrawTitleText(font.get(), 1, 35, colorTable.Black, localization->str(static_cast<LstrId>(9)), 3, frameRect); // "Congratulations!"
    int64_t experience = 0;
    for (int i = 0; i < 4; i++) {
        const Character &character = pParty->pCharacters[i];
        GUIWindow::DrawTitleText(font.get(), 1, i * (font->GetHeight() - 2) + font->GetHeight() + 46, colorTable.Black,
                                 localization->format(static_cast<LstrId>(129), character.name, character.GetBaseLevel(),
                                                      localization->className(character.classType)), 3, frameRect);
        experience += character.experience;
    }
    GUIWindow::DrawTitleText(font.get(), 1, 5 * (font->GetHeight() + 7), colorTable.Black, mm6EndingText(font.get(), "win.str", frameRect), 0,
                             frameRect);

    const std::string &years = localization->str(static_cast<LstrId>(duration.years == 1 ? 245 : 132));
    const std::string &months = localization->str(static_cast<LstrId>(duration.months == 1 ? 146 : 148));
    const std::string &days = localization->str(static_cast<LstrId>(duration.days == 1 ? 56 : 57));
    GUIWindow::DrawTitleText(font.get(), 1, frameRect.h - 2 * font->GetHeight() - 5, colorTable.Black,
                             fmt::format("{} {} {}, {} {}, {} {} ", localization->str(static_cast<LstrId>(37)), duration.years, years,
                                         duration.months, months, duration.days, days), 3, frameRect);
    GUIWindow::DrawTitleText(font.get(), 1, frameRect.h, colorTable.Black,
                             localization->format(static_cast<LstrId>(94), experience / totalDays), 3, frameRect); // "Your score: %lu"

    std::unique_ptr<GraphicsImage> result = captureMM6EndingScreen(background);
    ufs->write("MM6_Win.Pcx", pcx::encode(result->rgba()));
    return result;
}

std::unique_ptr<GraphicsImage> CreateMM6LoseScreen() {
    render->Present();
    render->BeginScene2D();
    GraphicsImage *background = assets->getIcon("losebg.pcx");
    render->DrawQuad2D(background, {0, 0});
    const Recti frameRect(75, 60, 495, 338);
    std::unique_ptr<GUIFont> font = GUIFont::LoadFont("endgame.fnt");
    GUIWindow::DrawTitleText(font.get(), 1, 35, colorTable.Black, localization->str(static_cast<LstrId>(9)), 3, frameRect);
    GUIWindow::DrawTitleText(font.get(), 1, 110, colorTable.Black, mm6EndingText(font.get(), "lose.str", frameRect), 0, frameRect);
    return captureMM6EndingScreen(background);
}
