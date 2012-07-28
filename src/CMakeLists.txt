set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

find_package(SDL REQUIRED)
find_package(OpenGL REQUIRED)
find_package(OpenAL REQUIRED)
find_package(ALUT REQUIRED)
find_package(PNG REQUIRED)
find_package(SDL_ttf REQUIRED)
find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)
find_package(Freetype)
find_package(ZLIB)

BISON_TARGET(cfgparser config.y ${CMAKE_CURRENT_SOURCE_DIR}/parser.c)
FLEX_TARGET(cfgscanner config.l  ${CMAKE_CURRENT_SOURCE_DIR}/lexer.c)
ADD_FLEX_BISON_DEPENDENCY(cfgscanner cfgparser)

set(SRCs
	main.c
	stage.c
	replay.c
	global.c
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
	stages/stage0.c
	stages/stage1.c
	stages/stage1_events.c
	stages/stage2.c
	stages/stage2_events.c
	stages/stage3.c
	stages/stage3_events.c
	stages/stage4.c
	stages/stage4_events.c
	stages/stage6.c
	stages/stage6_events.c
	resource/resource.c
	resource/texture.c
	resource/animation.c
	resource/font.c
	resource/shader.c
	resource/audio.c
	resource/model.c
	${BISON_cfgparser_OUTPUTS}
	${FLEX_cfgscanner_OUTPUTS})

if(RELATIVE)
	set(SRCs ${SRCs} paths/relative.c)	
else()
	set(SRCs ${SRCs} paths/static.c)
endif()

if(WIN32)
	set(SRCs ${SRCs} taisei_err.c)
endif()

add_definitions(-DGL_GLEXT_PROTOTYPES -DPREFIX="${CMAKE_INSTALL_PREFIX}" -Wall)

if(TAISEI_DEBUG)
	add_definitions(-DDEBUG)
endif()

set(LIBs ${LIBs}
	${SDL_LIBRARY}
	${PNG_LIBRARY}
	${SDLTTF_LIBRARY}
	${OPENAL_LIBRARY}
	${ALUT_LIBRARY}
	${OPENGL_LIBRARY}
	m)
	

if(FREETYPE_FOUND)
	set(LIBs ${LIBs} ${FREETYPE_LIBRARY})
endif()

if(ZLIB_FOUND)
	set(LIBs ${LIBs} ${ZLIB_LIBRARY})
endif()

if(WIN32)
	set(LIBs ${LIBs} -ldxguid -limm32 -lversion)
endif()
	
include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${SDL_INCLUDE_DIR} ${ALUT_INCLUDE_DIR})
add_executable(taisei WIN32 ${SRCs})
target_link_libraries(taisei ${LIBs})

if(RELATIVE)
	install(TARGETS taisei RUNTIME DESTINATION .)
else()
	install(TARGETS taisei RUNTIME DESTINATION bin)
endif()
