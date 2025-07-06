CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
option(EXAMPLE "shall we complie example" on)
# INCLUDE(build_for_host)

PROJECT(RtspDemo LANGUAGES C CXX)

message("-- system processor: ${CMAKE_SYSTEM_PROCESSOR}")

set(CMAKE_CXX_STANDARD 11)

MESSAGE(STATUS ${CMAKE_SYSTEM_NAME})

option(USE_RK_HW_ENCODER "Use Rockchip hardware encoder" OFF)

# find_package(x264 REQUIRED)

include_directories(
    ${CMAKE_SOURCE_DIR}
    include
    v4l2demo/ringbuf
    v4l2demo/camera
    v4l2demo/h264camera
    v4l2demo/epoll
    v4l2demo/calculate
)

add_subdirectory(src)

# set(CMAKE_BUILD_TYPE "Debug")
# if (${CMAKE_BUILD_TYPE} MATCHES "Debug")
#     add_definitions(-DBACKTRACE_DEBUG)
# endif ()

# 执行git命令，并把结果重定向到自定义变量（此处是我的变量<GIT_VERSION>）。
execute_process(
	COMMAND	git log -1 --format=%H 		
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} 
	OUTPUT_VARIABLE GIT_VERSION
)
execute_process(
	COMMAND git remote -v 			
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} 
	OUTPUT_VARIABLE GIT_REMOTE_PATH
)

# 对得到的结果进行处理，尤其注意 \n,\t,\r之类的特殊字符，在cmake时没问题，但是生成的.cmake文件有问题，导致make出错。
string (REGEX REPLACE ".*\t| .*" "" GIT_PATH ${GIT_REMOTE_PATH})
string (REGEX REPLACE "[\n\t\r]" "" GIT_VERSION ${GIT_VERSION})

# 增加编译选项，把宏导入源码
add_definitions( -DGIT_VERSION=\"${GIT_VERSION}\")
add_definitions( -DGIT_PATH=\"${GIT_PATH}\")

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
        pthread
        dl
    )

    find_package(PkgConfig REQUIRED)

    file(GLOB main_srcs 
        v4l2demo/camera/video_capture.cpp
        v4l2demo/h264camera/*.cpp
        v4l2demo/epoll/*.cpp
        v4l2demo/ringbuf/*.cpp
        v4l2demo/*.cpp
        v4l2demo/calculate/calculate_cpu.cpp
    )

    if (USE_RK_HW_ENCODER)
        pkg_search_module(RK_MPP REQUIRED rockchip_mpp)
        if (NOT RK_MPP_FOUND)
            message(FATAL_ERROR "rockchip_mpp is not found use x264")
        endif ()
        pkg_search_module(RGA librga)
        if (NOT RGA_FOUND)
            add_definitions(-DUSE_LIBYUV)
            message("librga is not found, use libyuv instead")
            list(APPEND COMMON_LIBRARIES
                -lyuv
            )
        else()
            list(APPEND main_srcs v4l2demo/calculate/calculate_rockchip.cpp)
        endif ()
        add_definitions(-DUSE_RK_HW_ENCODER)

        list(APPEND main_srcs 
            v4l2demo/rkmpp/rk_mpp_encoder.cpp
        )

        list(APPEND COMMON_LIBRARIES
            ${RK_MPP_LIBRARIES}
            ${RGA_LIBRARIES}
        )

        include_directories(
            v4l2demo/rkmpp
            ${RK_MPP_INCLUDE_DIRS}
            ${RGA_INCLUDE_DIRS}
        )
    else()
        # 查找libx264库
        pkg_check_modules(LIBX264 x264)

        if (LIBX264_FOUND)
            list(APPEND main_srcs
                v4l2demo/camera/h264encoder.cpp
                v4l2demo/camera/h264_camera.cpp
            )
            list(APPEND COMMON_LIBRARIES
                ${LIBX264_LIBRARIES}
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


