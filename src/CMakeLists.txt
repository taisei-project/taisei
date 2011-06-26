set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

find_package(SDL REQUIRED)
find_package(OpenGL REQUIRED)
find_package(OpenAL REQUIRED)
find_package(ALUT REQUIRED)
find_package(PNG REQUIRED)
find_package(SDL_ttf REQUIRED)
find_package(GLEW REQUIRED)
find_package(BISON)
find_package(FLEX)

BISON_TARGET(cfgparser config.y ${CMAKE_CURRENT_SOURCE_DIR}/parser.c)
FLEX_TARGET(cfgscanner config.l  ${CMAKE_CURRENT_SOURCE_DIR}/lexer.c)
ADD_FLEX_BISON_DEPENDENCY(cfgscanner cfgparser)

set(SRCs
	main.c
	stage.c
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
	menu/menu.c
	menu/mainmenu.c
	menu/ingamemenu.c
	menu/difficulty.c
	menu/charselect.c
	stages/stage0.c
	resource/texture.c
	resource/animation.c
	resource/font.c
	resource/shader.c
	resource/audio.c
	taisei_err.c
	${BISON_cfgparser_OUTPUTS}
	${FLEX_cfgscanner_OUTPUTS})

add_definitions(-DPREFIX="${CMAKE_INSTALL_PREFIX}")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

include_directories(${SDL_INCLUDE_DIRS} ${ALUT_INCLUDE_DIRS})
add_executable(taisei ${SRCs})
target_link_libraries(taisei ${SDL_LIBRARY} ${OPENGL_LIBRARY} ${PNG_LIBRARY}
				${SDLTTF_LIBRARY} ${OPENAL_LIBRARY} ${ALUT_LIBRARY} ${GLEW_LIBRARY})

install(TARGETS taisei RUNTIME DESTINATION bin)
