project(taisei C)

cmake_minimum_required(VERSION 3.2)

include(CheckSymbolExists)

if(DEFINED TAISEI_DEBUG)
    # XXX: avoid using this unless you absolutely have to
    if(TAISEI_DEBUG)
        set(CMAKE_BUILD_TYPE "Debug")
    else()
        set(CMAKE_BUILD_TYPE "Release")
    endif()
endif()

if("${CMAKE_BUILD_TYPE}" STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release")
    #message(WARNING "CMAKE_BUILD_TYPE was not set, assuming ${CMAKE_BUILD_TYPE}.")
endif()

message(STATUS "Build configuration: ${CMAKE_BUILD_TYPE}")

set(BUILDSCRIPTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/scripts")

if(WIN32 OR APPLE)
	set(RELATIVE_DEFAULT ON)
else()
	set(RELATIVE_DEFAULT OFF)
endif()

option(RELATIVE "Use only relative paths to the executable and install everything in the same directory." ${RELATIVE_DEFAULT})
option(NO_AUDIO "Build without audio support" OFF)
option(DEBUG_USE_UBSAN "Enable the Undefined Behaviour Sanitizer (UBSan) in debug builds. Only disable if the compiler or target platform doesn't support it." ON)
option(DEBUG_USE_ASAN "Enable the Address Sanitizer (ASan) and leak detection in debug builds." OFF)
option(RELEASE_USE_LTO "Enable Link Time Optimization in release builds." ON)
option(USE_COTIRE "Use cotire (COmpile TIme REducer) to speed up builds." OFF)
option(PACKAGE_DATA "Package the game's assets into a compressed archive instead of bundling plain files." ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/scripts")

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

set(GAME_RESOURCES_DIRS
    "gfx"
    "sfx"
    "bgm"
    "shader"
    "models"
)

if(PACKAGE_DATA)
    set(GAME_RESOURCES_ZIP "${CMAKE_CURRENT_BINARY_DIR}/taisei-gamedata.zip")

    configure_file(
        "${BUILDSCRIPTS_DIR}/package_data.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/package_data.cmake"
        @ONLY)

    install(SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/package_data.cmake)
    install(FILES ${GAME_RESOURCES_ZIP} DESTINATION ${DATA_DIR})
else()
    foreach(dir ${GAME_RESOURCES_DIRS})
        install(DIRECTORY ${dir} DESTINATION ${DATA_DIR})
    endforeach()
endif()

# uninstall target
configure_file(
    "${BUILDSCRIPTS_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
