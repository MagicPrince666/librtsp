#ifndef __CALCULATE_CPU_H__
#define __CALCULATE_CPU_H__

#include "calculate.h"

class CalculateCpu : public Calculate
{
public:
    CalculateCpu();
    ~CalculateCpu();

    void Init();

    void Yuv422Rgb(const uint8_t* yuv, uint8_t* rgb, int width, int height);

    void Nv12Rgb24(const uint8_t* nv12, uint8_t* rgb, int width, int height);

    void Nv12Yuv420p(const uint8_t* nv12, uint8_t* yuv420p, int width, int height);

private:
};

#endif
