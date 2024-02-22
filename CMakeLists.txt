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

set(CMAKE_CXX_STANDARD 11)

SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb ")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

MESSAGE(STATUS ${CMAKE_SYSTEM_NAME})

option(USE_RK_HW_ENCODER "Use Rockchip hardware encoder" ON)

add_subdirectory(src)

# set(CMAKE_BUILD_TYPE "Debug")
if (${CMAKE_BUILD_TYPE} MATCHES "Debug")
add_definitions(-DBACKTRACE_DEBUG)
endif ()

file(GLOB demo_srcs
    example/example.cpp
    # v4l2demo/epoll/*.cpp
)

add_executable(${PROJECT_NAME} ${demo_srcs})

target_link_libraries(${PROJECT_NAME}
    pthread
    rtsp
)

IF (CMAKE_SYSTEM_NAME MATCHES "Linux")
    MESSAGE(STATUS "Now is ${CMAKE_SYSTEM_NAME}.")

    set(EXEC_NAME2 V4l2Demo)

    set(COMMON_LIBRARIES
        rtsp
        x264
        pthread
        dl
    )

    find_package(PkgConfig REQUIRED)
    if (USE_RK_HW_ENCODER)
        pkg_search_module(RK_MPP REQUIRED rockchip_mpp)
        if (NOT RK_MPP_FOUND)
            message(FATAL_ERROR "rockchip_mpp is not found")
        endif ()
        pkg_search_module(RGA librga)
        if (NOT RGA_FOUND)
            add_definitions(-DUSE_LIBYUV)
            message("librga is not found, use libyuv instead")
        endif ()
    endif ()

    file(GLOB main_srcs 
        v4l2demo/camera/*.cpp
        v4l2demo/h264camera/*.cpp
        v4l2demo/epoll/*.cpp
        v4l2demo/ringbuf/*.cpp
        v4l2demo/*.cpp
    )

    if (USE_RK_HW_ENCODER)
        list(APPEND COMMON_LIBRARIES
            ${RK_MPP_LIBRARIES}
            ${RGA_LIBRARIES}
        )
    endif ()

    if (USE_RK_HW_ENCODER)
        add_definitions(-DUSE_RK_HW_ENCODER)
        list(APPEND main_srcs rk_mpp/rk_mpp_encoder.cpp)
        list(APPEND COMMON_INCLUDE_DIRS
            ${RK_MPP_INCLUDE_DIRS}
            ${RGA_INCLUDE_DIRS}
        )
        list(APPEND COMMON_LIBRARIES
            ${RGA_LIBRARIES}
            ${RK_MPP_LIBRARIES}
        )
        if (NOT RGA_FOUND)
            list(APPEND COMMON_LIBRARIES
            -lyuv
            )
        endif ()
    endif ()

    add_executable(${EXEC_NAME2} ${main_srcs})

    target_link_libraries(${EXEC_NAME2}
        ${COMMON_LIBRARIES}
    )

    install(TARGETS ${PROJECT_NAME} ${EXEC_NAME2}
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

    install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION /usr/bin)
ELSE ()
    MESSAGE(STATUS "Not support ${CMAKE_SYSTEM_NAME} yet")
ENDIF ()


