/*
 * @Author: cijliu
 * @Date: 2021-02-08 19:51:36
 * @LastEditTime: 2021-02-26 13:55:05
 */
#ifndef __H264_H__
#define __H264_H__

#include "rtp.h"

#include <stdint.h>
#include <stdio.h>

#define H264_NAL(v) (v & 0x1F)

typedef enum {
    H264_NAL_PFRAME = 1,
    H264_NAL_IDR    = 5,
    H264_NAL_SEI    = 6,
    H264_NAL_SPS    = 7,
    H264_NAL_PPS    = 8
} h264_nalu_enum_t;

typedef struct h264_nalu {
    h264_nalu_enum_t type;
    uint8_t *data;
    uint32_t len;
    struct h264_nalu *next;
} h264_nalu_t;

// #define H264FUN H264::GetInstance()

class H264
{
public:
    H264();
    ~H264();
    // static H264& GetInstance() {
    //     static H264 instance;
    //     return instance;
    // }

    h264_nalu_t *NalPacketMalloc(uint8_t *buf, int len);
    void NalPacketFree(h264_nalu_t *nal);

private:
    uint8_t *NaluFind(uint8_t *ptr, uint8_t *end);
    uint8_t *SearchStartCode(uint8_t *ptr, uint8_t *end);
    int NalType(uint8_t *ptr);
    int NalNewAccess(uint8_t *ptr, uint8_t *end);
    uint8_t *NalDataPtr(uint8_t *ptr);
    int NalIsHeader(uint8_t *ptr);
    uint8_t *NextNal(uint8_t *ptr, uint8_t *end);
};

#endif
