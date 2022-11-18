file(GLOB camera_srcs "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_library(camera 
    ${camera_srcs}
)

target_include_directories(camera
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
