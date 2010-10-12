project(openth)

cmake_minimum_required(VERSION 2.6)

add_subdirectory(src)

set(DATA_DIR "share/openth")
install(DIRECTORY gfx DESTINATION ${DATA_DIR})
