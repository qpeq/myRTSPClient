cmake_minimum_required(VERSION 3.16)

add_executable(${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS OFF
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>    
        $<INSTALL_INTERFACE:inc>
)

target_sources(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc/my_media_sink.h
        ${CMAKE_CURRENT_SOURCE_DIR}/inc/my_rtsp_client.h
        ${CMAKE_CURRENT_SOURCE_DIR}/inc/stream_client_state.h
        ${CMAKE_CURRENT_SOURCE_DIR}/inc/get_nal_unit_type.h
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/my_media_sink.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/my_rtsp_client.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/get_nal_unit_type.cpp
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        liveMedia
        BasicUsageEnvironment
        UsageEnvironment
        groupsock
        $<$<NOT:$<BOOL:${WIN32}>>:pthread>
)

