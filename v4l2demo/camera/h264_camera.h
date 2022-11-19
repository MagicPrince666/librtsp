/**
 * @file h264_camera.h
 * @author 黄李全 (846863428@qq.com)
 * @brief 获取视频流并编码
 * @version 0.1
 * @date 2022-11-18
 * @copyright Copyright (c) {2021} 个人版权所有
 */
#pragma once

#include "video_capture.h"
#include <mutex>
#include <thread>

#define CLEAR(x) memset(&(x), 0, sizeof(x))

#define DelayTime 33 * 1000 //(33us*1000=0.05s 30f/s)

class V4l2H264hData
{
public:
    V4l2H264hData();
    virtual ~V4l2H264hData();

    void Init();

    void VideoCaptureThread();
    void VideoEncodeThread();

    void startCap();
    void stopCap();

    void EmptyBuffer();
    int getData(void *fTo, unsigned fMaxSize, unsigned &fFrameSize, unsigned &fNumTruncatedBytes);

private:
    void setSource(void *_p) {
        s_source_ = _p;
    }

    struct cam_data {
        unsigned char cam_mbuf[BUF_SIZE]; /*缓存区数组5242880=5MB//缓存区数组10485760=10MB//缓存区数组1536000=1.46484375MB,10f*/
        int wpos;
        int rpos;                 /*写与读的位置*/
        // std::cond captureOK; /*线程采集满一个缓冲区时的标志*/
        // std::cond encodeOK;  /*线程编码完一个缓冲区的标志*/
        std::mutex lock;     /*互斥锁*/
    };

    V4l2VideoCapture *p_capture_;
    void *s_source_;
    bool s_b_running_;
    bool s_quit_;
    bool empty_buffer_;
    struct cam_data cam_data_buff_[2];
    bool buff_full_flag_[2];

    std::thread video_capture_thread_;
    std::thread video_encode_thread_;
};
