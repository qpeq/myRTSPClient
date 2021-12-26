cmake_minimum_required(VERSION 3.16)

project(myRTSPClient	
    VERSION 0.1.0
	DESCRIPTION "Another RTSP Client built with live555"
    LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/include_submodules.cmake)
add_submodules_in_directory(submodules)

add_subdirectory(myRTSPClient)