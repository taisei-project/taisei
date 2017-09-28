project(taisei C)
cmake_minimum_required(VERSION 3.3)

set(BUILDSCRIPTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/scripts")
set(MISC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/misc")
set(ICONS_DIR "${MISC_DIR}/icons")

list(APPEND CMAKE_MODULE_PATH "${BUILDSCRIPTS_DIR}/cmake-modules")
list(APPEND CMAKE_MODULE_PATH "${BUILDSCRIPTS_DIR}/cmake-modules/git")

include(GetGitRevisionDescription)

#
#   Set up version variables
#

set(TAISEI_VERSION_FALLBACK "1.1.1")

function(taisei_parse_version version_string)
    string(STRIP ${version_string} version_string)
    set(version_string_orig ${version_string})
    string(SUBSTRING ${version_string} 0 1 first_char)

    if(first_char STREQUAL "v")
        string(SUBSTRING ${version_string} 1 32 version_string)
    endif()

    string(REPLACE "+" "-" version_string ${version_string})
    string(REGEX REPLACE "-.*" "" version_string_tmp ${version_string})
    string(REGEX MATCH "-[0-9]+" version_string_extra ${version_string})

    set(TAISEI_VERSION ${version_string} PARENT_SCOPE)
    set(TAISEI_VERSION_FULL_STR "Taisei v${version_string}" PARENT_SCOPE)

    set(version_string ${version_string_tmp})

    string(REPLACE "." ";" version_string_tmp ${version_string})

    list(LENGTH version_string_tmp version_numelems)

    if(version_numelems GREATER 3)
        message(FATAL_ERROR "Too many dot-separated elements in version string ${version_string_orig}, please use the following format: [v]major[.minor[.patch]][-tweak[extrainfo]]")
    endif()

    while(version_numelems LESS 3)
        list(APPEND version_string_tmp 0)
        list(LENGTH version_string_tmp version_numelems)
    endwhile()

    list(GET version_string_tmp 0 major)
    list(GET version_string_tmp 1 minor)
    list(GET version_string_tmp 2 patch)

    set(TAISEI_VERSION_SIMPLE ${major}.${minor}.${patch} PARENT_SCOPE)

    set(TAISEI_VERSION_MAJOR ${major} PARENT_SCOPE)
    set(TAISEI_VERSION_MINOR ${minor} PARENT_SCOPE)
    set(TAISEI_VERSION_PATCH ${patch} PARENT_SCOPE)
    set(TAISEI_VERSION_TWEAK 0 PARENT_SCOPE)

    if(version_string_extra)
        string(SUBSTRING ${version_string_extra} 1 32 version_string_extra)
        set(TAISEI_VERSION_TWEAK ${version_string_extra} PARENT_SCOPE)
    endif()
endfunction()

if(TAISEI_VERSION_OVERRIDE)
    taisei_parse_version(${TAISEI_VERSION_OVERRIDE})
    message(STATUS "Version information manually overriden: v${TAISEI_VERSION}")
else()
    git_describe(TAISEI_VERSION --tags --match "v[0-9]*[!a-z]")

    if(TAISEI_VERSION STREQUAL "GIT-NOTFOUND")
        message(WARNING "Git is not installed. Version information will be less accurate.")
        set(TAISEI_VERSION ${TAISEI_VERSION_FALLBACK})
    elseif(TAISEI_VERSION STREQUAL "HEAD-HASH-NOTFOUND")
        message(WARNING "Source tree doesn't seem to be in a git repository. Version information will be less accurate.")
        set(TAISEI_VERSION ${TAISEI_VERSION_FALLBACK})
    endif()

    taisei_parse_version(${TAISEI_VERSION})
endif()

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
        install(FILES "${ICONS_DIR}/taisei.icns" DESTINATION "${RES_DIR}" RENAME "Taisei.icns")
    else()
        set(DATA_DIR "data")
    endif()
else()
	set(DATA_DIR "share/taisei")
    set(DOC_DIR "share/doc/taisei")

    install(DIRECTORY "xdg/" DESTINATION ".")
endif()

if(WIN32)
    set(DOC_FILES "STORY.txt" "COPYING" "misc/README_installed.txt")
    set(DOC_TMP_DIR "${CMAKE_CURRENT_BINARY_DIR}/win32docs")

    configure_file(
        "${BUILDSCRIPTS_DIR}/Win32FixDocs.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/Win32FixDocs.cmake"
        @ONLY)

    install(SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/Win32FixDocs.cmake")
    set(DOC_SRC_DIR "${DOC_TMP_DIR}")
else()
    set(DOC_SRC_DIR ".")
endif()

install(FILES "${DOC_SRC_DIR}/STORY.txt" DESTINATION "${DOC_DIR}")
install(FILES "${DOC_SRC_DIR}/COPYING" DESTINATION "${DOC_DIR}" RENAME "COPYING.txt")
install(FILES "${DOC_SRC_DIR}/misc/README_installed.txt" DESTINATION "${DOC_DIR}" RENAME "README.txt")

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
    install(DIRECTORY "resources/" DESTINATION ${DATA_DIR})
endif()

install(FILES ${MISC_DIR}/gamecontrollerdb/gamecontrollerdb.txt DESTINATION ${DATA_DIR})

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
