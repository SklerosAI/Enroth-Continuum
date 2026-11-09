cmake_minimum_required(VERSION 3.27 FATAL_ERROR)

set(GUI_UI_SOURCES
        ItemGrid.cpp
        NPCTopics.cpp
        UISpellbook.cpp
        UIBooks.cpp
        UICharacter.cpp
        MM6CharacterScreen.cpp
        UIChest.cpp
        UICredits.cpp
        UIDialogue.cpp
        UIBranchlessDialogue.cpp
        UIGame.cpp
        UIGameOver.cpp
        UIHouses.cpp
        MM6HouseServiceData.cpp
        MM6RoomDescriptors.cpp
        MM6TransitionDialog.cpp
        MM6SaveLoadMenu.cpp
        UIInventory.cpp
        UIMainMenu.cpp
        UIPartyCreation.cpp
        UIPopup.cpp
        UIQuickReference.cpp
        UIRest.cpp
        UISaveLoad.cpp
        UISpell.cpp
        UIStatusBar.cpp
        UITransition.cpp
        UIMessageScroll.cpp)

set(GUI_UI_HEADERS
        ItemGrid.h
        NPCTopics.h
        UISpellbook.h
        UIBooks.h
        UICharacter.h
        MM6CharacterScreen.h
        UIChest.h
        UICredits.h
        UIDialogue.h
        UIBranchlessDialogue.h
        UIGame.h
        UIGameOver.h
        UIHouseEnums.h
        UIHouses.h
        MM6HouseServiceData.h
        MM6RoomDescriptors.h
        MM6TransitionDialog.h
        MM6SaveLoadMenu.h
        MainMenuResources.h
        UIInventory.h
        UIMainMenu.h
        UIPartyCreation.h
        UIPopup.h
        UIQuickReference.h
        UIRest.h
        UISaveLoad.h
        UISpell.h
        UIStatusBar.h
        UITransition.h
        UIMessageScroll.h)

add_library(gui_ui STATIC ${GUI_UI_SOURCES} ${GUI_UI_HEADERS})
target_check_style(gui_ui)

target_link_libraries(gui_ui
        PUBLIC
        gui_ui_books
        gui_ui_houses
        arcomage
        core_serialization
        engine_spells
        utility)

add_subdirectory(Books)
add_subdirectory(Houses)

if(OE_BUILD_TESTS)
    add_library(test_gui_ui_mm6_rooms OBJECT Tests/MM6HouseServiceData_ut.cpp Tests/MM6RoomDescriptors_ut.cpp Tests/MM6TransitionDialog_ut.cpp)
    target_link_libraries(test_gui_ui_mm6_rooms PUBLIC testing_unit gui_ui)
    target_check_style(test_gui_ui_mm6_rooms)
    target_link_libraries(OpenEnroth_UnitTest PUBLIC test_gui_ui_mm6_rooms)
endif()
