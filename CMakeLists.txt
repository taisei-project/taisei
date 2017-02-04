project(taisei)

cmake_minimum_required(VERSION 2.6)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${taisei_SOURCE_DIR}/cmake" "${taisei_SOURCE_DIR}/cmake/sdl2")

if(NOT DEFINED USE_SDL2_PATHS)
    set(USE_SDL2_PATHS BOOL TRUE)
endif()

if(WIN32)
	set(RELATIVE TRUE)
endif()

if(CMAKE_COMPILER_IS_GNUCC)  
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
endif()
	
add_subdirectory(src)

if(RELATIVE)
    add_definitions(-DRELATIVE)
	set(DATA_DIR "data")

    install(FILES "story.txt" DESTINATION .)
else()
	set(DATA_DIR "share/taisei")

    install(FILES "taisei.desktop" DESTINATION "share/applications")
    install(FILES "taisei.png" DESTINATION "share/icons/hicolor/128x128/apps")
    install(FILES "story.txt" DESTINATION ${DATA_DIR})
endif()

install(DIRECTORY gfx DESTINATION ${DATA_DIR}
	FILES_MATCHING PATTERN "*.png")
install(DIRECTORY gfx DESTINATION ${DATA_DIR}
	FILES_MATCHING PATTERN "*.ttf")
install(DIRECTORY sfx DESTINATION ${DATA_DIR})
install(DIRECTORY bgm DESTINATION ${DATA_DIR})
install(DIRECTORY shader DESTINATION ${DATA_DIR})
install(DIRECTORY models DESTINATION ${DATA_DIR}
	FILES_MATCHING PATTERN "*.obj")

# uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
