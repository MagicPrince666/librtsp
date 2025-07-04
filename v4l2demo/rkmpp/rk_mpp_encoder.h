#ifndef __RK_MPP_ENCODER_H__
#define __RK_MPP_ENCODER_H__

#include "video_capture.h"
#include "video_source.h"
#include "timer_fd.h"
#include "mpp.h"
#include <iostream>
#include <memory>
#include <thread>
#include <mutex>
#include <stdint.h>
#include <stdio.h>
#include "calculate_rockchip.h"

#include "x264.h"

class RkMppEncoder : public VideoStream
{
public:
    RkMppEncoder(std::string dev, uint32_t width, uint32_t height, uint32_t fps);
    ~RkMppEncoder();

    int32_t getData(void *fTo, unsigned fMaxSize, unsigned &fFrameSize, unsigned &fNumTruncatedBytes);

private:
    // std::shared_ptr<V4l2Context> v4l2_ctx;
    std::shared_ptr<V4l2VideoCapture> v4l2_ctx;
    std::shared_ptr<MppContext> mpp_ctx;
    std::shared_ptr<TimerFd> loop_timer_ptr;
    std::shared_ptr<Calculate> calculate_ptr_;
    std::mutex data_mtx_;
    uint8_t *camera_buf_;
    uint8_t *h264_buf_;
    int h264_lenght_;
    std::thread loop_thread_;
    typedef struct {
        x264_param_t *param;
        x264_t *handle;
        // 说明一个视频序列中每帧特点
        x264_picture_t *picture;
        x264_nal_t *nal;
    } EncoderData;

    int32_t video_width_;
    int32_t video_height_;
    EncoderData encode_;

    void Init();

    bool ProcessImage(const uint8_t *p, const uint32_t size);
    bool WriteFrame(const uint8_t*data, const uint32_t size);

    void MainLoop();
};

class UvcMppCamera : public VideoFactory
{
public:
    VideoStream *createVideoStream(std::string dev, uint32_t width, uint32_t height, uint32_t fps)
    {
        return new RkMppEncoder(dev, width, height, fps);
    }
};

#endif
