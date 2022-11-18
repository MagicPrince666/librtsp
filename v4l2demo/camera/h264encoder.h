/**
 * @file h264encoder.h
 * @author 黄李全 (846863428@qq.com)
 * @brief 编码H264
 * @version 0.1
 * @date 2022-11-18
 * @copyright Copyright (c) {2021} 个人版权所有
 */
#pragma once

#include "x264.h"
#include <stdint.h>
#include <stdio.h>

typedef struct {
    x264_param_t *param;
    x264_t *handle;
    x264_picture_t *picture; //说明一个视频序列中每帧特点
    x264_nal_t *nal;
} Encoder;

class H264Encoder
{
public:
    H264Encoder();
    ~H264Encoder();

    //初始化编码器，并返回一个编码器对象
    void CompressBegin(int width, int height);

    //编码一帧
    int CompressFrame(int type, uint8_t *in, uint8_t *out);

private:
    //释放内存
    void CompressEnd();
    int X264ParamApplyPreset(x264_param_t *param, const char *preset);

private:
    Encoder* encode_;
};