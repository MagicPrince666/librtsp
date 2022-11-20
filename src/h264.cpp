/*
 * @Author: cijliu
 * @Date: 2021-02-02 15:48:13
 * @LastEditTime: 2021-02-24 17:26:52
 */
#include "h264.h"
#include <sys/stat.h>
#include <assert.h>
#include <new>

H264::H264() {}

H264::~H264() {}

uint8_t *H264::NalDataPtr(uint8_t *ptr)
{
    if (ptr[0] == 0x00 && ptr[1] == 0x00 && ptr[2] == 0x01) {
        return &ptr[3];
    }
    if (ptr[0] == 0x00 && ptr[1] == 0x00 && ptr[2] == 0x00 && ptr[3] == 0x01) {
        return &ptr[4];
    }
    return NULL;
}

int H264::NalIsHeader(uint8_t *ptr)
{
    if (ptr[0] == 0x00 && ptr[1] == 0x00 && ptr[2] == 0x01) {
        return 0;
    }
    if (ptr[0] == 0x00 && ptr[1] == 0x00 && ptr[2] == 0x00 && ptr[3] == 0x01) {
        return 0;
    }
    return -1;
}

uint8_t *H264::NextNal(uint8_t *ptr, uint8_t *end)
{
    uint8_t *p = ptr;
    while (p < end) {
        if (!NalIsHeader(p)) {
            return p;
        }
        p++;
    }
    return NULL;
}

h264_nalu_t *H264::NalPacketMalloc(uint8_t *buf, int len)
{
    uint8_t *end          = buf + len - 1;
    uint8_t *data         = NalDataPtr(buf);
    h264_nalu_t *nalu     = new h264_nalu_t;
    h264_nalu_t *h264_nal = nalu;
    while (data && data < end) {
        memset(nalu, 0, sizeof(h264_nalu_t));
        uint8_t *next = NextNal(data, end);

        nalu->type = (h264_nalu_enum_t)H264_NAL(data[0]);
        nalu->data = data;
        nalu->len  = next - data;
        if (next == NULL) {
            nalu->len = end - data + 1;
            return h264_nal;
        }
        nalu->next = new h264_nalu_t;
        nalu       = nalu->next;
        data       = NalDataPtr(next);
    }
    delete h264_nal;
    return NULL;
}

void H264::NalPacketFree(h264_nalu_t *nal)
{
    h264_nalu_t *p = nal;
    while (p) {
        h264_nalu_t *next = p->next;
        delete p;
        p = next;
    }
    nal = NULL;
}
