cmake_minimum_required(VERSION 3.24 FATAL_ERROR)

set(BIN_OPENENROTH_SOURCES
        OpenEnroth.cpp
        OpenEnrothOptions.cpp)

set(BIN_OPENENROTH_HEADERS
        OpenEnrothOptions.h)

if(OE_BUILD_PLATFORM STREQUAL "android")
    add_library(main SHARED)
    target_sources(main PUBLIC ${BIN_OPENENROTH_HEADERS} ${BIN_OPENENROTH_SOURCES})
    target_check_style(main)
    target_link_libraries(main PUBLIC application library_cli library_platform_main library_stack_trace)
    target_link_options(main PRIVATE "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/libmain.map")
else()
    if (WIN32)
        add_executable(OpenEnroth ${BIN_OPENENROTH_HEADERS} ${BIN_OPENENROTH_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/OpenEnroth.rc)

        install(TARGETS OpenEnroth)
    elseif(APPLE)
        # Set apple plist items and icon
        set(MACOSX_BUNDLE_BUNDLE_NAME "OpenEnroth")
        set(MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}")
        set(MACOSX_BUNDLE_COPYRIGHT "GPLv3")
        set(MACOSX_BUNDLE_GUI_IDENTIFIER "OpenEnroth")
        set(MACOSX_BUNDLE_ICON_FILE "OpenEnroth")  # Legacy .icns for pre-Tahoe macOS
        set(MACOSX_BUNDLE_INFO_STRING "Open reimplementation of Might and Magic 6 7 8 game engine.")
        set(MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_VERSION}")
        set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}")

        # Legacy .icns icon (for macOS < 26)
        set(OpenEnroth_ICON ${CMAKE_CURRENT_SOURCE_DIR}/OpenEnroth.icns)
        set_source_files_properties(${OpenEnroth_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

        # Assets.car for macOS 26+ (Tahoe)
        set(OpenEnroth_ASSETS_CAR ${CMAKE_CURRENT_SOURCE_DIR}/Assets.car)
        set_source_files_properties(${OpenEnroth_ASSETS_CAR} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

        add_executable(OpenEnroth MACOSX_BUNDLE ${OpenEnroth_ICON} ${OpenEnroth_ASSETS_CAR} ${BIN_OPENENROTH_HEADERS} ${BIN_OPENENROTH_SOURCES})
        set_target_properties(OpenEnroth PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in)

        # Atlas stages this repository's MM7 engine in its own helper slot.
        add_executable(MM7 MACOSX_BUNDLE ${OpenEnroth_ICON} ${OpenEnroth_ASSETS_CAR} ${BIN_OPENENROTH_HEADERS} ${BIN_OPENENROTH_SOURCES})
        set_target_properties(MM7 PROPERTIES
            MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
            MACOSX_BUNDLE_BUNDLE_NAME "Enroth Continuum MM7"
            MACOSX_BUNDLE_BUNDLE_VERSION "0.1.0"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "0.1.0"
            MACOSX_BUNDLE_LONG_VERSION_STRING "Enroth Continuum MM7 0.1.0"
            MACOSX_BUNDLE_GUI_IDENTIFIER "org.sklerosai.enrothcontinuum.mm7"
            MACOSX_BUNDLE_INFO_STRING "Personal Might and Magic VII runtime based on OpenEnroth."
            MACOSX_BUNDLE_COPYRIGHT "OpenEnroth and Enroth Continuum contributors (LGPL-3.0-or-later)")
        target_compile_definitions(MM7 PRIVATE EC_MM7_ONLY=1)
        target_link_libraries(MM7 PUBLIC application library_cli library_platform_main library_stack_trace)
        target_check_style(MM7)

        # Can't make install on OSX due to signing, dmg generation, etc
    else ()
        add_executable(OpenEnroth ${BIN_OPENENROTH_HEADERS} ${BIN_OPENENROTH_SOURCES})

        install(TARGETS OpenEnroth)
    endif()

    target_check_style(OpenEnroth)
    target_link_libraries(OpenEnroth PUBLIC application library_cli library_platform_main library_stack_trace)

    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT OpenEnroth)
endif()
