set(SRCs
	main.c
	stage.c
	global.c
	player.c
	texture.c
	projectile.c
	animation.c
	fairy.c
	list.c
	font.c
	stages/stage0.c)

find_package(SDL REQUIRED)
find_package(OpenGL REQUIRED)
find_package(SDL_image REQUIRED)
find_package(SDL_ttf REQUIRED)

add_definitions(-DPREFIX="${CMAKE_INSTALL_PREFIX}")

include_directories(${SDL_INCLUDE_DIRS})
add_executable(taisei ${SRCs})
target_link_libraries(taisei ${SDL_LIBRARY} ${OPENGL_LIBRARY} ${SDLIMAGE_LIBRARY} ${SDLTTF_LIBRARY})

install(TARGETS taisei RUNTIME DESTINATION bin)
