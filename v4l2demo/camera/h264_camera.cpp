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

static bool s_quit = true;

pthread_t thread[3];
int flag[2], point = 0;

V4l2H264hData::V4l2H264hData()
{
    s_b_running_ = false;
    s_source_    = nullptr;
}

V4l2H264hData::~V4l2H264hData()
{
    SoftUinit();
    if (video_capture_thread_.joinable()) {
        video_capture_thread_.join();
    }
    if (video_encode_thread_.joinable()) {
        video_encode_thread_.join();
    }
}

void V4l2H264hData::Init()
{
    flag[0] = flag[1] = 0;

    cam_data_buff_ = new (std::nothrow) cam_data;

    pthread_mutex_init(&cam_data_buff_->lock, NULL); //以动态方式创建互斥锁

    pthread_cond_init(&cam_data_buff_->captureOK, NULL); //初始化captureOK条件变量

    pthread_cond_init(&cam_data_buff_->encodeOK, NULL); //初始化encodeOK条件变量

    cam_data_buff_->rpos = 0;
    cam_data_buff_->wpos = 0;

    // if (cam_data_buff_->wpos + len <= BUF_SIZE) { //缓冲区剩余空间足够存放当前帧数据
    //     memcpy(cam_data_buff_->cam_mbuf + cam_data_buff_->wpos, data, len); //把一帧数据拷贝到缓冲区
    //     cam_data_buff_->wpos += len;
    // }

    // if (cam_data_buff_->wpos + len > BUF_SIZE) {
    //     //缓冲区剩余空间不够存放当前帧数据，切换下一缓冲区
    //     return;
    // }

    SoftInit();

    video_capture_thread_ = std::thread([](V4l2H264hData *p_this) { p_this->VideoCaptureThread(); }, this);
    video_encode_thread_  = std::thread([](V4l2H264hData *p_this) { p_this->VideoEncodeThread(); }, this);
}

void V4l2H264hData::VideoCaptureThread()
{

    int i   = 0;
    int len = capture_.FrameLength();

    struct timeval now;
    struct timespec outtime;

    while (1) {
        if (s_quit) {
            usleep(10);
            continue;
        }
        usleep(DelayTime);

        gettimeofday(&now, NULL);

        outtime.tv_sec = now.tv_sec;

        outtime.tv_nsec = DelayTime * 1000;

        pthread_mutex_lock(&(cam_data_buff_[i].lock)); /*获取互斥锁,锁定当前缓冲区*/

        while ((cam_data_buff_[i].wpos + len) % BUF_SIZE == cam_data_buff_[i].rpos && cam_data_buff_[i].rpos != 0) {
            /*等待缓存区处理操作完成*/
            pthread_cond_timedwait(&(cam_data_buff_[i].encodeOK), &(cam_data_buff_[i].lock), &outtime);
        }

        // &cam_data_buff_[i]
        struct v4l2_buffer v4l2_buf;
        v4l2_buf = capture_.BuffOneFrame();
        int length = v4l2_buf.bytesused;

        if (cam_data_buff_->wpos + length <= BUF_SIZE) {
            //缓冲区剩余空间足够存放当前帧数据
            memcpy(cam_data_buff_->cam_mbuf + cam_data_buff_->wpos, capture_.GetData(v4l2_buf), length); //把一帧数据拷贝到缓冲区

            cam_data_buff_->wpos += length;
        }

        if (length > 0) {
            //采集一帧数据

            pthread_cond_signal(&(cam_data_buff_[i].captureOK)); /*设置状态信号*/

            pthread_mutex_unlock(&(cam_data_buff_[i].lock)); /*释放互斥锁*/

            flag[i] = 1; //缓冲区i已满

            cam_data_buff_[i].rpos = 0;

            i = !i; //切换到另一个缓冲区

            cam_data_buff_[i].wpos = 0;

            flag[i] = 0; //缓冲区i为空
        }

        pthread_cond_signal(&(cam_data_buff_[i].captureOK)); /*设置状态信号*/

        pthread_mutex_unlock(&(cam_data_buff_[i].lock)); /*释放互斥锁*/
    }
}

void V4l2H264hData::VideoEncodeThread()
{
    int i = -1;
    H264Encoder encoder;
    encoder.CompressBegin(capture_.GetWidth(), capture_.GetHeight());
    while (1) {
        if (s_quit) {
            usleep(10);
            continue;
        }

        if ((flag[1] == 0 && flag[0] == 0) || (flag[i] == -1)) {
            continue;
        }

        if (flag[0] == 1) {
            i = 0;
        }

        if (flag[1] == 1) {
            i = 1;
        }

        pthread_mutex_lock(&(cam_data_buff_[i].lock)); /*获取互斥锁*/

        /*H.264压缩视频*/
        int h264_length = encoder.CompressFrame(-1, cam_data_buff_[i].cam_mbuf + cam_data_buff_[i].rpos, capture_.GetUint8tH264Buf());

        if (h264_length > 0) {

            // printf("%s%d\n","-----------h264_length=",h264_length);
            //写h264文件
            // fwrite(capture_.GetUint8tH264Buf(), h264_length, 1, h264_fp);
#if 1
            RINGBUF.Write(capture_.GetUint8tH264Buf(), h264_length);
#else
            fwrite(capture_.GetUint8tH264Buf(), h264_length, 1, h264_fp);
#endif
            // printf("buf front:%d rear :%d\n",q.front,q.rear);
        }

        cam_data_buff_[i].rpos += capture_.FrameLength();

        if (cam_data_buff_[i].rpos >= BUF_SIZE) {
            cam_data_buff_[i].rpos  = 0;
            cam_data_buff_[!i].rpos = 0;
            flag[i]                 = -1;
        }

        /*H.264压缩视频*/
        pthread_cond_signal(&(cam_data_buff_[i].encodeOK));

        pthread_mutex_unlock(&(cam_data_buff_[i].lock)); /*释放互斥锁*/
    }
}

void V4l2H264hData::SoftInit()
{
    spdlog::info("Camera init");
}

void V4l2H264hData::SoftUinit()
{
    spdlog::info("Camera uinit");
}



int V4l2H264hData::getData(void *fTo, unsigned fMaxSize, unsigned &fFrameSize, unsigned &fNumTruncatedBytes)
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

void V4l2H264hData::EmptyBuffer()
{
    empty_buffer_ = true;
}

void V4l2H264hData::startCap()
{
    s_b_running_ = true;
    if (!s_quit) {
        return;
    }
    s_quit = false;
    SoftInit();
    spdlog::debug("FetchData startCap");
}

void V4l2H264hData::stopCap()
{
    s_b_running_ = false;
    s_quit       = true;
    SoftUinit();
    spdlog::debug("FetchData stopCap");
}
