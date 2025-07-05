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
    v4l2_ctx_ = std::make_shared<V4l2VideoCapture>(dev_name_, video_width_, video_height_, video_fps_);
    // v4l2_ctx_->AddCallback(std::bind(&RkMppEncoder::ProcessImage, this, std::placeholders::_1, std::placeholders::_2));
    camera_buf_ = new (std::nothrow) uint8_t[video_width_ * video_height_ * 2];

    v4l2_ctx_->Init(V4L2_PIX_FMT_MJPEG); // V4L2_PIX_FMT_MJPEG

    // calculate_ptr_ = std::make_shared<CalculateRockchip>(video_width_, video_height_);
    // calculate_ptr_->Init();
}

bool RkMppEncoder::mppFrame2H264(const MppFrame frame, uint8_t *data)
{
    int width        = mpp_frame_get_width(frame);
    int height       = mpp_frame_get_height(frame);
    MppBuffer buffer = mpp_frame_get_buffer(frame);
    memset(data, 0, width * height * 3);
    auto buffer_ptr = mpp_buffer_get_ptr(buffer);
    rga_info_t src_info;
    rga_info_t dst_info;
    // NOTE: memset to zero is MUST
    memset(&src_info, 0, sizeof(rga_info_t));
    memset(&dst_info, 0, sizeof(rga_info_t));
    src_info.fd      = -1;
    src_info.mmuFlag = 1;
    src_info.virAddr = buffer_ptr;
    src_info.format  = RK_FORMAT_YCbCr_420_SP;
    dst_info.fd      = -1;
    dst_info.mmuFlag = 1;
    dst_info.virAddr = data;
    dst_info.format  = RK_FORMAT_BGR_888;
    rga_set_rect(&src_info.rect, 0, 0, width, height, width, height, RK_FORMAT_YCbCr_420_SP);
    rga_set_rect(&dst_info.rect, 0, 0, width, height, width, height, RK_FORMAT_BGR_888);
    int ret = c_RkRgaBlit(&src_info, &dst_info, nullptr);
    if (ret) {
        std::cerr << "c_RkRgaBlit error " << ret << " errno " << strerror(errno) << std::endl;
        return false;
    }
    return true;
}

int32_t RkMppEncoder::getData(void *fTo, unsigned fMaxSize, unsigned &fFrameSize, unsigned &fNumTruncatedBytes)
{
    std::unique_lock<std::mutex> lck(data_mtx_);
    camera_buf_size_ = v4l2_ctx_->BuffOneFrame(camera_buf_);

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
