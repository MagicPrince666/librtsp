#include "rk_mpp_encoder.h"

RkMppEncoder::RkMppEncoder()
{
    p_capture_ = std::make_shared<V4l2VideoCapture>("/dev/video0", 640, 480, 30);
}

RkMppEncoder::~RkMppEncoder()
{
    p_capture_ = nullptr;
}

void RkMppEncoder::Init()
{

}