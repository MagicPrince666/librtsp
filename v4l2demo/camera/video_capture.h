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

#define BUF_SIZE 614400
/*C270 YUV 4:2:2 frame size(char)
160*120*2  = 38400
176*144*2  = 50688
320*176*2  = 112640
320*240*2  = 153600
352*288*2  = 202752
432*240*2  = 207360
544*288*2  = 313344
640*360*2  = 460800
640*480*2  = 614400
752*416*2  = 625664
800*448*2  = 716800
800*600*2  = 960000
864*480*2  = 829440
960*544*2  = 1044480
960*720*2  = 1382400
1024*576*2 = 1179648
1184*656*2 = 1553408
*/
//#define FIFO_NAME "/tmp/my_video.h264"

#include "h264encoder.h"

#define SET_WIDTH 640
#define SET_HEIGHT 480

class V4l2VideoCapture
{
public:
    V4l2VideoCapture(std::string dev = "/dev/video0");
    ~V4l2VideoCapture();

    uint8_t* GetUint8tH264Buf();

    int BuffOneFrame(uint8_t* data, int32_t offset);

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
