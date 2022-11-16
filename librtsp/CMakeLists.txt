file(GLOB rtsp_srcs "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_library(rtsp 
    ${rtsp_srcs}
)
target_include_directories(rtsp
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
