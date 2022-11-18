#include "ringbuffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

RingBuff::RingBuff(uint64_t rbuf) : buffer_size_(rbuf)
{
}

RingBuff::~RingBuff()
{
    RingBuffer_destroy();
}

bool RingBuff::RingBuffer_create(uint64_t length)
{
    uint64_t size = ROUND_UP_2(length);

    if ((size & (size - 1)) || (size < DEFAULT_BUF_SIZE)) {
        size = DEFAULT_BUF_SIZE;
    }

    p_buffer_ = (RingBuffer *)malloc(sizeof(RingBuffer));
    if (!p_buffer_) {
        return false;
    }
    memset(p_buffer_, 0, sizeof(RingBuffer));

    p_buffer_->size = size;
    p_buffer_->in   = 0;
    p_buffer_->out  = 0;

    p_buffer_->buf = (unsigned char *)malloc(size);
    if (!p_buffer_->buf) {
        free(p_buffer_);
        return false;
    }

    memset(p_buffer_->buf, 0, size);

    return true;
}

void RingBuff::RingBuffer_destroy()
{
    if (p_buffer_) {
        free(p_buffer_->buf);
        free(p_buffer_);
    }
}

bool RingBuff::RingBuffer_Reset()
{
    if (p_buffer_ == NULL) {
        return false;
    }

    p_buffer_->in  = 0;
    p_buffer_->out = 0;
    memset(p_buffer_->buf, 0, p_buffer_->size);

    return true;
}

bool RingBuff::RingBuffer_empty()
{
    return p_buffer_->in == p_buffer_->out;
}

uint64_t RingBuff::RingBuffer_write(uint8_t *data, uint64_t length)
{
    uint64_t len = 0;

    length = Min(length, p_buffer_->size - p_buffer_->in + p_buffer_->out);
    len    = Min(length, p_buffer_->size - (p_buffer_->in & (p_buffer_->size - 1)));

    memcpy(p_buffer_->buf + (p_buffer_->in & (p_buffer_->size - 1)), data, len);
    memcpy(p_buffer_->buf, data + len, length - len);

    p_buffer_->in += length;

    return length;
}

uint64_t RingBuff::RingBuffer_read(uint8_t *target, uint64_t amount)
{
    uint64_t len = 0;

    amount = Min(amount, p_buffer_->in - p_buffer_->out);
    len    = Min(amount, p_buffer_->size - (p_buffer_->out & (p_buffer_->size - 1)));

    memcpy(target, p_buffer_->buf + (p_buffer_->out & (p_buffer_->size - 1)), len);
    memcpy(target + len, p_buffer_->buf, amount - len);

    p_buffer_->out += amount;

    return amount;
}
