project(taisei)

cmake_minimum_required(VERSION 2.6)

if(WIN32)
	add_definitions(-DRELATIVE)
	set(RELATIVE TRUE)
endif()
	
add_subdirectory(src)

if(RELATIVE)
	set(DATA_DIR "data")
else()
	set(DATA_DIR "share/taisei")
endif()

install(DIRECTORY gfx DESTINATION ${DATA_DIR}
	FILES_MATCHING PATTERN "*.png")
install(DIRECTORY gfx DESTINATION ${DATA_DIR}
	FILES_MATCHING PATTERN "*.ttf")
install(DIRECTORY sfx DESTINATION ${DATA_DIR})
install(DIRECTORY shader DESTINATION ${DATA_DIR})

# uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)