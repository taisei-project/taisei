project(taisei)

cmake_minimum_required(VERSION 2.6)

add_subdirectory(src)

set(CMAKE_CFLAGS="--std=c99")

set(DATA_DIR "share/taisei")
install(DIRECTORY gfx DESTINATION ${DATA_DIR})
# uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)