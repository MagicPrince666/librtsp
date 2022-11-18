#include "ringbuffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

RingBuff::RingBuff(uint64_t rbuf) : buffer_size_(rbuf)
{
}

RingBuff::~RingBuff()
{
    RingBufferDestroy();
}

bool RingBuff::RingBufferCreate(uint64_t length)
{
    uint64_t size = ROUND_UP_2(length);

    if ((size & (size - 1)) || (size < DEFAULT_BUF_SIZE)) {
        size = DEFAULT_BUF_SIZE;
    }

    p_buffer_ =  new RingBuffer;
    if (!p_buffer_) {
        return false;
    }
    memset(p_buffer_, 0, sizeof(RingBuffer));

    p_buffer_->size = size;
    p_buffer_->in   = 0;
    p_buffer_->out  = 0;

    p_buffer_->buf = new uint8_t[size];
    if (!p_buffer_->buf) {
        delete p_buffer_;
        return false;
    }

    memset(p_buffer_->buf, 0, size);

    return true;
}

void RingBuff::RingBufferDestroy()
{
    if (p_buffer_) {
        delete[] p_buffer_->buf;
        delete p_buffer_;
    }
}

bool RingBuff::RingBufferReset()
{
    if (p_buffer_ == nullptr) {
        return false;
    }

    p_buffer_->in  = 0;
    p_buffer_->out = 0;
    memset(p_buffer_->buf, 0, p_buffer_->size);

    return true;
}

bool RingBuff::RingBufferEmpty()
{
    return p_buffer_->in == p_buffer_->out;
}

uint64_t RingBuff::RingBufferWrite(uint8_t *data, uint64_t length)
{
    uint64_t len = 0;

    length = Min(length, p_buffer_->size - p_buffer_->in + p_buffer_->out);
    len    = Min(length, p_buffer_->size - (p_buffer_->in & (p_buffer_->size - 1)));

    memcpy(p_buffer_->buf + (p_buffer_->in & (p_buffer_->size - 1)), data, len);
    memcpy(p_buffer_->buf, data + len, length - len);

    p_buffer_->in += length;

    return length;
}

uint64_t RingBuff::RingBufferRead(uint8_t *target, uint64_t amount)
{
    uint64_t len = 0;

    amount = Min(amount, p_buffer_->in - p_buffer_->out);
    len    = Min(amount, p_buffer_->size - (p_buffer_->out & (p_buffer_->size - 1)));

    memcpy(target, p_buffer_->buf + (p_buffer_->out & (p_buffer_->size - 1)), len);
    memcpy(target + len, p_buffer_->buf, amount - len);

    p_buffer_->out += amount;

    return amount;
}
