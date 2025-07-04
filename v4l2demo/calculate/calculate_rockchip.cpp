#include "calculate_rockchip.h"
#include <unistd.h>

CalculateRockchip::CalculateRockchip() {}

CalculateRockchip::~CalculateRockchip() {}

void CalculateRockchip::Init()
{
    // 初始化 RGA
    if (c_RkRgaInit() != 0) {
        fprintf(stderr, "RGA Init fail\n");
        return;
    }
}

bool CalculateRockchip::Yuv422Rgb(const uint8_t* yuyv, uint8_t* rgb, int width, int height)
{
    // 配置源图像（NV12）
    rga_info_t srcInfo;
    memset(&srcInfo, 0, sizeof(rga_info_t));
    srcInfo.fd = -1;  // 使用虚拟地址
    srcInfo.virAddr = (void*)yuyv;
    srcInfo.mmuFlag = 1;  // 启用MMU

    // 设置源图像区域参数
    rga_set_rect(&srcInfo.rect,
        0, 0,             // 起点坐标
        width, height,     // 宽高
        width, height,     // 虚拟宽高（步长）
        RK_FORMAT_YUYV_422  // YUYV格式
    );
    
    // 配置目标图像（RGB888）
    rga_info_t dstInfo;
    memset(&dstInfo, 0, sizeof(rga_info_t));
    dstInfo.fd = -1;
    dstInfo.virAddr = rgb;
    dstInfo.mmuFlag = 1;

    // 设置目标图像区域参数
    rga_set_rect(&dstInfo.rect,
        0, 0,             // 起点坐标
        width, height,     // 宽高
        width, height, // 步长=宽度x3（RGB888）
        RK_FORMAT_RGB_888  // RGB格式
    );

    int ret = c_RkRgaBlit(&srcInfo, &dstInfo, nullptr);
    if (ret != 0) {
        fprintf(stderr, "RGA transfer fail with: %d\n", ret);
    }
    return true;
}

bool CalculateRockchip::Nv12Rgb24(const uint8_t* nv12, uint8_t* rgb, int width, int height)
{
    if (width <= 0 || height <= 0 || !nv12 || !rgb) {
        std::cerr << "Invalid input parameters" << std::endl;
        return false;
    }

    // 配置源图像（NV12）
    rga_info_t srcInfo;
    memset(&srcInfo, 0, sizeof(rga_info_t));
    srcInfo.fd = -1;  // 使用虚拟地址
    srcInfo.virAddr = (void*)nv12;
    srcInfo.mmuFlag = 1;  // 启用MMU

    // 设置源图像区域参数
    rga_set_rect(&srcInfo.rect,
        0, 0,             // 起点坐标
        width, height,     // 宽高
        width, height,     // 虚拟宽高（步长）
        RK_FORMAT_YCbCr_422_SP  // NV12格式
    );
    
    // 配置目标图像（RGB888）
    rga_info_t dstInfo;
    memset(&dstInfo, 0, sizeof(rga_info_t));
    dstInfo.fd = -1;
    dstInfo.virAddr = rgb;
    dstInfo.mmuFlag = 1;

    // 设置目标图像区域参数
    rga_set_rect(&dstInfo.rect,
        0, 0,             // 起点坐标
        width, height,     // 宽高
        width, height, // 步长=宽度x3（RGB888）
        RK_FORMAT_RGB_888  // RGB格式
    );

    int ret = c_RkRgaBlit(&srcInfo, &dstInfo, nullptr);
    if (ret != 0) {
        fprintf(stderr, "RGA transfer fail with: %d\n", ret);
    }
    return true;
}

bool CalculateRockchip::TransferRgb888(const uint8_t* raw, uint8_t* rgb, int width, int height, const uint32_t format)
{
    if (width <= 0 || height <= 0 || !raw || !rgb) {
        std::cerr << "Invalid input parameters" << std::endl;
        return false;
    }

    // 配置源图像
    rga_info_t srcInfo;
    memset(&srcInfo, 0, sizeof(rga_info_t));
    srcInfo.fd = -1;  // 使用虚拟地址
    srcInfo.virAddr = (void*)raw;
    srcInfo.mmuFlag = 1;  // 启用MMU

    // 设置源图像区域参数
    rga_set_rect(&srcInfo.rect,
        0, 0,             // 起点坐标
        width, height,     // 宽高
        width, height,     // 虚拟宽高（步长）
        0  // raw格式
    );
    // 转换一下
    srcInfo.rect.format = pix_fmt_map_[format];
    
    // 配置目标图像（RGB888）
    rga_info_t dstInfo;
    memset(&dstInfo, 0, sizeof(rga_info_t));
    dstInfo.fd = -1;
    dstInfo.virAddr = rgb;
    dstInfo.mmuFlag = 1;

    // 设置目标图像区域参数
    rga_set_rect(&dstInfo.rect,
        0, 0,             // 起点坐标
        width, height,     // 宽高
        width, height, // 步长=宽度x3（RGB888）
        RK_FORMAT_RGB_888  // RGB格式
    );

    int ret = c_RkRgaBlit(&srcInfo, &dstInfo, nullptr);
    if (ret != 0) {
        fprintf(stderr, "RGA transfer fail with: %d\n", ret);
        return false;
    }
    return true;
}

bool CalculateRockchip::Nv12Yuv420p(const uint8_t* nv12, uint8_t* yuv420p, int width, int height)
{
    if (width <= 0 || height <= 0 || !nv12 || !yuv420p) {
        std::cerr << "Invalid input parameters" << std::endl;
        return false;
    }

    // 配置源图像
    rga_info_t srcInfo;
    memset(&srcInfo, 0, sizeof(rga_info_t));
    srcInfo.fd = -1;  // 使用虚拟地址
    srcInfo.virAddr = (void*)nv12;
    srcInfo.mmuFlag = 1;  // 启用MMU

    // 设置源图像区域参数
    rga_set_rect(&srcInfo.rect,
        0, 0,             // 起点坐标
        width, height,     // 宽高
        width, height,     // 虚拟宽高（步长）
        V4L2_PIX_FMT_NV12  // raw格式
    );
    
    // 配置目标图像（RGB888）
    rga_info_t dstInfo;
    memset(&dstInfo, 0, sizeof(rga_info_t));
    dstInfo.fd = -1;
    dstInfo.virAddr = yuv420p;
    dstInfo.mmuFlag = 1;

    // 设置目标图像区域参数
    rga_set_rect(&dstInfo.rect,
        0, 0,             // 起点坐标
        width, height,     // 宽高
        width, height, // 步长=宽度x3（RGB888）
        RK_FORMAT_YUYV_420  // RGB格式
    );

    int ret = c_RkRgaBlit(&srcInfo, &dstInfo, nullptr);
    if (ret != 0) {
        fprintf(stderr, "RGA transfer fail with: %d\n", ret);
        return false;
    }
    return true;
}
