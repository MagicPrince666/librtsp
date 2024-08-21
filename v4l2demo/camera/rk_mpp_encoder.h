#ifndef __RK_MPP_ENCODER_H__
#define __RK_MPP_ENCODER_H__

#include "video_capture.h"
#include "video_source.h"
#include "v4l2.h"
#include "mpp.h"
#include <iostream>
#include <memory>

class RkMppEncoder : public VideoStream
{
public:
    RkMppEncoder(std::string dev, uint32_t width, uint32_t height, uint32_t fps);
    ~RkMppEncoder();

    int32_t getData(void *fTo, unsigned fMaxSize, unsigned &fFrameSize, unsigned &fNumTruncatedBytes);

private:
    std::shared_ptr<V4l2Context> v4l2_ctx;
    std::shared_ptr<MppContext> mpp_ctx;
    std::mutex data_mtx_;
    uint8_t *h264_buf_;
    int h264_lenght_;

    void Init();

    bool ProcessImage(uint8_t *p, int size);
    bool WriteFrame(uint8_t*data,int size);
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
