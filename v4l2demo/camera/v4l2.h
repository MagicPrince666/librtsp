#ifndef _V4L2_H
#define _V4L2_H

#include <functional>
#include <iostream>
#include <linux/videodev2.h>
#include <memory>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <vector>

enum IO_METHOD {
    IO_METHOD_READ,
    IO_METHOD_MMAP,
};

struct buffer {
    void *start;
    size_t length;
};

class V4l2Context
{
public:
    V4l2Context();
    ~V4l2Context();
    bool force_format;
    uint32_t width;
    uint32_t height;
    uint32_t pixelformat;
    uint32_t field;

    /*call back function*/
    std::function<bool(uint8_t *, int)> process_image_;

    int OpenDevice(const char *device);
    /*function pointer*/
    int InitDevice();
    int StartCapturing();
    void MainLoop();

private:
    int32_t fd;
    enum IO_METHOD io_method;
    const char *dev_name;
    struct buffer *buffers;
    uint32_t n_buffers;

    int V4l2Close();
    int InitMmap();
    int InitRead(unsigned int buffer_size);
    int xioctl(int fh, int request, void *arg);
    int ReadFrame();
};

#endif /* !_V4L2_H */
