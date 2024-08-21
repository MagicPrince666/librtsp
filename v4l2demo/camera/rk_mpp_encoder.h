#ifndef __RK_MPP_ENCODER_H__
#define __RK_MPP_ENCODER_H__

#include "video_capture.h"
#include <iostream>
#include <memory>

class RkMppEncoder {
public:
    RkMppEncoder();
    ~RkMppEncoder();

    void Init();

private:
    std::shared_ptr<V4l2VideoCapture> p_capture_;
};

#endif
