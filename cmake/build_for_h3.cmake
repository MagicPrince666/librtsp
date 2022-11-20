######## cross compile env define ###################
SET(CMAKE_SYSTEM_NAME Linux)
# 配置库的安装路径
SET(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install)

SET(CMAKE_SYSTEM_PROCESSOR arm)

# 包含x264头文件
include_directories(${CMAKE_SOURCE_DIR}/v4l2demo/x264/h3)

# x264库路径
link_directories(${CMAKE_SOURCE_DIR}/v4l2demo/x264/h3)

# 工具链地址
SET(TOOLCHAIN_DIR  "/Volumes/unix/openwrt/staging_dir/toolchain-arm_cortex-a7+neon-vfpv4_gcc-11.3.0_musl_eabi/bin/")
SET(STAGING_DIR  "/Volumes/unix/openwrt/staging_dir")

# sunxi h3
SET(CMAKE_C_COMPILER ${TOOLCHAIN_DIR}arm-openwrt-linux-muslgnueabi-gcc)
SET(CMAKE_CXX_COMPILER ${TOOLCHAIN_DIR}arm-openwrt-linux-muslgnueabi-g++)
