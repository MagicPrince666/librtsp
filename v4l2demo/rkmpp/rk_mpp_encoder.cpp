#include "rk_mpp_encoder.h"
#include "calculate_rockchip.h"
#include <iostream>

RkMppEncoder::RkMppEncoder(std::string dev, uint32_t width, uint32_t height, uint32_t fps)
    : VideoStream(dev, width, height, fps)
{
    Init();
}

RkMppEncoder::~RkMppEncoder()
{
}

void RkMppEncoder::Init()
{
    std::cout << "RkMppEncoder::Init()" << std::endl;
    v4l2_ctx_ = std::make_shared<V4l2VideoCapture>(dev_name_, video_width_, video_height_, video_fps_);
    // v4l2_ctx_->AddCallback(std::bind(&RkMppEncoder::ProcessImage, this, std::placeholders::_1, std::placeholders::_2));
    camera_buf_ = new (std::nothrow) uint8_t[video_width_ * video_height_ * 2];

    v4l2_ctx_->Init(V4L2_PIX_FMT_MJPEG); // V4L2_PIX_FMT_MJPEG

    calculate_ptr_ = std::make_shared<CalculateRockchip>(video_width_, video_height_);
    calculate_ptr_->Init();
}

int32_t RkMppEncoder::getData(void *fTo, unsigned fMaxSize, unsigned &fFrameSize, unsigned &fNumTruncatedBytes)
{
    uint32_t len = v4l2_ctx_->BuffOneFrame(camera_buf_);

    if (h264_lenght_ < fMaxSize) {
        memcpy(fTo, camera_buf_, h264_lenght_);
        fFrameSize         = h264_lenght_;
        fNumTruncatedBytes = 0;
    } else {
        memcpy(fTo, camera_buf_, fMaxSize);
        fNumTruncatedBytes = h264_lenght_ - fMaxSize;
        fFrameSize         = fMaxSize;
    }
    h264_lenght_ = 0;
    return fFrameSize;
}
