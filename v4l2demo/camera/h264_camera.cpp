#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "spdlog/cfg/env.h"  // support for loading levels from the environment variable
#include "spdlog/fmt/ostr.h" // support for user defined types
#include "spdlog/spdlog.h"

#include "epoll.h"
#include "h264_camera.h"
#include "h264encoder.h"
#include "ringbuffer.h"

#define USE_BUF_LIST 0

V4l2H264hData::V4l2H264hData(std::string dev) : v4l2_device_(dev)
{
    s_b_running_    = true;
    s_pause_        = false;
    h264_fp_        = nullptr;
    h264_file_name_ = "h264_encoder.h264";
}

V4l2H264hData::~V4l2H264hData()
{
    if (video_encode_thread_.joinable()) {
        video_encode_thread_.join();
    }

    CloseFile();

    if (p_capture_) {
        delete p_capture_;
    }

    if(h264_buf_) {
        delete[] h264_buf_;
    }

    if(camera_buf_) {
        delete[] camera_buf_;
    }
}

void V4l2H264hData::Init()
{
    p_capture_ = new (std::nothrow) V4l2VideoCapture(v4l2_device_.c_str());
    p_capture_->Init(); // 初始化摄像头

    camera_buf_ = new (std::nothrow) uint8_t[p_capture_->GetFrameLength()];

    encoder_ = new (std::nothrow) H264Encoder(p_capture_->GetWidth(), p_capture_->GetHeight());
    encoder_->Init();

#if USE_BUF_LIST
#else
    // 申请H264缓存
    h264_buf_ = new (std::nothrow) uint8_t[p_capture_->GetFrameLength()];
#endif

    InitFile(); // 存储264文件

    video_encode_thread_ = std::thread([](V4l2H264hData *p_this) { p_this->VideoEncodeThread(); }, this);
}

void V4l2H264hData::RecordAndEncode()
{
    int32_t len = p_capture_->BuffOneFrame(camera_buf_);

    if(len <= 0) {
        return;
    }

#if USE_BUF_LIST
    Buffer h264_buf;
    memset(&h264_buf, 0, sizeof(struct Buffer));
    /*H.264压缩视频*/
    encoder_->CompressFrame(FRAME_TYPE_AUTO, camera_buf_, h264_buf.buf_ptr, h264_buf.length);

    if (h264_buf.length > 0) {
        // RINGBUF.Write(h264_buf.buf_ptr, h264_buf.length);
        if (h264_fp_) {
            fwrite(h264_buf.buf_ptr, h264_buf.length, 1, h264_fp_);
        }
    } else {
        spdlog::info("get size after encoder = {}", h264_buf.length);
    }

    if(h264_buf.buf_ptr) {
        delete[] h264_buf.buf_ptr;
    }
#else
    uint64_t length = 0;
    encoder_->CompressFrame(FRAME_TYPE_AUTO, camera_buf_, h264_buf_, length);

    if (length > 0) {
        // RINGBUF.Write(h264_buf_, h264_buf.length);
        if (h264_fp_) {
            fwrite(h264_buf_, length, 1, h264_fp_);
        }
    } else {
        spdlog::info("get size after encoder = {}", length);
    }
#endif
}

void V4l2H264hData::VideoEncodeThread()
{
    // 设置缓冲区
    MY_EPOLL.EpollAdd(p_capture_->GetHandle(), std::bind(&V4l2H264hData::RecordAndEncode, this));
    MY_EPOLL.EpollLoop();
}

int32_t V4l2H264hData::getData(void *fTo, unsigned fMaxSize, unsigned &fFrameSize, unsigned &fNumTruncatedBytes)
{
    if (!s_b_running_) {
        spdlog::warn("V4l2H264hData::getData s_b_running_ = false");
        return 0;
    }

#if 1
    if (RINGBUF.Empty()) {
        usleep(100); //等待数据
        fFrameSize         = 0;
        fNumTruncatedBytes = 0;
    }
    fFrameSize = RINGBUF.Write((uint8_t *)fTo, fMaxSize);

    fNumTruncatedBytes = 0;
#else
    fFrameSize         = 0;
    fNumTruncatedBytes = 0;
#endif
    // //拷贝视频到live555缓存
    // if(len < fMaxSize)
    // {
    // 	fFrameSize = len;
    // 	fNumTruncatedBytes = 0;
    // }
    // else
    // {
    // 	fNumTruncatedBytes = len - fMaxSize;
    // 	fFrameSize = fMaxSize;
    // }

    return fFrameSize;
}

void V4l2H264hData::stopCap()
{
    s_b_running_ = false;
    spdlog::info("FetchData stopCap");
}

inline bool FileExists(const std::string& name) {
  struct stat buffer;   
  return (stat (name.c_str(), &buffer) == 0); 
}

void V4l2H264hData::InitFile()
{
    if(FileExists(h264_file_name_)) {
        remove(h264_file_name_.c_str());
    }
    h264_fp_ = fopen(h264_file_name_.c_str(), "wa+");
}

void V4l2H264hData::CloseFile()
{
    fclose(h264_fp_);
}

bool V4l2H264hData::PauseCap(bool pause)
{
    s_pause_ = pause;
    return s_pause_;
}