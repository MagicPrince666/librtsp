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

#define SET_WIDTH 640
#define SET_HEIGHT 480

class V4l2VideoCapture
{
public:
    V4l2VideoCapture(std::string dev = "/dev/video0");
    ~V4l2VideoCapture();

    uint8_t* GetUint8tH264Buf();

    int BuffOneFrame(uint8_t* data, int32_t offset, uint64_t maxsize);

    int FrameLength();
    int GetWidth();
    int GetHeight();

    void Init();

private:
    void V4l2Close();

    void OpenCamera();
    void CloseCamera();

    void StartCapturing();
    void StopCapturing();

    void InitCamera();
    void UninitCamera();

    void InitMmap();

    void InitFile();
    void CloseFile();

    void ErrnoExit(const char *s);
    int xioctl(int fd, int request, void *arg);

private:
    struct buffer {
        void *start;
        size_t length;
    };

    struct camera {
        char *device_name;
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

    std::string h264_file_name_ = "test.264";
    std::string v4l2_device_;
    FILE *h264_fp_;

    uint32_t n_buffers_ = 0;
    struct camera *camera_;
};
