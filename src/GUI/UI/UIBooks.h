#pragma once

#include <memory>
#include <string>
#include <string_view>

#include "GUI/GUIWindow.h"

class GraphicsImage;

enum class BookButtonAction {
    BOOK_ZOOM_IN = 0,
    BOOK_ZOOM_OUT = 1,
    BOOK_SCROLL_UP = 2,
    BOOK_SCROLL_DOWN = 3,
    BOOK_SCROLL_RIGHT = 4,
    BOOK_SCROLL_LEFT = 5,
    BOOK_NOTES_POTION = 6,
    BOOK_NOTES_FOUNTAIN = 7,
    BOOK_NOTES_OBELISK = 8,
    BOOK_NOTES_SEER = 9,
    BOOK_NOTES_MISC = 10,
    BOOK_NOTES_INSTRUCTORS = 11,
    BOOK_NEXT_PAGE = 12,
    BOOK_PREV_PAGE = 13,

    BOOK_BUTTON_FIRST = BOOK_ZOOM_IN,
    BOOK_BUTTON_LAST = BOOK_PREV_PAGE
};
using enum BookButtonAction;

class GUIWindow_Book : public GUIWindow {
 public:
    GUIWindow_Book();
    virtual ~GUIWindow_Book();

    void bookButtonClicked(BookButtonAction action);

 protected:
    void drawExitButton();

    /**
     * Creates an MM6 side tab in the tab column.
     *
     * @param y                         Top of the tab.
     * @param action                    Book button the tab presses.
     * @param inputAction               Key binding that also presses it.
     * @param label                     Hover text.
     * @param onArt                     The tab's `on` art.
     * @return                          The created button.
     */
    GUIButton *createMM6Tab(int y, BookButtonAction action, InputAction inputAction, std::string_view label, GraphicsImage *onArt);

    /**
     * Draws an MM6 side tab, the `on` art in the tab column or the `off` art three pixels to its right.
     *
     * @param y                         Top of the tab.
     * @param onArt                     The tab's `on` art.
     * @param offArt                    The tab's `off` art.
     * @param drawOn                    Whether to draw the `on` art.
     */
    void drawMM6Tab(int y, GraphicsImage *onArt, GraphicsImage *offArt, bool drawOn);

    bool isButtonPressed(BookButtonAction action) const;

    std::unique_ptr<GUIWindow> pChildBooksOverlay;

    GraphicsImage *ui_book_button8_off{ nullptr };
    GraphicsImage *ui_book_button8_on{ nullptr };
    GraphicsImage *ui_book_button7_off{ nullptr };
    GraphicsImage *ui_book_button7_on{ nullptr };
    GraphicsImage *ui_book_button6_off{ nullptr };
    GraphicsImage *ui_book_button6_on{ nullptr };
    GraphicsImage *ui_book_button5_off{ nullptr };
    GraphicsImage *ui_book_button5_on{ nullptr };
    GraphicsImage *ui_book_button4_off{ nullptr };
    GraphicsImage *ui_book_button4_on{ nullptr };
    GraphicsImage *ui_book_button3_off{ nullptr };
    GraphicsImage *ui_book_button3_on{ nullptr };
    GraphicsImage *ui_book_button2_off{ nullptr };
    GraphicsImage *ui_book_button2_on{ nullptr };
    GraphicsImage *ui_book_button1_off{ nullptr };
    GraphicsImage *ui_book_button1_on{ nullptr };

    GraphicsImage *ui_book_map_frame{ nullptr };
    GraphicsImage *ui_book_quest_div_bar{ nullptr };

    static constexpr int BOOK_BUTTON_PRESSED_FRAMES = 10; // How long a clicked book button stays drawn pressed.

    int _bookButtonClicked = 0; // Frames left to draw the pressed button, counts down from BOOK_BUTTON_PRESSED_FRAMES.
    BookButtonAction _bookButtonAction = BOOK_ZOOM_IN;

 private:
    /**
     * @offset 0x411AAA
     */
    void initializeFonts();
};


class GUIWindow_BooksButtonOverlay : public GUIWindow {
 public:
    GUIWindow_BooksButtonOverlay(Pointi position, Sizei dimensions, GUIButton *button, std::string_view hint = {});

    virtual void Update() override;

 private:
    GUIButton *_button = nullptr;
};
