CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
option(EXAMPLE "shall we complie example" on)
# INCLUDE(build_for_host)

# find_package(x264 REQUIRED)

include_directories(
    ${CMAKE_SOURCE_DIR}
    include
    v4l2demo/ringbuf
    v4l2demo/camera
    v4l2demo/h264camera
    v4l2demo/epoll
)

PROJECT(RtspDemo)

message("-- system processor: ${CMAKE_SYSTEM_PROCESSOR}")

set(CMAKE_CXX_STANDARD 17)

SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb ")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

MESSAGE(STATUS ${CMAKE_SYSTEM_NAME})

add_subdirectory(src)

set(EXEC_NAME rtsp_demo)

add_executable(${EXEC_NAME} example/example.cpp
)

target_link_libraries(${EXEC_NAME}
    pthread
    rtsp
)

IF (CMAKE_SYSTEM_NAME MATCHES "Linux")
    MESSAGE(STATUS "Now is ${CMAKE_SYSTEM_NAME}.")

    set(EXEC_NAME2 V4l2Demo)

    file(GLOB main_srcs 
        v4l2demo/camera/*.cpp
        v4l2demo/h264camera/*.cpp
        v4l2demo/epoll/*.cpp
        v4l2demo/ringbuf/*.cpp
        v4l2demo/*.cpp
    )

    add_executable(${EXEC_NAME2} ${main_srcs})

    target_link_libraries(${EXEC_NAME2}
        rtsp
        x264
        pthread
        dl
    )

    install(TARGETS ${EXEC_NAME} ${EXEC_NAME2}
        RUNTIME DESTINATION /usr/bin)
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Windows")
    MESSAGE(STATUS "Now is windows.")
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "darwin")
    find_package(OpenCV REQUIRED)
    IF (OPENCV_FOUND)
        set(OPENCV_DEMO_NAME CameraShow)
        include_directories(${OpenCV_INCLUDE_DIRS})
        add_executable(${OPENCV_DEMO_NAME} opencv/camera_cap.cpp)
        target_link_libraries(${OPENCV_DEMO_NAME} ${OpenCV_LIBS})
    ENDIF()

    install(TARGETS ${EXEC_NAME}
        RUNTIME DESTINATION /usr/bin)
ELSE ()
    MESSAGE(STATUS "Not support ${CMAKE_SYSTEM_NAME} yet")
ENDIF ()


