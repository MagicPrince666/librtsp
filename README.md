# librtsp  
RTSP协议库



* 编译  

> make  

* 运行  
> cd example && ./demo  
>
> **rtsp服务默认端口设置为8554，客户端需要指定端口，比如服务器IP:192.168.1.2, 那么RTSP地址为:rtsp://192.168.1.2:8554/live，测试程序会循环推送H264文件内容**  

# 编译全志H3
```
$ mkdir build
$ cd build
$ cmake -DCMAKE_TOOLCHAIN_FILE=cmake/build_for_h3.cmake ..
```
# 编译全志V831
```
$ mkdir build
$ cd build
$ cmake -DCMAKE_TOOLCHAIN_FILE=cmake/build_for_v831.cmake ..
```

# 编译全志f1c100s
```
$ mkdir build
$ cd build
$ cmake -DCMAKE_TOOLCHAIN_FILE=cmake/build_for_f1c100s.cmake ..
```
# 编译主机
```
$ mkdir build
$ cd build
$ cmake -DCMAKE_TOOLCHAIN_FILE=cmake/build_for_host.cmake -DCMAKE_BUILD_TYPE=Debug ..
```
# 编译MacOs
```
$ mkdir build
$ cd build
$ cmake -DCMAKE_TOOLCHAIN_FILE=cmake/build_for_darwin.cmake -DCMAKE_BUILD_TYPE=Debug ..
```
# 获取摄像头信息
```
$ v4l2-ctl --list-devices
$ v4l2-ctl --all
$ v4l2-ctl -d /dev/video0 --all
```

# 编译x264
```
./configure --host=arm-linux --disable-asm --prefix=$PWD/install
CC = /Volumes/unix/openwrt/staging_dir/toolchain-arm_cortex-a7+neon-vfpv4_gcc-11.3.0_musl_eabi/bin/arm-openwrt-linux-muslgnueabi-gcc
LD = /Volumes/unix/openwrt/staging_dir/toolchain-arm_cortex-a7+neon-vfpv4_gcc-11.3.0_musl_eabi/bin/arm-openwrt-linux-muslgnueabi-gcc
AR = /Volumes/unix/openwrt/staging_dir/toolchain-arm_cortex-a7+neon-vfpv4_gcc-11.3.0_musl_eabi/bin/arm-openwrt-linux-muslgnueabi-ar
RAMLIB = /Volumes/unix/openwrt/staging_dir/toolchain-arm_cortex-a7+neon-vfpv4_gcc-11.3.0_musl_eabi/bin/arm-openwrt-linux-muslgnueabi-ranlib
```