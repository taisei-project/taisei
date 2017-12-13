include(CheckSymbolExists)
include(CheckAndSetFlags)

if(LINK_TO_LIBGL)
	add_definitions(-DLINK_TO_LIBGL)
endif()

if(NOT USE_ZIP OR NOT ZIP_SUPPORTED)
	message(STATUS "ZIP support disabled")
	add_definitions(-DDISABLE_ZIP)
else()
	message(STATUS "ZIP support enabled")
endif()

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/version_auto.c.in"
	"${CMAKE_CURRENT_BINARY_DIR}/version_auto.c"
)

check_symbol_exists(_POSIX_VERSION "unistd.h" POSIX)
check_symbol_exists(__STDC_NO_COMPLEX__ "unistd.h" COMPLEX_MISSING)
check_symbol_exists(__STDC_NO_VLA__ "unistd.h" VLA_MISSING)

# This doesn't work on gcc...
# check_symbol_exists(_mm_crc32_u8 "immintrin.h" HAVE_INTEL_INTRIN)

unset(HAVE_INTEL_INTRIN CACHE)

check_c_source_compiles("
#include <immintrin.h>
__attribute__((target(\"sse4.2\")))
int main(int argc, char **argv) {
	return _mm_crc32_u8(argc, 42);
}
" HAVE_INTEL_INTRIN)

if(COMPLEX_MISSING OR VLA_MISSING)
	message(FATAL_ERROR "Your C implementation needs to support complex numbers and variable length arrays.")
endif()

set(SRCs
	main.c
	cli.c
	log.c
	util.c
	taiseigl.c
	random.c
	config.c
	color.c
	recolor.c
	gamepad.c
	stage.c
	stagedraw.c
	stagetext.c
	stageobjects.c
	replay.c
	global.c
	events.c
	player.c
	aniplayer.c
	projectile.c
	progress.c
	enemy.c
	item.c
	list.c
	refs.c
	hashtable.c
	objectpool.c
	# objectpool_fake.c
	objectpool_util.c
	boss.c
	plrmodes.c
	plrmodes/marisa.c
	plrmodes/marisa_a.c
	plrmodes/marisa_b.c
	plrmodes/youmu.c
	plrmodes/youmu_a.c
	plrmodes/youmu_b.c
	laser.c
	dialog.c
	fbo.c
	vbo.c
	stageutils.c
	matrix.c
	video.c
	transition.c
	color.c
	difficulty.c
	audio_common.c
	hirestime.c
	menu/menu.c
	menu/mainmenu.c
	menu/options.c
	menu/stageselect.c
	menu/replayview.c
	menu/ingamemenu.c
	menu/gameovermenu.c
	menu/savereplay.c
	menu/difficultyselect.c
	menu/charselect.c
	menu/stagepractice.c
	menu/submenus.c
	menu/spellpractice.c
	menu/common.c
	stages/stage1.c
	stages/stage1_events.c
	stages/stage2.c
	stages/stage2_events.c
	stages/stage3.c
	stages/stage3_events.c
	stages/stage4.c
	stages/stage4_events.c
	stages/stage5.c
	stages/stage5_events.c
	stages/stage6.c
	stages/stage6_events.c
	ending.c
	credits.c
	resource/resource.c
	resource/texture.c
	resource/animation.c
	resource/font.c
	resource/shader.c
	resource/model.c
	resource/postprocess.c
	rwops/rwops_dummy.c
	rwops/rwops_zlib.c
	rwops/rwops_segment.c
	rwops/rwops_autobuf.c
	vfs/private.c
	vfs/public.c
	vfs/setup.c
	vfs/pathutil.c
	vfs/union.c
	vfs/union_public.c
	vfs/vdir.c
	vfs/syspath_public.c
	vfs/zipfile_public.c
	version.c
	"${CMAKE_CURRENT_BINARY_DIR}/version_auto.c"
)

if(USE_INTEL_INTRIN AND HAVE_INTEL_INTRIN)
	set(SRCs ${SRCs}
		util_sse42.c
	)
	add_definitions(-DHAVE_INTEL_INTRIN)
	set_property(SOURCE util_sse42.c APPEND_STRING PROPERTY COMPILE_FLAGS "-msse4.2")
endif()

if(USE_ZIP AND ZIP_SUPPORTED)
	set(SRCs ${SRCs}
		rwops/rwops_zipfile.c
		vfs/zipfile.c
		vfs/zippath.c
	)
else()
	set(SRCs ${SRCs}
		vfs/zipfile_null.c
	)
endif()

if(WIN32)
	set(SRCs ${SRCs}
		windows_misc.c
	)
endif()

if(POSIX)
	set(SRCs ${SRCs}
		vfs/syspath_posix.c
		rwops/rwops_pipe_posix.c
	)
elseif(WIN32)
	set(SRCs ${SRCs}
		vfs/syspath_win32.c
		rwops/rwops_pipe_null.c
	)
	set(LIBs ${LIBs}
		shlwapi
	)
else()
	set(SRCs ${SRCs}
		vfs/syspath_posix.c		# eeehh, maybe it'll work ¯\_(ツ)_/¯
		rwops/rwops_pipe_null.c
	)
endif()

if(USE_AUDIO)
	set(SRCs ${SRCs}
		audio_mixer.c
		resource/bgm_mixer.c
		resource/sfx_mixer.c
	)
else()
	set(SRCs ${SRCs}
		audio_null.c
		resource/bgm_null.c
		resource/sfx_null.c
	)
endif()

if(WIN32)
	set(DEBUG_FLAGS "-gstabs")
else()
	set(DEBUG_FLAGS "-ggdb")
endif()

set(DEBUG_FLAGS "${DEBUG_FLAGS} -fno-omit-frame-pointer")
set(RELEASE_FLAGS "-DNDEBUG")

set(DEBUG_BUILD FALSE)		# "Debug build" is a build where ${DEBUG_FLAGS} are used.
set(RELEASE_BUILD FALSE)	# "Release build" is a build where ${RELEASE_FLAGS} are used.

if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug" OR ("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo" AND RELWITHDEBINFO_USE_DEBUG_FLAGS))
	set(DEBUG_BUILD TRUE)
endif()

if("${CMAKE_BUILD_TYPE}" STREQUAL "Release" OR "${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
	set(RELEASE_BUILD TRUE)
endif()

if(DEBUG_USE_UBSAN)
	CHECK_AND_SET_FLAGS(COMPILER_SUPPORTS_UBSAN "-fsanitize=undefined" DEBUG_FLAGS "-fsanitize=undefined")
endif()

if(DEBUG_USE_ASAN)
	CHECK_AND_SET_FLAGS(COMPILER_SUPPORTS_ASAN "-fsanitize=address" DEBUG_FLAGS "-fsanitize=address")
endif()

if(RELEASE_USE_LTO)
	CHECK_AND_SET_FLAGS(COMPILER_SUPPORTS_LTO "-flto" RELEASE_FLAGS "-flto")
endif()

set(CMAKE_C_FLAGS_DEBUG "${DEBUG_FLAGS} -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "${RELEASE_FLAGS} -O3")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${RELEASE_FLAGS} -O2")
set(CMAKE_C_FLAGS_MINSIZEREL "${RELEASE_FLAGS} -Os")

if(RELWITHDEBINFO_USE_DEBUG_FLAGS)
	set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} ${DEBUG_FLAGS}")
endif()

check_symbol_exists(backtrace "execinfo.h" HAVE_BACKTRACE)
if(${HAVE_BACKTRACE})
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DLOG_ENABLE_BACKTRACE")
	set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -DLOG_ENABLE_BACKTRACE")
endif()

set(WANT_WARNINGS
	"all"
	"pedantic"
	"parentheses"
	"type-limits"
	"strict-prototypes"
	"long-long"
	"init-self"
	"null-dereference"
	"format-pedantic"
	"gcc-compat"
	"float-overflow-conversion"
	"float-zero-conversion"
	"for-loop-analysis"
	"implicit-fallthrough"
	"sometimes-uninitialized"
	"unneeded-internal-declaration"
	"unreachable-code"
	"unreachable-code-loop-increment"
	"gnu"
)

foreach(warning IN LISTS WANT_WARNINGS)
	CHECK_AND_SET_FLAGS(COMPILER_HAS_W${warning} "-W${warning}" WARNINGS)
endforeach()

if(RELATIVE)
	add_definitions(-DRELATIVE)
endif()

if(WERROR)
	set(WARNINGS "${WARNINGS} -Werror")
endif()

if(FATALERRS)
	set(WARNINGS "${WARNINGS} -Wfatal-errors")
endif()

add_definitions(-DPREFIX="${CMAKE_INSTALL_PREFIX}" -std=c11 ${WARNINGS})

if(WIN32 OR APPLE)
	add_definitions(-DLOG_FATAL_MSGBOX)
endif()

set(LIBs ${LIBs}
	${SDL2_LIBRARIES}
	${SDL2_TTF_LIBRARIES}
	${ZLIB_LIBRARIES}
	${PNG_LIBRARIES}
	m)

set(LIBDIRs
	${SDL2_LIBRARY_DIRS}
	${SDL2_TTF_LIBRARY_DIRS}
	${ZLIB_LIBRARY_DIRS}
	${PNG_LIBRARY_DIRS}
	)

if(${WIN32})
	add_definitions(-D__USE_MINGW_ANSI_STDIO)
	set(LIBs ${LIBs} -ldxguid -lwinmm)

	string(REPLACE "gcc" "windres" CMAKE_RC_COMPILER_INIT ${CMAKE_C_COMPILER})
	enable_language(RC)
	set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> -O coff -o <OBJECT> <SOURCE>")

	configure_file(
		"${CMAKE_CURRENT_SOURCE_DIR}/taisei.rc.in"
		"${CMAKE_CURRENT_BINARY_DIR}/taisei.rc"
	)

	set(SRCs ${SRCs} "${CMAKE_CURRENT_BINARY_DIR}/taisei.rc")
endif()

set(INCs
	${CMAKE_CURRENT_SOURCE_DIR}
	${SDL2_INCLUDE_DIRS}
	${SDL2_TTF_INCLUDE_DIRS}
	${ZLIB_INCLUDE_DIRS}
	${PNG_INCLUDE_DIRS}
	)

if(LINK_TO_LIBGL)
	set(INCs ${INCs} ${OPENGL_INCLUDE_DIR})
	set(LIBs ${LIBs} ${OPENGL_gl_LIBRARY})
endif()

if(USE_ZIP AND ZIP_SUPPORTED)
	set(INCs ${INCs} ${ZIP_INCLUDE_DIRS})
	set(LIBs ${LIBs} ${ZIP_LIBRARIES})
	set(LIBDIRs ${LIBDIRs} ${ZIP_LIBRARY_DIRS})
endif()

if(USE_AUDIO)
	set(INCs ${INCs} ${SDL2_MIXER_INCLUDE_DIRS})
	set(LIBs ${LIBs} ${SDL2_MIXER_LIBRARIES})
	set(LIBDIRs ${LIBDIRs} ${SDL2_MIXER_LIBRARY_DIRS})
endif()

set(CMAKE_REQUIRED_INCLUDES ${INCs})
include_directories(${INCs})

if(POSIX)
	add_definitions(-D__POSIX__)
endif()

if (CMAKE_GENERATOR STREQUAL "Ninja" AND
    ((CMAKE_C_COMPILER_ID STREQUAL "GNU" AND NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 4.9) OR
     (CMAKE_C_COMPILER_ID STREQUAL "Clang" AND NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 3.5)))
    # Force colored warnings in Ninja's output, if the compiler has -fdiagnostics-color support.
    # Rationale in https://github.com/ninja-build/ninja/issues/814
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fdiagnostics-color=always")
endif()

list(SORT SRCs)
list(REMOVE_DUPLICATES SRCs)

link_directories(${LIBDIRs})

if(TAISEI_WIN32_CONSOLE)
	list(REMOVE_ITEM LIBs -mwindows)
	add_executable(taisei MACOSX_BUNDLE ${SRCs})
else()
	add_executable(taisei MACOSX_BUNDLE WIN32 ${SRCs})
endif()

target_link_libraries(taisei ${LIBs})

set(MACOSX_BUNDLE_BUNDLE_NAME "Taisei")

if(WIN32)
	set(TAISEI_BINARY_SUFFIX ".exe")
else()
	set(TAISEI_BINARY_SUFFIX "")
endif()

if(APPLE)
	set_target_properties(taisei PROPERTIES
		OUTPUT_NAME "${MACOSX_BUNDLE_BUNDLE_NAME}"
		MACOSX_BUNDLE_ICON_FILE "Taisei"
		MACOSX_BUNDLE_BUNDLE_NAME "Taisei"
		MACOSX_BUNDLE_GUI_IDENTIFIER "org.taisei-project.taisei"
		MACOSX_BUNDLE_COPYRIGHT "© 2011-2017, Taisei Project"
		MACOSX_BUNDLE_BUNDLE_VERSION "${TAISEI_VERSION_SIMPLE}.${TAISEI_VERSION_TWEAK}"
		MACOSX_BUNDLE_SHORT_VERSION_STRING "${TAISEI_VERSION_SIMPLE}"
	)

	install(TARGETS taisei BUNDLE DESTINATION .)

	set(OSX_BINARY_DIR "./${MACOSX_BUNDLE_BUNDLE_NAME}.app/Contents/MacOS")
	set(OSX_BINARY "${OSX_BINARY_DIR}/${MACOSX_BUNDLE_BUNDLE_NAME}")
	set(DYLIBS_DIR_NAME "dylibs")
	set(DYLIBS_DIR_FINAL "${OSX_BINARY_DIR}/${DYLIBS_DIR_NAME}")
	set(DYLIBS_DIR_INTERMEDIATE "${CMAKE_CURRENT_BINARY_DIR}/${DYLIBS_DIR_NAME}")
	set(TAISEI_BINARY "${OSX_BINARY}")

	configure_file(
		"${BUILDSCRIPTS_DIR}/OSXInstallDylibs.cmake.in"
		"${CMAKE_CURRENT_BINARY_DIR}/OSXInstallDylibs.cmake"
	@ONLY)

	install(SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/OSXInstallDylibs.cmake")
	install(DIRECTORY "${DYLIBS_DIR_INTERMEDIATE}" DESTINATION "${OSX_BINARY_DIR}")

	add_custom_target(dmg
		COMMAND "${BUILDSCRIPTS_DIR}/osx-gen-dmg.sh"
				"${CMAKE_INSTALL_PREFIX}"
				"${PROJECT_BINARY_DIR}/${MACOSX_BUNDLE_BUNDLE_NAME}-${TAISEI_VERSION}-macOS.dmg"
		DEPENDS install
		VERBATIM
	)

elseif(RELATIVE)
	install(TARGETS taisei RUNTIME DESTINATION .)
	set(TAISEI_BINARY "./taisei")
else()
	install(TARGETS taisei RUNTIME DESTINATION bin)
	set(TAISEI_BINARY "bin/taisei")
endif()

set(TAISEI_BINARY "${TAISEI_BINARY}${TAISEI_BINARY_SUFFIX}")

if("${CMAKE_BUILD_TYPE}" STREQUAL "Release" AND NOT APPLE)
	configure_file(
		"${BUILDSCRIPTS_DIR}/PostInstallRelease.cmake.in"
		"${CMAKE_CURRENT_BINARY_DIR}/PostInstallRelease.cmake"
	@ONLY)

	install(
		SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/PostInstallRelease.cmake"
	)
endif()
