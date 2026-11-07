cmake_minimum_required(VERSION 3.27 FATAL_ERROR)

set(GUI_UI_HOUSES_SOURCES
        Tavern.cpp
        Temple.cpp
        Training.cpp
        Transport.cpp
        MM6TransportRules.cpp
        Bank.cpp
        Jail.cpp
        MagicGuild.cpp
        TownHall.cpp
        MercenaryGuild.cpp
        Shops.cpp)

set(GUI_UI_HOUSES_HEADERS
        Tavern.h
        Temple.h
        Training.h
        Transport.h
        MM6TransportRules.h
        VendorRefreshMod.h
        Bank.h
        Jail.h
        MagicGuild.h
        TownHall.h
        MercenaryGuild.h
        Shops.h)

add_library(gui_ui_houses STATIC ${GUI_UI_HOUSES_SOURCES} ${GUI_UI_HOUSES_HEADERS})
target_check_style(gui_ui_houses)

target_link_libraries(gui_ui_houses PUBLIC engine_graphics utility)

if(OE_BUILD_TESTS)
    add_library(test_gui_ui_houses_transport OBJECT Tests/MM6TransportRules_ut.cpp Tests/VendorRefreshMod_ut.cpp)
    target_link_libraries(test_gui_ui_houses_transport PUBLIC testing_unit gui_ui_houses)
    target_check_style(test_gui_ui_houses_transport)
    target_link_libraries(OpenEnroth_UnitTest PUBLIC test_gui_ui_houses_transport)
endif()
