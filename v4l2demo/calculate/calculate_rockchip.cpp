#include "calculate_rockchip.h"
#include <unistd.h>

#define MPP_ALIGN(x, a) (((x) + (a)-1) & ~((a)-1))

CalculateRockchip::CalculateRockchip(uint32_t width, uint32_t height) 
: video_width_(width), video_height_(height)
{}

CalculateRockchip::~CalculateRockchip()
{
    if (mpp_frame_buffer_) {
        mpp_buffer_put(mpp_frame_buffer_);
        mpp_frame_buffer_ = nullptr;
    }
    if (mpp_packet_buffer_) {
        mpp_buffer_put(mpp_packet_buffer_);
        mpp_packet_buffer_ = nullptr;
    }
    if (mpp_frame_group_) {
        mpp_buffer_group_put(mpp_frame_group_);
        mpp_frame_group_ = nullptr;
    }
    if (mpp_packet_group_) {
        mpp_buffer_group_put(mpp_packet_group_);
        mpp_packet_group_ = nullptr;
    }
    if (mpp_frame_) {
        mpp_frame_deinit(&mpp_frame_);
        mpp_frame_ = nullptr;
    }
    if (mpp_packet_) {
        mpp_packet_deinit(&mpp_packet_);
        mpp_packet_ = nullptr;
    }
    if (mpp_ctx_) {
        mpp_destroy(mpp_ctx_);
        mpp_ctx_ = nullptr;
    }
}

void CalculateRockchip::Init()
{
    MPP_RET ret = mpp_create(&mpp_ctx_, &mpp_api_);
    if (ret != MPP_OK) {
        std::cerr << "mpp_create failed, ret = " << ret << std::endl;
        throw std::runtime_error("mpp_create failed");
    }
    MpiCmd mpi_cmd     = MPP_CMD_BASE;
    MppParam mpp_param = nullptr;

    mpi_cmd   = MPP_DEC_SET_PARSER_SPLIT_MODE;
    mpp_param = &need_split_;
    ret       = mpp_api_->control(mpp_ctx_, mpi_cmd, mpp_param);
    if (ret != MPP_OK) {
        std::cerr << "mpp_api_->control failed, ret = " << ret << std::endl;
        throw std::runtime_error("mpp_api_->control failed");
    }
    ret = mpp_init(mpp_ctx_, MPP_CTX_DEC, MPP_VIDEO_CodingMJPEG);
    if (ret != MPP_OK) {
        std::cerr << "mpp_init failed, ret = " << ret << std::endl;
        throw std::runtime_error("mpp_init failed");
    }
    MppFrameFormat fmt = MPP_FMT_YUV420SP_VU;
    mpp_param          = &fmt;
    ret                = mpp_api_->control(mpp_ctx_, MPP_DEC_SET_OUTPUT_FORMAT, mpp_param);
    if (ret != MPP_OK) {
        std::cerr << "mpp_api_->control failed, ret = " << ret << std::endl;
        throw std::runtime_error("mpp_api_->control failed");
    }
    ret = mpp_frame_init(&mpp_frame_);
    if (ret != MPP_OK) {
        std::cerr << "mpp_frame_init failed, ret = " << ret << std::endl;
        throw std::runtime_error("mpp_frame_init failed");
    }
    ret = mpp_buffer_group_get_internal(&mpp_frame_group_, MPP_BUFFER_TYPE_ION);
    if (ret != MPP_OK) {
        std::cerr << "mpp_buffer_group_get_internal failed, ret = " << ret << std::endl;
        throw std::runtime_error("mpp_buffer_group_get_internal failed");
    }
    ret = mpp_buffer_group_get_internal(&mpp_packet_group_, MPP_BUFFER_TYPE_ION);
    if (ret != MPP_OK) {
        std::cerr << "mpp_buffer_group_get_internal failed, ret = " << ret << std::endl;
        throw std::runtime_error("mpp_buffer_group_get_internal failed");
    }
    RK_U32 hor_stride = MPP_ALIGN(video_width_, 16);
    RK_U32 ver_stride = MPP_ALIGN(video_height_, 16);
    ret               = mpp_buffer_get(mpp_frame_group_, &mpp_frame_buffer_, hor_stride * ver_stride * 4);
    if (ret != MPP_OK) {
        std::cerr << "mpp_buffer_get failed, ret = " <<  ret << std::endl;
        throw std::runtime_error("mpp_buffer_get failed");
    }
    mpp_frame_set_buffer(mpp_frame_, mpp_frame_buffer_);
    ret = mpp_buffer_get(mpp_packet_group_, &mpp_packet_buffer_, video_width_ * video_height_ * 3);
    if (ret != MPP_OK) {
        std::cerr << "mpp_buffer_get failed, ret = " << ret << std::endl;
        throw std::runtime_error("mpp_buffer_get failed");
    }
    mpp_packet_init_with_buffer(&mpp_packet_, mpp_packet_buffer_);
    data_buffer_ = (uint8_t *)mpp_buffer_get_ptr(mpp_packet_buffer_);

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

    if (format == V4L2_PIX_FMT_MJPEG) {
        return Decode(raw, rgb, width, height);
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

bool CalculateRockchip::mppFrame2RGB(const MppFrame frame, uint8_t *data)
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
    dst_info.format  = RK_FORMAT_RGB_888;
    rga_set_rect(&src_info.rect, 0, 0, width, height, width, height, RK_FORMAT_YCbCr_420_SP);
    rga_set_rect(&dst_info.rect, 0, 0, width, height, width, height, RK_FORMAT_RGB_888);
    int ret = c_RkRgaBlit(&src_info, &dst_info, nullptr);
    if (ret) {
        std::cerr << "c_RkRgaBlit error " << ret << " errno " << strerror(errno) << std::endl;
        return false;
    }
    return true;
}

bool CalculateRockchip::Decode(const uint8_t* raw, uint8_t* rgb, int width, int height) {
  MPP_RET ret = MPP_OK;
  uint32_t camera_size = width * height * 3;
  memset(data_buffer_, 0, width * height * 3);
  memcpy(data_buffer_, raw, camera_size);
  mpp_packet_set_pos(mpp_packet_, data_buffer_);
  mpp_packet_set_length(mpp_packet_, camera_size);
  mpp_packet_set_eos(mpp_packet_);

  ret = mpp_api_->poll(mpp_ctx_, MPP_PORT_INPUT, MPP_POLL_BLOCK);
  if (ret != MPP_OK) {
    std::cerr << "mpp poll failed " << ret << std::endl;
    return false;
  }
  ret = mpp_api_->dequeue(mpp_ctx_, MPP_PORT_INPUT, &mpp_task_);
  if (ret != MPP_OK) {
    std::cerr << "mpp dequeue failed " << ret << std::endl;
    return false;
  }
  mpp_task_meta_set_packet(mpp_task_, KEY_INPUT_PACKET, mpp_packet_);
  mpp_task_meta_set_frame(mpp_task_, KEY_OUTPUT_FRAME, mpp_frame_);
  ret = mpp_api_->enqueue(mpp_ctx_, MPP_PORT_INPUT, mpp_task_);
  if (ret != MPP_OK) {
    std::cerr << "mpp enqueue failed " << ret << std::endl;
    return false;
  }
  ret = mpp_api_->poll(mpp_ctx_, MPP_PORT_OUTPUT, MPP_POLL_BLOCK);
  if (ret != MPP_OK) {
    std::cerr << "mpp poll failed " << ret << std::endl;
    return false;
  }
  ret = mpp_api_->dequeue(mpp_ctx_, MPP_PORT_OUTPUT, &mpp_task_);
  if (ret != MPP_OK) {
    std::cerr << "mpp dequeue failed " << ret << std::endl;
    return false;
  }
  if (mpp_task_) {
    MppFrame output_frame = nullptr;
    mpp_task_meta_get_frame(mpp_task_, KEY_OUTPUT_FRAME, &output_frame);
    if (mpp_frame_) {
      int tmp_width = mpp_frame_get_width(mpp_frame_);
      int tmp_height = mpp_frame_get_height(mpp_frame_);
      if (width != tmp_width || height != tmp_height) {
        std::cerr << "mpp frame size error " << tmp_height << " " << tmp_height << std::endl;
        return false;
      }
      if (!mppFrame2RGB(mpp_frame_, rgb)) {
        std::cerr << "mpp frame to rgb error" << std::endl;
        return false;
      }
      if (mpp_frame_get_eos(output_frame)) {
        std::cout <<  "mpp frame get eos" << std::endl;
      }
    }
    ret = mpp_api_->enqueue(mpp_ctx_, MPP_PORT_OUTPUT, mpp_task_);
    if (ret != MPP_OK) {
      std::cerr << "mpp enqueue failed " << ret << std::endl;
      return false;
    }
    // memcpy(dest, rgb_buffer_, video_width_ * video_height_ * 3);
    return true;
  }
  return false;
}
