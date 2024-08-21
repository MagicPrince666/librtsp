#include "rk_mpp_encoder.h"
#include <iostream>

RkMppEncoder::RkMppEncoder()
{
}

RkMppEncoder::~RkMppEncoder()
{
}

void RkMppEncoder::Init()
{
    v4l2_ctx                = std::make_shared<V4l2Context>();
    mpp_ctx                 = std::make_shared<MppContext>();
    v4l2_ctx->process_image_ = std::bind(&RkMppEncoder::m_process_image, this, std::placeholders::_1, std::placeholders::_2);
    v4l2_ctx->force_format  = 1;
    v4l2_ctx->width         = 640;
    v4l2_ctx->height        = 480;
    v4l2_ctx->pixelformat   = V4L2_PIX_FMT_YUYV;
    v4l2_ctx->field         = V4L2_FIELD_INTERLACED;
    mpp_ctx->width          = v4l2_ctx->width;
    mpp_ctx->height         = v4l2_ctx->height;
    mpp_ctx->fps            = 30;
    mpp_ctx->gop            = 60;
    mpp_ctx->bps            = mpp_ctx->width * mpp_ctx->height / 8 * mpp_ctx->fps * 2;
    mpp_ctx->write_frame_    = std::bind(&RkMppEncoder::write_frame, this, std::placeholders::_1, std::placeholders::_2);

    SpsHeader sps_header;
    v4l2_ctx->open_device((char *)"/dev/video0");
    v4l2_ctx->init_device();
    v4l2_ctx->start_capturing();
    mpp_ctx->write_header(&sps_header);
    v4l2_ctx->main_loop();
}

bool RkMppEncoder::m_process_image(uint8_t *p, int size)
{
    return mpp_ctx->process_image(p, size);
}

bool RkMppEncoder::write_frame(uint8_t *data, int size)
{
    return true;
}