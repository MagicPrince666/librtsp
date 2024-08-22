#include "v4l2.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

V4l2Context::V4l2Context() {}

V4l2Context::~V4l2Context()
{
    V4l2Close();
}

int V4l2Context::V4l2Close()
{
    enum v4l2_buf_type type;
    unsigned int i;

    switch (io_method) {
    case IO_METHOD_READ:
        /* Nothing to do. */
        break;
    case IO_METHOD_MMAP:
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        xioctl(fd, VIDIOC_STREAMOFF, &type);
        break;
    }
    switch (io_method) {
    case IO_METHOD_READ:
        free(buffers[0].start);
        break;
    case IO_METHOD_MMAP:
        for (i = 0; i < n_buffers; ++i)
            munmap(buffers[i].start, buffers[i].length);
        break;
    }
    free(buffers);
    close(fd);
    return 0;
}

int V4l2Context::xioctl(int fh, int request, void *arg)
{
    int r;
    do {
        r = ioctl(fh, request, arg);
    } while (-1 == r && EINTR == errno);

    return r;
}

int V4l2Context::OpenDevice(const char *device)
{
    dev_name = device;
    int ret;
    struct stat st;
    if (stat(device, &st) == -1) {
        fprintf(stderr, "Cannot identify '%s': %d, %s\n", device, errno, strerror(errno));
        return -1;
    }

    if (!S_ISCHR(st.st_mode)) {
        fprintf(stderr, "%s is not device", dev_name);
        return -1;
    }

    fd = open(device, O_RDWR /* required */ | O_NONBLOCK, 0);
    if (fd == -1) {
        fprintf(stderr, "Cannot open '%s': %d, %s\\n", device, errno, strerror(errno));
        return -1;
    }
    return 0;
}

int V4l2Context::InitDevice()
{
    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_format fmt;
    unsigned int min;

    if (xioctl(fd, VIDIOC_QUERYCAP, &cap) == -1) {
        fprintf(stderr, "get VIDIOC_QUERYCAP error: %d, %s\n", errno, strerror(errno));
        return -1;
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        fprintf(stderr, "%s is not video capture device\n", dev_name);
        return -1;
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        fprintf(stderr, "%s does not support streaming i/o\n", dev_name);

        if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
            fprintf(stderr, "%s does not support read i/o\n", dev_name);
            return -1;
        }
        io_method = IO_METHOD_READ;
    } else {
        io_method = IO_METHOD_MMAP;
    }
    /* Select video input, video standard and tune here. */
    memset(&cropcap, 0, sizeof(cropcap));
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (xioctl(fd, VIDIOC_CROPCAP, &cropcap) == 0) {
        memset(&crop, 0, sizeof(crop));
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c    = cropcap.defrect; /* reset to default */

        if (xioctl(fd, VIDIOC_S_CROP, &crop) == -1) {
            fprintf(stderr, "set VIDIOC_S_CROP failed: %d, %s\n", errno, strerror(errno));
        }
    } else {
        fprintf(stderr, "get VIDIOC_CROPCAP failed: %d, %s\n", errno, strerror(errno));
    }

    /* Enum pixel format */
    for (int i = 0; i < 20; i++) {
        struct v4l2_fmtdesc fmtdesc;
        fmtdesc.index = i;
        fmtdesc.type  = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (xioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc) == -1)
            break;
        printf("%d: %s\n", i, fmtdesc.description);
    }

    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (force_format) {
        fmt.fmt.pix.width       = width;
        fmt.fmt.pix.height      = height;
        fmt.fmt.pix.pixelformat = pixelformat;
        fmt.fmt.pix.field       = field;

        if (xioctl(fd, VIDIOC_S_FMT, &fmt) == -1) {
            fprintf(stderr, "get VIDIOC_S_FMT failed: %d, %s\n", errno, strerror(errno));
            return -1;
        }

        /* Note VIDIOC_S_FMT may change width and height. */
    } else {
        /* Preserve original settings as set by v4l2-ctl for example */
        if (xioctl(fd, VIDIOC_G_FMT, &fmt) == -1) {
            fprintf(stderr, "get VIDIOC_G_FMT failed: %d, %s\n", errno, strerror(errno));
            return -1;
        }
    }
    printf("fmt.w=%d,fmt.h=%d\n", fmt.fmt.pix.width, fmt.fmt.pix.height);
    printf("fmt.pixfmt=0x%x\n", fmt.fmt.pix.pixelformat);

    /* Buggy driver paranoia. */
    min = fmt.fmt.pix.width * 2;
    if (fmt.fmt.pix.bytesperline < min)
        fmt.fmt.pix.bytesperline = min;
    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
    if (fmt.fmt.pix.sizeimage < min)
        fmt.fmt.pix.sizeimage = min;

    if (io_method == IO_METHOD_MMAP) {
        return InitMmap();
    } else {
        return InitRead(fmt.fmt.pix.sizeimage);
    }
    return 0;
}

void V4l2Context::MainLoop()
{
    int fd = fd;
    int r;
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    for (;;) {
        struct timeval tv;
        tv.tv_sec  = 2;
        tv.tv_usec = 0;

        fd_set rdset = fds;
        r            = select(fd + 1, &rdset, NULL, NULL, &tv);

        if (r > 0) {
            if (ReadFrame() == -2) {
                break;
            }
        } else if (r == 0) {
            fprintf(stderr, "select timeout\n");
        } else {
            if (EINTR == errno || EAGAIN == errno) {
                continue;
            }
            fprintf(stderr, "select failed: %d, %s\n", errno, strerror(errno));
            break;
        }
        /* EAGAIN - continue select loop. */
    }
}

int V4l2Context::InitMmap()
{
    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));

    req.count  = 4;
    req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (xioctl(fd, VIDIOC_REQBUFS, &req) == -1) {
        fprintf(stderr, "set VIDIOC_REQBUFS failed: %d, %s\n", errno, strerror(errno));
        return -1;
    }

    if (req.count < 2) {
        fprintf(stderr, "Insufficient buffer memory on %s\n", dev_name);
        return -1;
    }

    buffers = (struct buffer *)calloc(req.count, sizeof(struct buffer));

    if (!buffers) {
        fprintf(stderr, "Out of memory\n");
        return -1;
    }

    for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index  = n_buffers;

        if (xioctl(fd, VIDIOC_QUERYBUF, &buf) == -1) {
            fprintf(stderr, "set VIDIOC_QUERYBUF %u failed: %d, %s\n", n_buffers, errno, strerror(errno));
            return -1;
        }

        buffers[n_buffers].length = buf.length;
        buffers[n_buffers].start =
            mmap(NULL /* start anywhere */,
                 buf.length,
                 PROT_READ | PROT_WRITE /* required */,
                 MAP_SHARED /* recommended */,
                 fd, buf.m.offset);

        if (MAP_FAILED == buffers[n_buffers].start) {
            fprintf(stderr, "mmap %u failed: %d, %s\n", n_buffers, errno, strerror(errno));
            return -1;
        }
    }

    return 0;
}

int V4l2Context::InitRead(unsigned int buffer_size)
{
    buffers = (struct buffer *)calloc(1, sizeof(struct buffer));

    if (!buffers) {
        fprintf(stderr, "Out of memory\n");
        return -1;
    }

    buffers[0].length = buffer_size;
    buffers[0].start  = malloc(buffer_size);

    if (!buffers[0].start) {
        fprintf(stderr, "Out of memory\n");
        return -1;
    }

    return 0;
}

int V4l2Context::StartCapturing()
{
    unsigned int i;
    enum v4l2_buf_type type;

    switch (io_method) {
    case IO_METHOD_READ:
        /* Nothing to do. */
        break;
    case IO_METHOD_MMAP:
        for (i = 0; i < n_buffers; ++i) {
            struct v4l2_buffer buf;
            memset(&buf, 0, sizeof(buf));

            buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index  = i;

            if (xioctl(fd, VIDIOC_QBUF, &buf) == -1) {
                fprintf(stderr, "set VIDIOC_QBUF failed: %d, %s\n", errno, strerror(errno));
                return -1;
            }
        }
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (xioctl(fd, VIDIOC_STREAMON, &type) == -1) {
            fprintf(stderr, "set VIDIOC_STREAMON failed: %d, %s\n", errno, strerror(errno));
            return -1;
        }
        break;
    }
    return 0;
}

int V4l2Context::ReadFrame()
{
    struct v4l2_buffer buf;
    switch (io_method) {
    case IO_METHOD_READ:
        if (read(fd, buffers[0].start, buffers[0].length) == -1) {
            if (errno == EAGAIN || errno == EINTR) {
                return 0;
            } else {
                fprintf(stderr, "read failed: %d, %s\n", errno, strerror(errno));
                return -1;
            }
        }
        if (!process_image_((uint8_t *)buffers[0].start, buffers[0].length)) {
            return -2;
        }
        break;

    case IO_METHOD_MMAP:
        memset(&buf, 0, sizeof(buf));

        buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

        if (xioctl(fd, VIDIOC_DQBUF, &buf) == -1) {
            if (errno == EAGAIN || errno == EINTR) {
                return 0;
            } else {
                fprintf(stderr, "set VIDIOC_DQBUF failed: %d, %s\n", errno, strerror(errno));
                return -1;
            }
        }
        if (buf.index < n_buffers) {
            if (!process_image_((uint8_t *)buffers[buf.index].start, buf.bytesused)) {
                return -2;
            }
            if (-1 == xioctl(fd, VIDIOC_QBUF, &buf)) {
                fprintf(stderr, "set VIDIOC_QBUF failed: %d, %s\n", errno, strerror(errno));
                return -1;
            }
        }
        break;
    }
    return 0;
}