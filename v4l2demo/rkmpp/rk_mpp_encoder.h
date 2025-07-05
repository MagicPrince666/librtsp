#ifndef __RK_MPP_ENCODER_H__
#define __RK_MPP_ENCODER_H__

#include "video_capture.h"
#include "video_source.h"
#include <iostream>
#include <memory>
#include <thread>
#include <mutex>
#include <stdint.h>
#include <stdio.h>
#include "calculate.h"

#include <rockchip/mpp_buffer.h>
#include <rockchip/mpp_err.h>
#include <rockchip/mpp_frame.h>
#include <rockchip/mpp_log.h>
#include <rockchip/mpp_packet.h>
#include <rockchip/mpp_rc_defs.h>
#include <rockchip/mpp_task.h>
#include <rockchip/rk_mpi.h>
#include <rga/RgaApi.h>
#define MPP_ALIGN(x, a) (((x) + (a)-1) & ~((a)-1))

class RkMppEncoder : public VideoStream
{
public:
    RkMppEncoder(std::string dev, uint32_t width, uint32_t height, uint32_t fps);
    ~RkMppEncoder();

    int32_t getData(void *fTo, unsigned fMaxSize, unsigned &fFrameSize, unsigned &fNumTruncatedBytes);

private:
    std::shared_ptr<V4l2VideoCapture> v4l2_ctx_;
    std::shared_ptr<Calculate> calculate_ptr_;
    std::mutex data_mtx_;
    uint8_t *camera_buf_;
    uint32_t camera_buf_size_;
    int h264_lenght_;
    std::thread loop_thread_;

    void Init();

    bool mppFrame2H264(const MppFrame frame, uint8_t* data);
};

class MppCamera : public VideoFactory
{
public:
    VideoStream *createVideoStream(std::string dev, uint32_t width, uint32_t height, uint32_t fps)
    {
        return new RkMppEncoder(dev, width, height, fps);
    }
};

#endif
