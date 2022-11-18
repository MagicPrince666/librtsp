/*****************************************************************************************

 * 文件名  H264_UVC_TestAP.cpp
 * 描述    ：录制H264裸流
 * 平台    ：linux
 * 版本    ：V1.0.0
 * 作者    ：Leo Huang  QQ：846863428
 * 邮箱    ：Leo.huang@junchentech.cn
 * 修改时间  ：2017-06-28

*****************************************************************************************/
#include <chrono>
#include <errno.h>
#include <fcntl.h>
#include <functional>
#include <iostream>
#include <linux/videodev2.h>
#include <memory>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "H264_UVC_Cap.h"
#include "ringbuffer.h"

#define CLEAR(x) memset(&(x), 0, sizeof(x))

H264UvcCap::H264UvcCap(std::string dev, uint32_t width, uint32_t height)
: v4l2_device_(dev),
video_width_(width),
video_height_(height)
{
    capturing_ = true;
    buffers_   = nullptr;
    n_buffers_ = 0;
    rec_fp1_   = nullptr;
}

H264UvcCap::~H264UvcCap()
{
    if (rec_fp1_) {
        fclose(rec_fp1_);
    }
    if (video_) {
        if (video_->fd) {
            close(video_->fd);
        }
        close_v4l2(video_);
    }
}

int errnoexit(const char *s)
{
    printf("%s error %d, %s", s, errno, strerror(errno));
    return -1;
}

int xioctl(int fd, int request, void *arg)
{
    int r;

    do
        r = ioctl(fd, request, arg);
    while (-1 == r && EINTR == errno);

    return r;
}

bool H264UvcCap::CreateFile(bool yes)
{
    if (yes) { // 不创建文件
        return false;
    }
    // remove("Record.264");
    time_t tt        = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::string file = "/tmp/" + std::to_string(tt) + ".264";
    rec_fp1_         = fopen(file.c_str(), "a+b");
    if (rec_fp1_ == nullptr) {
        return false;
    }
    return true;
}

bool H264UvcCap::OpenDevice()
{
    struct stat st;

    if (-1 == stat(v4l2_device_.c_str(), &st)) {
        printf("Cannot identify '%s': %d, %s", v4l2_device_.c_str(), errno, strerror(errno));
        return false;
    }

    if (!S_ISCHR(st.st_mode)) {
        printf("%s is no device", v4l2_device_.c_str());
        return false;
    }
    video_     = (struct vdIn *)calloc(1, sizeof(struct vdIn));
    video_->fd = open(v4l2_device_.c_str(), O_RDWR);

    if (-1 == video_->fd) {
        printf("Cannot open '%s': %d, %s", v4l2_device_.c_str(), errno, strerror(errno));
        return false;
    }
    return true;
}

int H264UvcCap::InitMmap(void)
{
    struct v4l2_requestbuffers req;

    CLEAR(req);
    req.count  = 4;
    req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (-1 == xioctl(video_->fd, VIDIOC_REQBUFS, &req)) {
        if (EINVAL == errno) {
            printf("%s does not support memory mapping", v4l2_device_.c_str());
            return -1;
        } else {
            return errnoexit("VIDIOC_REQBUFS");
        }
    }

    if (req.count < 2) {
        printf("Insufficient buffer memory on %s", v4l2_device_.c_str());
        return -1;
    }

    buffers_ = (buffer *)calloc(req.count, sizeof(*buffers_));

    if (!buffers_) {
        printf("Out of memory");
        return -1;
    }

    for (n_buffers_ = 0; n_buffers_ < req.count; ++n_buffers_) {
        struct v4l2_buffer buf;
        CLEAR(buf);

        buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index  = n_buffers_;

        if (-1 == xioctl(video_->fd, VIDIOC_QUERYBUF, &buf))
            return errnoexit("VIDIOC_QUERYBUF");

        buffers_[n_buffers_].length = buf.length;
        buffers_[n_buffers_].start =
            mmap(NULL,
                 buf.length,
                 PROT_READ | PROT_WRITE,
                 MAP_SHARED,
                 video_->fd, buf.m.offset);

        if (MAP_FAILED == buffers_[n_buffers_].start)
            return errnoexit("mmap");
    }

    return 0;
}

int H264UvcCap::InitDevice(int width, int height, int format)
{
    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_format fmt;
    unsigned int min;

    if (-1 == xioctl(video_->fd, VIDIOC_QUERYCAP, &cap)) {
        if (EINVAL == errno) {
            printf("%s is no V4L2 device", v4l2_device_.c_str());
            return -1;
        } else {
            return errnoexit("VIDIOC_QUERYCAP");
        }
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        printf("%s is no video capture device", v4l2_device_.c_str());
        return -1;
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        printf("%s does not support streaming i/o", v4l2_device_.c_str());
        return -1;
    }

    CLEAR(cropcap);
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (0 == xioctl(video_->fd, VIDIOC_CROPCAP, &cropcap)) {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c    = cropcap.defrect;

        if (-1 == xioctl(video_->fd, VIDIOC_S_CROP, &crop)) {
            switch (errno) {
            case EINVAL:
                break;
            default:
                break;
            }
        }
    }

    CLEAR(fmt);

    fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = width;
    fmt.fmt.pix.height      = height;
    fmt.fmt.pix.pixelformat = format;
    fmt.fmt.pix.field       = V4L2_FIELD_ANY;

    if (-1 == xioctl(video_->fd, VIDIOC_S_FMT, &fmt))
        return errnoexit("VIDIOC_S_FMT");

    min = fmt.fmt.pix.width * 2;

    if (fmt.fmt.pix.bytesperline < min)
        fmt.fmt.pix.bytesperline = min;
    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
    if (fmt.fmt.pix.sizeimage < min)
        fmt.fmt.pix.sizeimage = min;

    struct v4l2_streamparm parm;
    memset(&parm, 0, sizeof parm);
    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(video_->fd, VIDIOC_G_PARM, &parm);
    parm.parm.capture.timeperframe.numerator   = 1;
    parm.parm.capture.timeperframe.denominator = 30;
    ioctl(video_->fd, VIDIOC_S_PARM, &parm);

    return InitMmap();
}

int H264UvcCap::StartPreviewing(void)
{
    uint32_t i;
    enum v4l2_buf_type type;

    for (i = 0; i < n_buffers_; ++i) {
        struct v4l2_buffer buf;
        CLEAR(buf);

        buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index  = i;

        if (-1 == xioctl(video_->fd, VIDIOC_QBUF, &buf))
            return errnoexit("VIDIOC_QBUF");
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (-1 == xioctl(video_->fd, VIDIOC_STREAMON, &type))
        return errnoexit("VIDIOC_STREAMON");

    return 0;
}

bool H264UvcCap::InitH264Camera(void)
{
    int format = V4L2_PIX_FMT_H264;

    bool ret = -1;

    if (OpenDevice()) {
        ret = InitDevice(video_width_, video_height_, format);
    } else {
        return false;
    }

    if (ret != -1) {
        ret = StartPreviewing();
    } else {
        return false;
    }

    if (ret < 0) {
        return false;
    }

    struct tm *tdate;
    time_t curdate;
    tdate = localtime(&curdate);
    XU_OSD_Set_CarcamCtrl(video_->fd, 0, 0, 0);
    if (XU_OSD_Set_RTC(video_->fd, tdate->tm_year + 1900, tdate->tm_mon + 1, tdate->tm_mday, tdate->tm_hour, tdate->tm_min, tdate->tm_sec) < 0)
        printf("XU_OSD_Set_RTC_fd = %d Failed\n", video_->fd);
    if (XU_OSD_Set_Enable(video_->fd, 1, 1) < 0)
        printf(" XU_OSD_Set_Enable_fd = %d Failed\n", video_->fd);

    ret = XU_Init_Ctrl(video_->fd);
    if (ret < 0) {
        printf("XU_H264_Set_BitRate Failed\n");
    } else {
        double m_BitRate = 0.0;
        if (XU_H264_Set_BitRate(video_->fd, 4096 * 1024) < 0) //设置码率
        {
            printf("XU_H264_Set_BitRate Failed\n");
        }

        XU_H264_Get_BitRate(video_->fd, &m_BitRate);
        if (m_BitRate < 0) {
            printf("XU_H264_Get_BitRate Failed\n");
        }

        printf("-----XU_H264_Set_BitRate %fbps----\n", m_BitRate);
    }

    CreateFile(true);
    return true;
}

int64_t H264UvcCap::CapVideo()
{
    struct v4l2_buffer buf;
    CLEAR(buf);

    buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    int ret = ioctl(video_->fd, VIDIOC_DQBUF, &buf);
    if (ret < 0) {
        printf("Unable to dequeue buffer!\n");
        return -1;
    }

    if (rec_fp1_) {
        fwrite(buffers_[buf.index].start, buf.bytesused, 1, rec_fp1_);
    }

    RINGBUF.Write((uint8_t *)buffers_[buf.index].start, buf.bytesused);

    ret = ioctl(video_->fd, VIDIOC_QBUF, &buf);

    if (ret < 0) {
        printf("Unable to requeue buffer");
        return -1;
    }

    return buf.bytesused;
}
