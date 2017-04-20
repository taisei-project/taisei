project(taisei)

cmake_minimum_required(VERSION 2.6)

include(CheckSymbolExists)

if(WIN32 OR APPLE)
	set(RELATIVE_DEFAULT ON)
else()
	set(RELATIVE_DEFAULT OFF)
endif()

option(RELATIVE "Use only relative paths to the executable and install everything in the same directory." ${RELATIVE_DEFAULT})
option(USE_SDL2_PATHS "Use the SDL2 filesystem API to determine where to save settings." ON)
option(NO_AUDIO "Build without audio support" OFF)
option(DEBUG_USE_UBSAN "Enable the Undefined Behaviour Sanitizer (UBSan) in debug builds. Only disable if the compiler or target platform doesn't support it." ON)
option(DEBUG_USE_ASAN "Enable the Address Sanitizer (ASan) and leak detection in debug builds." OFF)
option(RELEASE_USE_LTO "Enable Link Time Optimization in release builds." ON)
option(USE_COTIRE "Use cotire (COmpile TIme REducer) to speed up builds" OFF)

add_subdirectory(src)

if(RELATIVE)
    add_definitions(-DRELATIVE)

    if(APPLE)
        set(RES_DIR "Taisei.app/Contents/Resources")
        set(DATA_DIR "${RES_DIR}/data")
        install(FILES "OSX-iconset.icns" DESTINATION "${RES_DIR}" RENAME "Taisei.icns")
    else()
	   set(DATA_DIR "data")
    endif()

    install(FILES "story.txt" DESTINATION .)
else()
	set(DATA_DIR "share/taisei")

    install(FILES "taisei.desktop" DESTINATION "share/applications")
    install(FILES "taisei.png" DESTINATION "share/icons/hicolor/128x128/apps")
    install(FILES "story.txt" DESTINATION ${DATA_DIR})
endif()

install(DIRECTORY gfx DESTINATION ${DATA_DIR}
	FILES_MATCHING PATTERN "*.png")
install(DIRECTORY gfx DESTINATION ${DATA_DIR}
	FILES_MATCHING PATTERN "*.ttf")
install(DIRECTORY gfx DESTINATION ${DATA_DIR}
    FILES_MATCHING PATTERN "*.ani")
install(DIRECTORY sfx DESTINATION ${DATA_DIR})
install(DIRECTORY bgm DESTINATION ${DATA_DIR})
install(DIRECTORY shader DESTINATION ${DATA_DIR})
install(DIRECTORY models DESTINATION ${DATA_DIR}
	FILES_MATCHING PATTERN "*.obj")

# uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
