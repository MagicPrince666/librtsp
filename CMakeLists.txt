CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
option(EXAMPLE "shall we complie example" on)
# INCLUDE(build_for_host)

include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

PROJECT(RtspDemo)

cmake_host_system_information(RESULT var QUERY OS_PLATFORM)
message("processor platfrom: ${var}")

set(CMAKE_CXX_STANDARD 20)

SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb ")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

MESSAGE(STATUS ${CMAKE_SYSTEM_NAME})

IF (CMAKE_HOST_WIN32)
	MESSAGE(STATUS "Now is windows.")
    # add_subdirectory(iocp)
ELSEIF (UNIX)
	MESSAGE(STATUS "Now is UNIX-like OS's.")
    # add_subdirectory(epoll)
ENDIF ()

add_subdirectory(src)
add_subdirectory(v4l2demo/ringbuf)

set(EXEC_NAME rtsp_demo)

add_executable(${EXEC_NAME} example/example.cpp
)

target_link_libraries(${EXEC_NAME}
    pthread
    rtsp
)

IF (CMAKE_HOST_APPLE)
    MESSAGE(STATUS "Not support yet")
ELSEIF (CMAKE_HOST_UNIX)
    set(EXEC_NAME2 uvc_demo)
    # add_subdirectory(v4l2demo/camera)
    add_subdirectory(v4l2demo/h264camera)

    add_executable(${EXEC_NAME2} v4l2demo/example.cpp)

    target_link_libraries(${EXEC_NAME2} 
        pthread
        rtsp
        # camera
        h264camera
        ringbuf
    )
ENDIF()