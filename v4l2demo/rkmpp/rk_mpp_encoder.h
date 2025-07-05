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
#include "calculate_rockchip.h"

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

    MppCtx mpp_ctx_ = nullptr;
    MppApi* mpp_api_ = nullptr;
    MppPacket mpp_packet_ = nullptr;
    MppFrame mpp_frame_ = nullptr;
    MppDecCfg mpp_dec_cfg_ = nullptr;
    MppBuffer mpp_frame_buffer_ = nullptr;
    MppBuffer mpp_packet_buffer_ = nullptr;
    uint8_t* data_buffer_ = nullptr;
    MppBufferGroup mpp_frame_group_ = nullptr;
    MppBufferGroup mpp_packet_group_ = nullptr;
    MppTask mpp_task_ = nullptr;
    uint32_t need_split_ = 0;
    uint8_t* rgb_buffer_ = nullptr;

    void Init();

    bool mppFrame2RGB(const MppFrame frame, uint8_t* data);

    bool Decode(uint8_t* dest);
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
