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

#include "h264_camera.h"
#include "h264encoder.h"
#include "ringbuffer.h"

V4l2H264hData::V4l2H264hData(uint64_t size) :
cam_mbuf_size_(size)
{
    s_b_running_  = true;
    s_pause_       = false;
    h264_fp_ = nullptr;
    h264_file_name_ = "test.264";
}

V4l2H264hData::~V4l2H264hData()
{
    if (video_capture_thread_.joinable()) {
        video_capture_thread_.join();
    }
    if (video_encode_thread_.joinable()) {
        video_encode_thread_.join();
    }
    CloseFile();
    if (p_capture_) {
        delete p_capture_;
    }
    delete[] cam_data_buff_[0].cam_mbuf;
    delete[] cam_data_buff_[1].cam_mbuf;
}

void V4l2H264hData::Init()
{
    buff_full_flag_[0] = 0;
    buff_full_flag_[1] = 0;

    cam_data_buff_[0].rpos = 0;
    cam_data_buff_[0].wpos = 0;
    cam_data_buff_[1].rpos = 0;
    cam_data_buff_[1].wpos = 0;

    cam_data_buff_[0].cam_mbuf = new (std::nothrow) uint8_t[cam_mbuf_size_];
    cam_data_buff_[1].cam_mbuf = new (std::nothrow) uint8_t[cam_mbuf_size_];

    p_capture_ = new (std::nothrow) V4l2VideoCapture("/dev/video0");
    p_capture_->Init(); // 初始化摄像头

    InitFile();         // 存储264文件

    video_capture_thread_ = std::thread([](V4l2H264hData *p_this) { p_this->VideoCaptureThread(); }, this);
    video_encode_thread_  = std::thread([](V4l2H264hData *p_this) { p_this->VideoEncodeThread(); }, this);
}

void V4l2H264hData::VideoCaptureThread()
{
    int i   = 0;
    int len = p_capture_->FrameLength();

    struct timeval now;
    struct timespec outtime;

    while (s_b_running_) {
        if (s_pause_) {
            usleep(50000);
            continue;
        }
        usleep(DelayTime);

        gettimeofday(&now, nullptr);
        outtime.tv_sec = now.tv_sec;
        outtime.tv_nsec = DelayTime * 1000;

        cam_data_buff_[i].lock.lock(); /*获取互斥锁,锁定当前缓冲区*/

        while ((cam_data_buff_[i].wpos + len) % cam_mbuf_size_ == cam_data_buff_[i].rpos && cam_data_buff_[i].rpos != 0) {
            /*等待缓存区处理操作完成*/
            // pthread_cond_timedwait(&(cam_data_buff_[i].encodeOK), &(cam_data_buff_[i].lock), &outtime);
        }

        int length = p_capture_->BuffOneFrame(cam_data_buff_[i].cam_mbuf, cam_data_buff_[i].wpos, cam_mbuf_size_);
        cam_data_buff_[i].wpos += length;
        if (cam_data_buff_[i].wpos + length > cam_mbuf_size_) {
            //缓冲区剩余空间不够存放当前帧数据，切换下一缓冲区
            // pthread_cond_signal(&(cam_data_buff_[i].captureOK)); /*设置状态信号*/
            cam_data_buff_[i].lock.unlock(); /*释放互斥锁*/
            buff_full_flag_[i] = true;  //缓冲区i已满
            cam_data_buff_[i].rpos = 0;
            i = !i;                     //切换到另一个缓冲区
            cam_data_buff_[i].wpos = 0;
            buff_full_flag_[i] = false; //缓冲区i为空
        }

        // pthread_cond_signal(&(cam_data_buff_[i].captureOK)); /*设置状态信号*/
        cam_data_buff_[i].lock.unlock(); /*释放互斥锁*/
    }
}

void V4l2H264hData::VideoEncodeThread()
{
    int i = -1;
    H264Encoder encoder;
    encoder.CompressBegin(p_capture_->GetWidth(), p_capture_->GetHeight());
    // 设置缓冲区
    h264_buf_ = new (std::nothrow) uint8_t[sizeof(uint8_t) * p_capture_->GetWidth() * p_capture_->GetHeight() * 3];

    while (s_b_running_) {
        if (s_pause_) {
            usleep(50000);
            continue;
        }

        if (buff_full_flag_[1] == false && buff_full_flag_[0] == false) {
            continue;
        }

        if (buff_full_flag_[0]) {
            i = 0;
        }

        if (buff_full_flag_[1]) {
            i = 1;
        }

        cam_data_buff_[i].lock.lock();

        /*H.264压缩视频*/
        int h264_length = encoder.CompressFrame(FRAME_TYPE_AUTO, cam_data_buff_[i].cam_mbuf + cam_data_buff_[i].rpos, h264_buf_);

        if (h264_length > 0) {
            RINGBUF.Write(h264_buf_, h264_length);
            if(h264_fp_) {
                fwrite(h264_buf_, h264_length, 1, h264_fp_);
            }
        }

        cam_data_buff_[i].rpos += p_capture_->FrameLength();
        if (cam_data_buff_[i].rpos >= cam_mbuf_size_) {
            cam_data_buff_[i].rpos  = 0;
            cam_data_buff_[!i].rpos = 0;
            buff_full_flag_[i]      = false;
        }

        /*H.264压缩视频*/
        // pthread_cond_signal(&(cam_data_buff_[i].encodeOK));
        cam_data_buff_[i].lock.unlock(); /*释放互斥锁*/
    }

    delete[] h264_buf_;
    h264_buf_ = nullptr;
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
    fFrameSize = 0;
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
    spdlog::debug("FetchData stopCap");
}

void V4l2H264hData::InitFile()
{
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