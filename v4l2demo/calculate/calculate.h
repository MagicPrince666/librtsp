#ifndef __CALCULATE_H__
#define __CALCULATE_H__

#include <iostream>

class Calculate {
public:
    Calculate() {}
    ~Calculate() {}

    virtual void Init() = 0;

    virtual void Yuv422Rgb(const uint8_t* yuv, uint8_t* rgb, int width, int height) = 0;

    virtual void Nv12Rgb24(const uint8_t* nv12, uint8_t* rgb, int width, int height) = 0;

    virtual void Nv12Yuv420p(const uint8_t* nv12, uint8_t* yuv, int width, int height) = 0;
};

#endif
