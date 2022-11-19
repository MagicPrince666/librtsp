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

set(CMAKE_CXX_STANDARD 17)

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
ELSEIF ()
    set(EXEC_NAME2 v4l2_demo)
    add_subdirectory(v4l2demo/camera)
    add_subdirectory(v4l2demo/h264camera)
    add_subdirectory(v4l2demo/epoll)

    add_executable(${EXEC_NAME2} v4l2demo/v4l2example.cpp)

    target_link_libraries(${EXEC_NAME2} 
        pthread
        rtsp
        camera
        h264camera
        ringbuf
        x264
        dl
    )
ENDIF()

find_package(OpenCV REQUIRED)
IF (OPENCV_FOUND)
    set(OPENCV_DEMO_NAME CameraShow)
    include_directories(${OpenCV_INCLUDE_DIRS})
    add_executable(${OPENCV_DEMO_NAME} opencv/camera_cap.cpp)
    target_link_libraries(${OPENCV_DEMO_NAME} ${OpenCV_LIBS})
ENDIF()