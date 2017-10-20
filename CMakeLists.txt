project(taisei C)
cmake_minimum_required(VERSION 3.1)
cmake_policy(SET CMP0054 NEW)

set(PYTHON_VERSION_REQUIRED "3.4")

find_package(PkgConfig REQUIRED)
find_package(PythonInterp ${PYTHON_VERSION_REQUIRED})

if(PYTHON_VERSION_STRING AND PYTHON_VERSION_STRING VERSION_LESS ${PYTHON_VERSION_REQUIRED})
    # workaround for cmake retardation: if there's a bad entry in the cache, it won't bother trying to replace it.
    unset(PYTHON_EXECUTABLE CACHE)
    find_package(PythonInterp ${PYTHON_VERSION_REQUIRED})
endif()

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
        string(SUBSTRING ${version_string} 1 -1 version_string)
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
        string(SUBSTRING ${version_string_extra} 1 -1 version_string_extra)
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
    message(WARNING "You have TAISEI_DEBUG set, which overrides CMAKE_BUILD_TYPE. This is a deprecated option and you should not use it, unless maybe your cross-toolchain has a cmake wrapper that is doing something idiotic. Consider unsetting it with cmake -UTAISEI_DEBUG.")

    # XXX: avoid using this unless you absolutely have to
    if(TAISEI_DEBUG)
        set(CMAKE_BUILD_TYPE "Debug")
    else()
        set(CMAKE_BUILD_TYPE "Release")
    endif()
endif()

if("${CMAKE_BUILD_TYPE}" STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release")
endif()

message(STATUS ${TAISEI_VERSION_FULL_STR})
message(STATUS "Build configuration: ${CMAKE_BUILD_TYPE}")

if(WIN32 OR APPLE)
	set(RELATIVE_DEFAULT ON)
else()
	set(RELATIVE_DEFAULT OFF)
endif()

option(RELATIVE "Use only relative paths to the executable and install everything in the same directory." ${RELATIVE_DEFAULT})
option(USE_AUDIO "Build with audio support (SDL2_mixer)." ON)
option(USE_ZIP "Build with .zip package support." ON)
option(USE_CPACK "Add a 'package' target, used to prepare a release for binary distribution. Currently only used to create Windows installers. Requires CPack and NSIS." OFF)
option(USE_COTIRE "Use cotire (COmpile TIme REducer) to speed up builds." OFF)
option(DEBUG_USE_UBSAN "Enable the Undefined Behaviour Sanitizer (UBSan) in debug builds. Only disable if the compiler or target platform doesn't support it." ON)
option(DEBUG_USE_ASAN "Enable the Address Sanitizer (ASan) and leak detection in debug builds." OFF)
option(RELWITHDEBINFO_USE_DEBUG_FLAGS "Use debug flags in RelWithDebInfo builds (e.g. sanitizers)." OFF)
option(RELEASE_USE_LTO "Enable Link Time Optimization in release builds." ON)
option(PACKAGE_DATA "Package the game's assets into a compressed archive instead of bundling plain files. Requires USE_ZIP=ON." ON)
option(PACKAGE_DATA_LEANIFY "Optimize the assets archive for size. This process can be very slow. Requires Leanify (https://github.com/JayXon/Leanify) and PACKAGE_DATA=ON." OFF)
option(LINK_TO_LIBGL "Link to the OpenGL library instead of loading it at runtime. This is strongly discouraged, as it is not portable, and may not even work on some systems." OFF)
option(WERROR "Treat compiler warnings as errors." OFF)
option(FATALERRS "Abort compilation after first error is encountered." OFF)

if(APPLE)
    set(OSX_LIB_PATH "" CACHE STRING "Colon-separated list of paths from where required runtime libraries will be copied into the bundle.")
    set(OSX_TOOL_PATH "" CACHE STRING "Colon-separated list of paths from where macOS-specific library utilities (such as otool) can be found. This is prepended to PATH.")
    set(OSX_TOOL_PREFIX "" CACHE STRING "Names of macOS-specific tools are prefixed with this string (useful for cross-compiling).")
endif()

if(LINK_TO_LIBGL)
	find_package(OpenGL REQUIRED)
endif()

pkg_check_modules(SDL2 sdl2>=2.0.5 REQUIRED)
pkg_check_modules(SDL2_TTF SDL2_ttf REQUIRED)
pkg_check_modules(ZLIB zlib REQUIRED)
pkg_check_modules(PNG libpng>=1.5 REQUIRED)

if(USE_AUDIO)
	pkg_check_modules(SDL2_MIXER SDL2_mixer REQUIRED)
endif()

# PACKAGE_DATA means we requested packaging; PACKAGE_DATA_SUPPORTED means it is supported at all.
# So we should package if we requested it AND it is supported.
# We will not check for support if packaging is not requested.
set(PACKAGE_DATA_SUPPORTED ON)
if(PACKAGE_DATA)
	if(CMAKE_VERSION VERSION_LESS 3.3)
		message(WARNING "Your version of CMake is too old, so packaging of data will not be used. Upgrade CMake to at least version 3.3 to enable packaging or specify PACKAGE_DATA=OFF to avoid this warning.")
		set(PACKAGE_DATA_SUPPORTED OFF)
	endif()
endif()

# USE_ZIP means we would like to link to libzip; ZIP_SUPPORTED means libzip functionality is supported at all.
# So we shouldn't use ZIP in code if we disabled it OR it is not supported.
# We will not check for support if ZIP is disabled.
set(ZIP_SUPPORTED ON)
if(USE_ZIP)
	pkg_check_modules(ZIP libzip>=1.0)
	if(NOT ZIP_FOUND)
		message(WARNING "You have no suitable libzip, so packaging of data will not be used and ZIP support will be disabled. Install at least libzip 1.0 to enable ZIP support or specify USE_ZIP=ON to avoid this warning.")
		set(ZIP_SUPPORTED OFF)
	endif()
endif()

# Obviously, we should not package data if we're compiling without ZIP support.
if(NOT USE_ZIP OR NOT ZIP_SUPPORTED)
	if(NOT USE_ZIP AND PACKAGE_DATA AND PACKAGE_DATA_SUPPORTED)
		# It is not a hazardous situation, so we would not issue a warning, only status message.
		message(STATUS "You have disabled ZIP support, packaging of data will not be used.")
	endif()
	set(PACKAGE_DATA_SUPPORTED OFF)
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

if(PACKAGE_DATA AND PACKAGE_DATA_SUPPORTED)
    message(STATUS "Packaging of data enabled")
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
    message(STATUS "Packaging of data disabled")
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
