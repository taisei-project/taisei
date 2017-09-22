project(taisei C)
cmake_minimum_required(VERSION 3.2)

set(BUILDSCRIPTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/scripts")

list(APPEND CMAKE_MODULE_PATH "${BUILDSCRIPTS_DIR}/cmake-modules")
list(APPEND CMAKE_MODULE_PATH "${BUILDSCRIPTS_DIR}/cmake-modules/git")

include(GetGitRevisionDescription)

#
#   Set up version variables
#

git_describe(TAISEI_VERSION)
set(TAISEI_VERSION_FALLBACK "1.1")

if(TAISEI_VERSION STREQUAL "GIT-NOTFOUND")
    message(WARNING "Git is not installed. Version information will be less accurate.")
    set(TAISEI_VERSION "")
elseif(TAISEI_VERSION STREQUAL "HEAD-HASH-NOTFOUND")
    message(WARNING "Source tree doesn't seem to be in a git repository. Version information will be less accurate.")
    set(TAISEI_VERSION "")
endif()

if(TAISEI_VERSION)
    git_revision(TAISEI_REVISION)
    string(SUBSTRING ${TAISEI_VERSION} 1 32 TAISEI_VERSION) # erase the leading v
    set(TAISEI_VERSION_FULL_STR "Taisei v${TAISEI_VERSION} (rev. ${TAISEI_REVISION})")
else()
    set(TAISEI_VERSION "${TAISEI_VERSION_FALLBACK}")
    set(TAISEI_REVISION 0)
    set(TAISEI_VERSION_FULL_STR "Taisei v${TAISEI_VERSION}")
endif()

string(REGEX REPLACE "-.*" "" VERSION_TMP ${TAISEI_VERSION})
string(REPLACE "." ";" VERSION_TMP ${VERSION_TMP})

list(LENGTH VERSION_TMP VERSION_TMP_LEN)

while(VERSION_TMP_LEN LESS 3)
    list(APPEND VERSION_TMP 0)
    list(LENGTH VERSION_TMP VERSION_TMP_LEN)
endwhile()

list(GET VERSION_TMP 0 TAISEI_VERSION_MAJOR)
list(GET VERSION_TMP 1 TAISEI_VERSION_MINOR)
list(GET VERSION_TMP 2 TAISEI_VERSION_PATCH)

# Let's never use letters in version numbers again, ok?
string(REGEX MATCH "[a-z]" PATCH_CHAR ${TAISEI_VERSION_MINOR})
if(PATCH_CHAR)
    string(FIND "abcdefghjklmnopqrstuvwxyz" ${PATCH_CHAR} PATCH_NUM)
    math(EXPR PATCH_NUM "${PATCH_NUM} + 1")

    if(PATCH_NUM)
        math(EXPR TAISEI_VERSION_PATCH "${TAISEI_VERSION_PATCH} + ${PATCH_NUM}")
        string(REGEX MATCH "[0-9]+" TAISEI_VERSION_MINOR ${TAISEI_VERSION_MINOR})
    endif()
endif()

set(TAISEI_VERSION_SIMPLE ${TAISEI_VERSION_MAJOR}.${TAISEI_VERSION_MINOR}.${TAISEI_VERSION_PATCH})

#
#   End of the version insanity
#

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

message(STATUS ${TAISEI_VERSION_FULL_STR})
message(STATUS "Build configuration: ${CMAKE_BUILD_TYPE}")

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
option(PACKAGE_DATA_LEANIFY "Optimize the assets archive for size. This process can be very slow. Requires Leanify (https://github.com/JayXon/Leanify) and PACKAGE_DATA=ON." OFF)
option(USE_CPACK "Add a 'package' target, used to prepare a release for binary distribution. Currently only used to create Windows installers. Requires CPack and NSIS." OFF)

if(APPLE)
    option(OSX_LIB_PATH "Colon-separated list of paths from where required runtime libraries will be copied into the bundle." "")
    option(OSX_TOOL_PATH "Colon-separated list of paths from where macOS-specific library utilities (such as otool) can be found. This is prepended to PATH." "")
    option(OSX_TOOL_PREFIX "Names of macOS-specific tools are prefixed with this string (useful for cross-compiling)." "")
endif()

add_subdirectory(src)

if(RELATIVE)
    add_definitions(-DRELATIVE)
    set(DOC_DIR .)

    if(APPLE)
        set(RES_DIR "Taisei.app/Contents/Resources")
        set(DATA_DIR "${RES_DIR}/data")
        install(FILES "OSX-iconset.icns" DESTINATION "${RES_DIR}" RENAME "Taisei.icns")
    else()
        set(DATA_DIR "data")
    endif()
else()
	set(DATA_DIR "share/taisei")
    set(DOC_DIR "share/doc/taisei")

    install(FILES "taisei.desktop" DESTINATION "share/applications")
    install(FILES "taisei.png" DESTINATION "share/icons/hicolor/128x128/apps")
endif()

install(FILES "STORY.txt" DESTINATION "${DOC_DIR}")
install(FILES "COPYING" DESTINATION "${DOC_DIR}" RENAME "COPYING.txt")
install(FILES "README_installed.txt" DESTINATION "${DOC_DIR}" RENAME "README.txt")

if(WIN32)
    set(DOC_FILES "STORY.txt" "COPYING.txt" "README.txt")

    configure_file(
        "${BUILDSCRIPTS_DIR}/Win32FixDocs.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/Win32FixDocs.cmake"
        @ONLY)

    install(SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/Win32FixDocs.cmake")
endif()

set(GAME_RESOURCES_DIRS
    "gfx"
    "sfx"
    "bgm"
    "shader"
    "models"
)

if(PACKAGE_DATA)
    set(GAME_RESOURCES_ZIP "${CMAKE_CURRENT_BINARY_DIR}/00-taisei.zip")

    configure_file(
        "${BUILDSCRIPTS_DIR}/package_data.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/package_data.cmake"
        @ONLY)

    install(SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/package_data.cmake")

    if(PACKAGE_DATA_LEANIFY)
        find_program(LEANIFY_COMMAND "leanify")

        if(NOT LEANIFY_COMMAND)
            message(FATAL_ERROR "'leanify' not found. Either install it from https://github.com/JayXon/Leanify or pass -DPACKAGE_DATA_LEANIFY=OFF to CMake.")
        else()
            configure_file(
                "${BUILDSCRIPTS_DIR}/leanify_data.cmake.in"
                "${CMAKE_CURRENT_BINARY_DIR}/leanify_data.cmake"
                @ONLY)

            install(SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/leanify_data.cmake)
        endif()
    endif()

    install(FILES ${GAME_RESOURCES_ZIP} DESTINATION ${DATA_DIR})
else()
    foreach(dir ${GAME_RESOURCES_DIRS})
        install(DIRECTORY ${dir} DESTINATION ${DATA_DIR})
    endforeach()
endif()

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/gamecontrollerdb/gamecontrollerdb.txt DESTINATION ${DATA_DIR})

if(WIN32)
    # XXX: this is supposedly for dynamic-link builds, untested due to MXE being an ass
    include(InstallRequiredSystemLibraries)
endif()

if(USE_CPACK)
    include("${BUILDSCRIPTS_DIR}/CPackSettings.cmake")
endif()

# uninstall target
configure_file(
    "${BUILDSCRIPTS_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
