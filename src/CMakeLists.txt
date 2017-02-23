
find_package(PkgConfig REQUIRED)
find_package(OpenGL REQUIRED)

pkg_check_modules(SDL2 sdl2 REQUIRED)
pkg_check_modules(SDL2_TTF SDL2_ttf REQUIRED)
pkg_check_modules(SDL2_MIXER SDL2_mixer REQUIRED)
pkg_check_modules(ZLIB zlib REQUIRED)
pkg_check_modules(PNG libpng REQUIRED)

set(SRCs
	main.c
	taiseigl.c
	random.c
	config.c
	gamepad.c
	stage.c
	replay.c
	global.c
	events.c
	player.c
	projectile.c
	progress.c
	enemy.c
	item.c
	list.c
	boss.c
	plrmodes.c
	laser.c
	dialog.c
	fbo.c
	vbo.c
	stageutils.c
	matrix.c
	video.c
	transition.c
	menu/menu.c
	menu/mainmenu.c
	menu/options.c
	menu/stageselect.c
	menu/replayview.c
	menu/ingamemenu.c
	menu/gameovermenu.c
	menu/savereplay.c
	menu/difficulty.c
	menu/charselect.c
	menu/spellpractice.c
	menu/common.c
	stages/stage1.c
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
	resource/audio.c
	resource/bgm.c
	resource/model.c
	rwops/rwops_zlib.c
	rwops/rwops_segment.c
	rwops/rwops_autobuf.c
)

if(USE_SDL2_PATHS)
    set(SRCs ${SRCs} paths/sdl.c)
elseif(RELATIVE)
	set(SRCs ${SRCs} paths/relative.c)
else()
	set(SRCs ${SRCs} paths/static.c)
endif()

if(WIN32)
	set(SRCs ${SRCs} taisei_err.c)
endif()

add_definitions(-DPREFIX="${CMAKE_INSTALL_PREFIX}" -Wall -Wno-parentheses -Wtype-limits -std=c11 -pedantic)

if(RELATIVE)
    add_definitions(-DRELATIVE)
endif()

if(TAISEI_DEBUG)
	add_definitions(-DDEBUG -ggdb)
endif()

if(WERROR)
	add_definitions(-Werror)
endif()

if(FATALERRS)
	add_definitions(-Wfatal-errors)
endif()

set(LIBs ${LIBs}
	${SDL2_LIBRARIES}
	${SDL2_TTF_LIBRARIES}
	${SDL2_MIXER_LIBRARIES}
	${PNG_LIBRARIES}
	${OPENGL_LIBRARIES}
	${ZLIB_LIBRARIES}
	m)

if(WIN32)
	set(LIBs ${LIBs} -ldxguid -lwinmm)

	string(REPLACE "gcc" "windres" CMAKE_RC_COMPILER_INIT ${CMAKE_C_COMPILER})
	enable_language(RC)
	set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> -O coff -o <OBJECT> <SOURCE>")

	set(SRCs ${SRCs} taisei.rc)
endif()

set(INCs
	${CMAKE_CURRENT_SOURCE_DIR}
	${SDL2_INCLUDE_DIRS}
	${SDL2_MIXER_INCLUDE_DIRS}
	${SDL2_TTF_INCLUDE_DIRS}
	${ZLIB_INCLUDE_DIRS}
	${OPENGL_INCLUDE_DIR}
	${PNG_INCLUDE_DIRS}
	)

set(CMAKE_REQUIRED_INCLUDES ${INCs})
include_directories(${INCs})

check_symbol_exists(_POSIX_VERSION "unistd.h" POSIX)
check_symbol_exists(glDrawArraysInstanced "taiseigl.h" HAVE_glDrawArraysInstanced)

if(POSIX)
	add_definitions(-D__POSIX__)
endif()

if(NOT HAVE_glDrawArraysInstanced)
	add_definitions(-DGL_USE_ARB_DRAW_INSTANCED)
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
list(REMOVE_DUPLICATES LIBs)

if(TAISEI_WIN32_CONSOLE)
	list(REMOVE_ITEM LIBs -mwindows)
	add_executable(taisei ${SRCs})
else()
	add_executable(taisei WIN32 ${SRCs})
endif()

target_link_libraries(taisei ${LIBs})

if(RELATIVE)
	install(TARGETS taisei RUNTIME DESTINATION .)
else()
	install(TARGETS taisei RUNTIME DESTINATION bin)
endif()
