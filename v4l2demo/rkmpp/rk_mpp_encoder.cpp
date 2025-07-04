#include "rk_mpp_encoder.h"
#include <iostream>

RkMppEncoder::RkMppEncoder(std::string dev, uint32_t width, uint32_t height, uint32_t fps)
    : VideoStream(dev, width, height, fps)
{
    Init();
}

RkMppEncoder::~RkMppEncoder()
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
    if (rgb_buffer_) {
        delete[] rgb_buffer_;
    }
}

void RkMppEncoder::Init()
{
    v4l2_ctx = std::make_shared<V4l2VideoCapture>(dev_name_, video_width_, video_height_, video_fps_);
    // v4l2_ctx->AddCallback(std::bind(&RkMppEncoder::ProcessImage, this, std::placeholders::_1, std::placeholders::_2));
    rgb_buffer_ = new uint8_t[video_width_ * video_height_ * 3];
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

    camera_buf_ = new (std::nothrow) uint8_t[video_width_ * video_height_ * 2];

    v4l2_ctx->Init(V4L2_PIX_FMT_MJPEG); // V4L2_PIX_FMT_MJPEG

    calculate_ptr_ = std::make_shared<CalculateRockchip>();
    calculate_ptr_->Init();
}

bool RkMppEncoder::mppFrame2RGB(const MppFrame frame, uint8_t *data)
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

bool RkMppEncoder::Decode(uint8_t *dest) {
  MPP_RET ret = MPP_OK;
  memset(data_buffer_, 0, video_width_ * video_height_ * 3);
  // memcpy(data_buffer_, frame->data(), frame->dataSize());
  mpp_packet_set_pos(mpp_packet_, data_buffer_);
  // mpp_packet_set_length(mpp_packet_, frame->dataSize());
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
      int width = mpp_frame_get_width(mpp_frame_);
      int height = mpp_frame_get_height(mpp_frame_);
      if (width != video_width_ || height != video_height_) {
        std::cerr << "mpp frame size error " << width << " " << height << std::endl;
        return false;
      }
      if (!mppFrame2RGB(mpp_frame_, rgb_buffer_)) {
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
    memcpy(dest, rgb_buffer_, video_width_ * video_height_ * 3);
    return true;
  }
  return false;
}

int32_t RkMppEncoder::getData(void *fTo, unsigned fMaxSize, unsigned &fFrameSize, unsigned &fNumTruncatedBytes)
{
    std::unique_lock<std::mutex> lck(data_mtx_);
    uint64_t lenght = v4l2_ctx->BuffOneFrame(camera_buf_);

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
