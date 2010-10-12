set(SRCs
	main.c
	stage.c
	global.c
	player.c
	texture.c
	projectile.c
	animation.c
	fairy.c
	stages/stage0.c)

find_package(SDL REQUIRED)
find_package(OpenGL REQUIRED)
find_package(SDL_image REQUIRED)

add_definitions(-DPREFIX="${CMAKE_INSTALL_PREFIX}")

include_directories(${SDL_INCLUDE_DIRS})
add_executable(openth ${SRCs})
target_link_libraries(openth ${SDL_LIBRARY} ${OPENGL_LIBRARY} ${SDLIMAGE_LIBRARY})

install(TARGETS openth RUNTIME DESTINATION bin)
