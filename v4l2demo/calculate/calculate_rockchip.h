#ifndef __CALCULATE_ROCKCHIP_H__
#define __CALCULATE_ROCKCHIP_H__

#include "calculate.h"
#include <unordered_map>
#include <rga/RgaApi.h>
#include <linux/videodev2.h>

class CalculateRockchip : public Calculate
{
public:
    CalculateRockchip();
    ~CalculateRockchip();

    void Init();

    bool Yuv422Rgb(const uint8_t* yuv, uint8_t* rgb, int width, int height);

    bool Nv12Rgb24(const uint8_t* nv12, uint8_t* rgb, int width, int height);

    bool TransferRgb888(const uint8_t* raw, uint8_t* rgb, int width, int height, const uint32_t format);

    bool Nv12Yuv420p(const uint8_t* nv12, uint8_t* yuv420p, int width, int height);

private:
    std::unordered_map<uint32_t, uint32_t> pix_fmt_map_ = {
        {V4L2_PIX_FMT_UYVY, RK_FORMAT_UYVY_422},
        {V4L2_PIX_FMT_NV12, RK_FORMAT_YCbCr_420_SP},
        {V4L2_PIX_FMT_NV16, RK_FORMAT_YCbCr_422_SP},
        {V4L2_PIX_FMT_YUYV, RK_FORMAT_YUYV_422},
    };
};

#endif
