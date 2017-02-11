set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(OpenAL REQUIRED)
find_package(ALUT REQUIRED)
find_package(PNG REQUIRED)
find_package(SDL2_ttf REQUIRED)
find_package(Freetype)
find_package(ZLIB REQUIRED)
find_package(OGG)

if(OGG_FOUND)
find_package(Vorbis)
endif()

if(VORBIS_FOUND)
find_package(VorbisFile)
endif()

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
)

if(VORBISFILE_FOUND)
	set(SRCs ${SRCs} resource/ogg.c)
	add_definitions(-DOGG_SUPPORT_ENABLED)
endif()

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

add_definitions(-DPREFIX="${CMAKE_INSTALL_PREFIX}" -Wall -Wno-parentheses)

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
	${SDL2_LIBRARY}
    ${SDL2_TTF_LIBRARIES}
    ${PNG_LIBRARY}
	${OPENAL_LIBRARY}
	${ALUT_LIBRARY}
	${OPENGL_LIBRARY}
	m)


if(FREETYPE_FOUND)
	set(LIBs ${LIBs} ${FREETYPE_LIBRARY})
endif()

if(VORBISFILE_FOUND)
	set(LIBs ${LIBs} ${OGG_LIBRARY} ${VORBIS_LIBRARY} ${VORBISFILE_LIBRARY})
endif()

if(ZLIB_FOUND)
	set(LIBs ${LIBs} ${ZLIB_LIBRARY})
endif()

if(WIN32)
	set(LIBs ${LIBs} -ldxguid -lwinmm)

	string(REPLACE "gcc" "windres" CMAKE_RC_COMPILER_INIT ${CMAKE_C_COMPILER})
	enable_language(RC)
	set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> -O coff -o <OBJECT> <SOURCE>")

	set(SRCs ${SRCs} taisei.rc)
endif()

if(TAISEI_WIN32_CONSOLE)
	list(REMOVE_ITEM LIBs -mwindows)
	add_definitions(-D__WINDOWS_CONSOLE__)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${SDL2_INCLUDE_DIR} ${ALUT_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} ${PNG_INCLUDE_DIR})

unset(POSIX CACHE)
check_symbol_exists(_POSIX_VERSION "unistd.h" POSIX)

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

add_executable(taisei ${SRCs})
target_link_libraries(taisei ${LIBs})

if(RELATIVE)
	install(TARGETS taisei RUNTIME DESTINATION .)
else()
	install(TARGETS taisei RUNTIME DESTINATION bin)
endif()
