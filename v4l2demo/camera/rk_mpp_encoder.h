#ifndef __RK_MPP_ENCODER_H__
#define __RK_MPP_ENCODER_H__

#include "video_capture.h"
#include "video_source.h"
#include "timer_fd.h"
#include "mpp.h"
#include <iostream>
#include <memory>
#include <thread>

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
    std::mutex data_mtx_;
    uint8_t *h264_buf_;
    int h264_lenght_;
    std::thread loop_thread_;

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
