#include "rk_mpp_encoder.h"
#include <iostream>

RkMppEncoder::RkMppEncoder(std::string dev, uint32_t width, uint32_t height, uint32_t fps)
: VideoStream(dev, width, height, fps)
{
    Init();
}

RkMppEncoder::~RkMppEncoder()
{
    // if (loop_thread_.joinable()) {
    //     loop_thread_.join();
    // }
}

void RkMppEncoder::Init()
{
    v4l2_ctx                = std::make_shared<V4l2VideoCapture>(dev_name_, video_width_, video_height_, video_fps_);
    mpp_ctx                 = std::make_shared<MppContext>();
    v4l2_ctx->AddCallback(std::bind(&RkMppEncoder::ProcessImage, this, std::placeholders::_1, std::placeholders::_2));
    // v4l2_ctx->process_image_ = std::bind(&RkMppEncoder::ProcessImage, this, std::placeholders::_1, std::placeholders::_2);
    // v4l2_ctx->force_format  = 1;
    // v4l2_ctx->width         = video_width_;
    // v4l2_ctx->height        = video_height_;
    // v4l2_ctx->pixelformat   = V4L2_PIX_FMT_YUYV;
    // v4l2_ctx->field         = V4L2_FIELD_INTERLACED;
    mpp_ctx->width          = video_width_;
    mpp_ctx->height         = video_height_;
    mpp_ctx->fps            = video_fps_;
    mpp_ctx->gop            = 30;
    mpp_ctx->bps            = mpp_ctx->width * mpp_ctx->height / 8 * mpp_ctx->fps * 2;
    mpp_ctx->write_frame_    = std::bind(&RkMppEncoder::WriteFrame, this, std::placeholders::_1, std::placeholders::_2);

    h264_buf_ = new (std::nothrow) uint8_t[video_width_ * video_height_ * 2];

    SpsHeader sps_header;
    mpp_ctx->WriteHeader(&sps_header);
    // v4l2_ctx->OpenDevice(dev_name_.c_str());
    // v4l2_ctx->InitDevice();
    // v4l2_ctx->StartCapturing();
    v4l2_ctx->Init();

    // loop_timer_ptr = std::make_shared<TimerFd>();
    // loop_timer_ptr->AddCallback(std::bind(&RkMppEncoder::MainLoop, this));
    // loop_timer_ptr->InitTimer();
    // loop_timer_ptr->StartTimer(0, 33000000);
    
    // loop_thread_ = std::thread([&]() { v4l2_ctx->MainLoop(); });
}

void RkMppEncoder::MainLoop()
{
    uint64_t lenght = v4l2_ctx->BuffOneFrame(h264_buf_);
    ProcessImage(h264_buf_, lenght);
}

bool RkMppEncoder::ProcessImage(const uint8_t *p, const uint32_t size)
{
    return mpp_ctx->ProcessImage(p, size);
}

bool RkMppEncoder::WriteFrame(const uint8_t* data, const uint32_t size)
{
    std::unique_lock<std::mutex> lck(data_mtx_);
    // std::cout << "frame size " << size << std::endl;
    memcpy(h264_buf_, data, size);
    h264_lenght_ = size;
    return true;
}

int32_t RkMppEncoder::getData(void *fTo, unsigned fMaxSize, unsigned &fFrameSize, unsigned &fNumTruncatedBytes)
{
    std::unique_lock<std::mutex> lck(data_mtx_);
    if (h264_lenght_ < fMaxSize) {
        memcpy(fTo, h264_buf_, h264_lenght_);
        fFrameSize         = h264_lenght_;
        fNumTruncatedBytes = 0;
    } else {
        memcpy(fTo, h264_buf_, fMaxSize);
        fNumTruncatedBytes = h264_lenght_ - fMaxSize;
        fFrameSize         = fMaxSize;
    }
    h264_lenght_ = 0;
    return fFrameSize;
}
