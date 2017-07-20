cmake_minimum_required(VERSION 3.5)
project(obs-virtuacam)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
add_subdirectory(src/virtual-output)
add_subdirectory(src/virtual-source)


	
