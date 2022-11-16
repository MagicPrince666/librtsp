CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
option(EXAMPLE "shall we complie example" on)
# INCLUDE(build_for_host)

include_directories(${CMAKE_SOURCE_DIR})

PROJECT(RtspDemo)

set(CMAKE_CXX_STANDARD 20)

MESSAGE(STATUS ${CMAKE_SYSTEM_NAME})

IF (CMAKE_HOST_WIN32)
	MESSAGE(STATUS "Now is windows.")
    # add_subdirectory(iocp)
ELSEIF (UNIX)
	MESSAGE(STATUS "Now is UNIX-like OS's.")
    # add_subdirectory(epoll)
ENDIF ()

add_subdirectory(src)

set(EXEC_NAME rtsp_demo)

add_executable(${EXEC_NAME} example/example.cpp
)

target_link_libraries(${EXEC_NAME} 
    rtsp
)

