/**
 * @file ringbuffer.h
 * @author 黄李全 (846863428@qq.com)
 * @brief
 * @version 0.1
 * @date 2022-11-18
 * @copyright Copyright (c) {2021} 个人版权所有
 */

#pragma once

#include <stdint.h>

#define Min(x, y) ((x) < (y) ? (x) : (y))
#define ROUND_UP_2(num) (((num) + 1) & ~1)
#define DEFAULT_BUF_SIZE (2 * 1024 * 1024)

class RingBuff
{
public:
    typedef struct cycle_buffer {
        unsigned char *buf;
        unsigned int size;
        unsigned int in;
        unsigned int out;
    } RingBuffer;

    RingBuff(uint64_t rbuf = DEFAULT_BUF_SIZE);
    ~RingBuff();

    bool RingBufferCreate(uint64_t length);
    bool RingBufferReset();

    uint64_t RingBufferRead(uint8_t *target, uint64_t amount);
    uint64_t RingBufferWrite(uint8_t *data, uint64_t length);

private:
    uint64_t buffer_size_;
    RingBuffer *p_buffer_;

    bool RingBufferEmpty();
    void RingBufferDestroy();
};
