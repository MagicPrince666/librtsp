#include <asm/types.h> /* for videodev2.h */
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h> /* low-level i/o */
#include <linux/videodev2.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "spdlog/spdlog.h"
#include "spdlog/cfg/env.h"  // support for loading levels from the environment variable
#include "spdlog/fmt/ostr.h" // support for user defined types

#include "video_capture.h"

#define CLEAR(x) memset(&(x), 0, sizeof(x))

V4l2VideoCapture::V4l2VideoCapture(std::string dev) :
v4l2_device_(dev) {
    h264_fp_ = nullptr;
}

V4l2VideoCapture::~V4l2VideoCapture() {
    V4l2Close();
}

void V4l2VideoCapture::ErrnoExit(const char *s)
{
    fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
    exit(EXIT_FAILURE);
}

static int xioctl(int fd, int request, void *arg)
{
    int r = 0;
    do {
        r = ioctl(fd, request, arg);
    } while (-1 == r && EINTR == errno);

    return r;
}

void V4l2VideoCapture::OpenCamera()
{
    camera_ = (struct camera *)malloc(sizeof(struct camera));
    if (!camera_) {
        spdlog::error("malloc camera failure!");
        return;
    }

    camera_->device_name = (char*)v4l2_device_.c_str();
    camera_->buffers     = nullptr;
    camera_->width       = SET_WIDTH;
    camera_->height      = SET_HEIGHT;
    camera_->fps         = 30; //设置30fps失败

    struct stat st;

    if (-1 == stat(camera_->device_name, &st)) {
        fprintf(stderr, "Cannot identify '%s': %d, %s\n", camera_->device_name,
                errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (!S_ISCHR(st.st_mode)) {
        fprintf(stderr, "%s is no device\n", camera_->device_name);
        exit(EXIT_FAILURE);
    }

    camera_->fd = open(camera_->device_name, O_RDWR, 0); //  | O_NONBLOCK

    if (-1 == camera_->fd) {
        fprintf(stderr, "Cannot open '%s': %d, %s\n", camera_->device_name, errno,
                strerror(errno));
        exit(EXIT_FAILURE);
    }
}

int V4l2VideoCapture::FrameLength()
{
    return sizeof(uint8_t) * camera_->width * camera_->height * 2;
}

void V4l2VideoCapture::CloseCamera()
{
    if (-1 == close(camera_->fd)) {
        ErrnoExit("close");
    }

    camera_->fd = -1;
    free(camera_);
}

void V4l2VideoCapture::InitFile()
{
    h264_fp_ = fopen(h264_file_name_.c_str(), "wa+");
}

void V4l2VideoCapture::CloseFile()
{
    fclose(h264_fp_);
}

void V4l2VideoCapture::InitEncoder()
{
    h264_buf_ = new (std::nothrow) uint8_t [sizeof(uint8_t) * camera_->width * camera_->height * 3]; // 设置缓冲区
}

void V4l2VideoCapture::CloseEncoder()
{
    delete[] h264_buf_;
}

uint8_t* V4l2VideoCapture::GetUint8tH264Buf() {
    return h264_buf_;
}

int V4l2VideoCapture::BuffOneFrame(uint8_t* data, int32_t offset)
{
    int len = 0;
    struct v4l2_buffer buf;
    CLEAR(buf);

    buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    // this operator below will change buf.index and (0 <= buf.index <= 3)
    if (-1 == ioctl(camera_->fd, VIDIOC_DQBUF, &buf)) {
        switch (errno) {
        case EAGAIN:
            return 0;
        case EIO:
            /* Could ignore EIO, see spec. */
            /* fall through */
        default:
            ErrnoExit("VIDIOC_DQBUF");
        }
    }

    len =(size_t)buf.bytesused;//当前帧的长度
    if(offset + len <= BUF_SIZE) {
        memcpy(data + offset, (uint8_t *)(camera_->buffers[buf.index].start) ,len);//把一帧数据拷贝到缓冲区
    }

    if (-1 == ioctl(camera_->fd, VIDIOC_QBUF, &buf)) {
        ErrnoExit("VIDIOC_QBUF");
    }

    return len;
}

void V4l2VideoCapture::StartCapturing()
{
    unsigned int i;
    enum v4l2_buf_type type;

    for (i = 0; i < n_buffers_; ++i) {
        struct v4l2_buffer buf;

        CLEAR(buf);

        buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index  = i;

        if (-1 == xioctl(camera_->fd, VIDIOC_QBUF, &buf)) {
            ErrnoExit("VIDIOC_QBUF");
        }
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (-1 == xioctl(camera_->fd, VIDIOC_STREAMON, &type)) {
        ErrnoExit("VIDIOC_STREAMON");
    }
}

void V4l2VideoCapture::StopCapturing()
{
    enum v4l2_buf_type type;

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (-1 == xioctl(camera_->fd, VIDIOC_STREAMOFF, &type)) {
        ErrnoExit("VIDIOC_STREAMOFF");
    }
}

void V4l2VideoCapture::UninitCamera()
{
    unsigned int i;

    for (i = 0; i < n_buffers_; ++i) {
        if (-1 == munmap(camera_->buffers[i].start, camera_->buffers[i].length)) {
            ErrnoExit("munmap");
        }
    }

    free(camera_->buffers);
}

void V4l2VideoCapture::InitMmap()
{
    struct v4l2_requestbuffers req;

    CLEAR(req);

    req.count  = 4;
    req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    //分配内存
    if (-1 == xioctl(camera_->fd, VIDIOC_REQBUFS, &req)) {
        if (EINVAL == errno) {
            fprintf(stderr, "%s does not support "
                            "memory mapping\n",
                    camera_->device_name);
            exit(EXIT_FAILURE);
        } else {
            ErrnoExit("VIDIOC_REQBUFS");
        }
    }

    if (req.count < 2) {
        fprintf(stderr, "Insufficient buffer memory on %s\n", camera_->device_name);
        exit(EXIT_FAILURE);
    }

    camera_->buffers = (buffer *)calloc(req.count, sizeof(*(camera_->buffers)));

    if (!camera_->buffers) {
        fprintf(stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }

    for (n_buffers_ = 0; n_buffers_ < req.count; ++n_buffers_) {
        struct v4l2_buffer buf;

        CLEAR(buf);

        buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index  = n_buffers_;

        //将VIDIOC_REQBUFS中分配的数据缓存转换成物理地址
        if (-1 == xioctl(camera_->fd, VIDIOC_QUERYBUF, &buf)) {
            ErrnoExit("VIDIOC_QUERYBUF");
        }

        camera_->buffers[n_buffers_].length = buf.length;
        camera_->buffers[n_buffers_].start  = mmap(NULL /* start anywhere */,
                                              buf.length, PROT_READ | PROT_WRITE /* required */,
                                              MAP_SHARED /* recommended */, camera_->fd, buf.m.offset);

        if (MAP_FAILED == camera_->buffers[n_buffers_].start) {
            ErrnoExit("mmap");
        }
    }
}

int V4l2VideoCapture::GetWidth()
{
    return camera_->width;
}
int V4l2VideoCapture::GetHeight() {
    return camera_->height;
}

void V4l2VideoCapture::InitCamera()
{
    struct v4l2_format *fmt = &(camera_->v4l2_fmt);

    CLEAR(*fmt);

    fmt->type           = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt->fmt.pix.width  = camera_->width;
    fmt->fmt.pix.height = camera_->height;
    // fmt->fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV; //yuv422
    fmt->fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;   // yuv420 但是我电脑不支持
    fmt->fmt.pix.field       = V4L2_FIELD_INTERLACED; //隔行扫描

    if (-1 == xioctl(camera_->fd, VIDIOC_S_FMT, fmt)) {
        ErrnoExit("VIDIOC_S_FMT");
    }

    // struct v4l2_streamparm parm;
    // memset(&parm, 0, sizeof(parm));
    // parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    // //if (-1 == xioctl(camera_->fd, VIDIOC_G_PARM, &parm))
    // //	ErrnoExit("VIDIOC_G_PARM");
    // parm.parm.capture.capturemode = V4L2_MODE_HIGHQUALITY;
    // //  parm.parm.capture.capability = V4L2_CAP_TIMEPERFRAME;
    // parm.parm.capture.timeperframe.numerator = 1;
    // parm.parm.capture.timeperframe.denominator = camera_->fps;
    // if (-1 == xioctl(camera_->fd, VIDIOC_S_PARM, &parm))
    // 	ErrnoExit("VIDIOC_S_PARM");
    struct v4l2_streamparm *parm = (struct v4l2_streamparm *)malloc(sizeof(struct v4l2_streamparm));
    memset(parm, 0, sizeof(struct v4l2_streamparm));
    parm->type                     = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    parm->parm.capture.capturemode = V4L2_MODE_HIGHQUALITY;
    //  parm->parm.capture.capability = V4L2_CAP_TIMEPERFRAME;
    parm->parm.capture.timeperframe.denominator = 30; //时间间隔分母
    parm->parm.capture.timeperframe.numerator   = 1;  //分子
    if (-1 == ioctl(camera_->fd, VIDIOC_S_PARM, parm)) {
        perror("set param:");
        exit(EXIT_FAILURE);
    }

    // get message
    if (-1 == xioctl(camera_->fd, VIDIOC_G_PARM, parm)) {
        ErrnoExit("VIDIOC_G_PARM");
    }
    printf("get fps = %d\n", parm->parm.capture.timeperframe.denominator);

    free(parm);
    InitMmap();
}

void V4l2VideoCapture::Init()
{
    OpenCamera();
    InitCamera();
    StartCapturing();
    InitEncoder();
    // InitFile();
}

void V4l2VideoCapture::V4l2Close()
{
    StopCapturing();
    UninitCamera();
    CloseCamera();
    // CloseFile();
    CloseEncoder();
}
