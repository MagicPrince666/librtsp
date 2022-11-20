file(GLOB camera_srcs "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_library(camera 
    ${camera_srcs}
)

include_directories(
    ${CMAKE_SOURCE_DIR}/v4l2demo/ringbuf
    ${CMAKE_SOURCE_DIR}/v4l2demo/epoll
    ${CMAKE_SOURCE_DIR}/v4l2demo/x264
)

target_include_directories(camera
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
