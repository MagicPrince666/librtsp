#ifndef __RK_MPP_ENCODER_H__
#define __RK_MPP_ENCODER_H__

#include "video_capture.h"
#include "v4l2.h"
#include "mpp.h"
#include <iostream>
#include <memory>

class RkMppEncoder {
public:
    RkMppEncoder();
    ~RkMppEncoder();

    void Init();

private:
    V4l2Context *v4l2_ctx;
    MppContext *mpp_ctx;

    bool m_process_image(uint8_t *p, int size);
    bool write_frame(uint8_t*data,int size);
};

#endif
