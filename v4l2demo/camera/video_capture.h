/**
 * @file video_capture.h
 * @author 黄李全 (846863428@qq.com)
 * @brief 获取v4l2视频
 * @version 0.1
 * @date 2022-11-18
 * @copyright Copyright (c) {2021} 个人版权所有
 */
#pragma once

#include <linux/videodev2.h>
#include <pthread.h>
#include <stdint.h>
#include <iostream>

#include "h264encoder.h"

class V4l2VideoCapture
{
public:
    V4l2VideoCapture(std::string dev = "/dev/video0", int width = 640, int height = 480, int fps = 30);
    ~V4l2VideoCapture();

    /**
     * @brief 初始化
     */
    bool Init();

    /**
     * @brief 获取一帧数据
     * @param data 拷贝到此地址
     * @param offset 地址偏移
     * @param maxsize 最大长度
     * @return uint64_t 
     */
    uint64_t BuffOneFrame(uint8_t* data);

    /**
     * 获取帧长度
    */
    int GetFrameLength();

    /**
     * @brief 获取视频宽度
     * @return int 
     */
    int GetWidth();

    /**
     * @brief 获取视频高度
     * @return int 
     */
    int GetHeight();

    /**
     * 获取句柄
    */
    int GetHandle();

private:
    /**
     * @brief 关闭v4l2资源
     */
    bool V4l2Close();

    /**
     * @brief 打开摄像头
     */
    bool OpenCamera();

    /**
     * @brief 关闭摄像头
     */
    bool CloseCamera();

    /**
     * @brief 开始录制
     */
    bool StartCapturing();

    /**
     * @brief 停止录制
     */
    bool StopCapturing();

    /**
     * @brief 初始化摄像头参数
     */
    bool InitCamera();

    /**
     * @brief 退出设置
     */
    bool UninitCamera();

    /**
     * @brief 初始化mmap
     */
    bool InitMmap();

    void ErrnoExit(const char *s);
    int xioctl(int fd, int request, void *arg);

private:
    struct buffer {
        void *start;
        size_t length;
    };

    struct camera {
        int fd;
        int width;
        int height;
        int fps;
        int display_depth;
        int image_size;
        int frame_number;
        struct v4l2_capability v4l2_cap;
        struct v4l2_cropcap v4l2_cropcap;
        struct v4l2_format v4l2_fmt;
        struct v4l2_crop crop;
        struct buffer *buffers;
    };

    std::string v4l2_device_;
    int v4l2_width_;
    int v4l2_height_;
    int v4l2_fps_;
    uint32_t n_buffers_ = 0;
    struct camera camera_;
};
