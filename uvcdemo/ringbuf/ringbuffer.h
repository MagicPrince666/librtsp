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
        uint8_t *buf;
        uint64_t size;
        uint64_t in;
        uint64_t out;
    } RingBuffer;

    ~RingBuff();
    static RingBuff& GetInstance() {
        static RingBuff instance;
        return instance;
    }

    bool RingBufferCreate(uint64_t length);
    bool RingBufferReset();

    uint64_t RingBufferRead(uint8_t *target, uint64_t amount);
    uint64_t RingBufferWrite(uint8_t *data, uint64_t length);

private:
    RingBuff(uint64_t rbuf = DEFAULT_BUF_SIZE);
    bool RingBufferEmpty();
    void RingBufferDestroy();

private:
    uint64_t buffer_size_;
    RingBuffer *p_buffer_;
};
