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
#include <dirent.h>
#include <sys/vfs.h>

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

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

std::string V4l2H264hData::getCurrentTime8() {
    std::time_t result = std::time(nullptr) + 8 * 3600;
    auto sec           = std::chrono::seconds(result);
    std::chrono::time_point<std::chrono::system_clock> now(sec);
    auto timet     = std::chrono::system_clock::to_time_t(now);
    auto localTime = *std::gmtime(&timet);

    std::stringstream ss;
    std::string str;
    ss << std::put_time(&localTime, "%Y.%m.%d-%H.%M.%S");
    ss >> str;

    return str;
}

uint64_t V4l2H264hData::DirSize(const char *dir) {
#ifndef _WIN32
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;
    long long int totalSize = 0;
    if ((dp = opendir(dir)) == NULL) {
        fprintf(stderr, "Cannot open dir: %s\n", dir);
        return 0;   //可能是个文件，或者目录不存在
    }

    //先加上自身目录的大小
    lstat(dir, &statbuf);
    totalSize += statbuf.st_size;

    while ((entry = readdir(dp)) != NULL) {
        char subdir[256];
        sprintf(subdir, "%s/%s", dir, entry->d_name);
        lstat(subdir, &statbuf);
        if (S_ISDIR(statbuf.st_mode)) {
            if (strcmp(".", entry->d_name) == 0 || strcmp("..", entry->d_name) == 0) {
                continue;
            }

            uint64_t subDirSize = DirSize(subdir);
            totalSize += subDirSize;
        } else {
            totalSize += statbuf.st_size;
        }
    }

    closedir(dp);
    return totalSize;
#else
    return 0;
#endif
}

bool V4l2H264hData::RmDirFiles(const std::string &path) {
#ifndef _WIN32
    std::string strPath = path;
    if (strPath.at(strPath.length() - 1) != '\\' || strPath.at(strPath.length() - 1) != '/') {
        strPath.append("/");
    }

    DIR *directory = opendir(strPath.c_str());   //打开这个目录
    if (directory != NULL) {
        for (struct dirent *dt = readdir(directory); dt != nullptr;
             dt                = readdir(directory)) {   //逐个读取目录中的文件到dt
            //系统有个系统文件，名为“..”和“.”,对它不做处理
            if (strcmp(dt->d_name, "..") != 0 && strcmp(dt->d_name, ".") != 0) {   //判断是否为系统隐藏文件
                struct stat st;                                                    //文件的信息
                std::string fileName;                                              //文件夹中的文件名
                fileName = strPath + std::string(dt->d_name);
                stat(fileName.c_str(), &st);
                if (!S_ISDIR(st.st_mode)) {
                    // 删除文件即可
                    remove(fileName.c_str());
                }
            }
        }
        closedir(directory);
    }

#endif
    return true;
}
