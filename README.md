# librtsp  
RTSP协议库

# 编译全志H3
```bash
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=cmake/build_for_h3.cmake ..
```
# 编译全志V831
```bash
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=cmake/build_for_v831.cmake ..
```

# 编译全志f1c100s
```bash
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=cmake/build_for_f1c100s.cmake ..
```
# 编译本机
```bash
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=cmake/build_for_host.cmake ..
```
# 编译MacOs
```zsh
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=cmake/build_for_darwin.cmake ..
```
# 获取摄像头信息
```bash
v4l2-ctl --list-devices
v4l2-ctl --all
v4l2-ctl -d /dev/video0 --all
v4l2-ctl -d /dev/video0 --list-formats-ext
```

# 编译x264
```bash
git clone https://code.videolan.org/videolan/x264.git
./configure --host=arm-linux --disable-asm --prefix=$PWD/install
CC = /Volumes/unix/openwrt/staging_dir/toolchain-arm_cortex-a7+neon-vfpv4_gcc-11.3.0_musl_eabi/bin/arm-openwrt-linux-muslgnueabi-gcc
LD = /Volumes/unix/openwrt/staging_dir/toolchain-arm_cortex-a7+neon-vfpv4_gcc-11.3.0_musl_eabi/bin/arm-openwrt-linux-muslgnueabi-gcc
AR = /Volumes/unix/openwrt/staging_dir/toolchain-arm_cortex-a7+neon-vfpv4_gcc-11.3.0_musl_eabi/bin/arm-openwrt-linux-muslgnueabi-ar
RAMLIB = /Volumes/unix/openwrt/staging_dir/toolchain-arm_cortex-a7+neon-vfpv4_gcc-11.3.0_musl_eabi/bin/arm-openwrt-linux-muslgnueabi-ranlib

cp libx264.a ../librtsp/v4l2demo/x264/
cp x264.h ../librtsp/v4l2demo/x264/
cp x264_config.h ../librtsp/v4l2demo/x264/
```

## ubuntu编译
```bash
sudo apt install -y libx264-dev libfaac-dev
```
# Donation

码农不易 尊重劳动

作者：大魔王leo

功能：rtsp视频服务, 基于cijliu的librtsp重写的C++版本[原仓库地址](https://github.com/cijliu/librtsp.git), 支持v4l2 YUV 4:2:2 格式摄像头，以及可以输出H264的UVC摄像头

QQ：846863428

TEL: 15220187476

email: 846863428@qq.com

修改时间 ：2022-11-23

If you find my work useful and you want to encourage the development of more free resources, you can do it by donating…

觉得不错给鼓励一下

**拿人家代码不star不fork的都是耍流氓**

![alipay](docs/alipay.jpg)
![wechat](docs/wechat.png)
