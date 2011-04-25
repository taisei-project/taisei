set(SRCs
	main.c
	stage.c
	global.c
	player.c
	texture.c
	projectile.c
	animation.c
	fairy.c
	poweritem.c
	slave.c
	list.c
	font.c
	audio.c
	boss.c
	plrmodes.c
	laser.c
	shader.c
	stages/stage0.c)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

find_package(SDL REQUIRED)
find_package(OpenGL REQUIRED)
find_package(OpenAL REQUIRED)
find_package(ALUT REQUIRED)
find_package(SDL_image REQUIRED)
find_package(SDL_ttf REQUIRED)

add_definitions(-DPREFIX="${CMAKE_INSTALL_PREFIX}" -DGL_GLEXT_PROTOTYPES)

include_directories(${SDL_INCLUDE_DIRS} ${ALUT_INCLUDE_DIRS})
add_executable(taisei ${SRCs})
target_link_libraries(taisei ${SDL_LIBRARY} ${OPENGL_LIBRARY} ${SDLIMAGE_LIBRARY} ${SDLTTF_LIBRARY} ${OPENAL_LIBRARY} ${ALUT_LIBRARY})

install(TARGETS taisei RUNTIME DESTINATION bin)
