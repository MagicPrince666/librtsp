/*
 * @Author: cijliu
 * @Date: 2021-02-04 16:35:26
 * @LastEditTime: 2021-02-26 16:39:31
 */
#include "rtp.h"
#include "h264.h"
#include <new>

#define RTP_SSRC_DEFAULT (0xd160b58f)
#define RTP_PAYLOAD_DEFAULT (RTP_H264)
#define RTP_MALLOC_REGISTER(hdr) switch (hdr->pt) {
#define RTP_REGISTER(pt, func)          \
    {                                   \
    case pt:                            \
        return func(header, data, len); \
    }

#define RTP_MALLOC_REGISTER_END() }
// void rtp_set_ssrc(rtp_header_t *header, uint32_t ssrc)
// {
//     RTP_ASSERT(header);
//     header->ssrc = htonl(ssrc);
// }

// void rtp_set_pt(rtp_header_t *header, rtp_pt_enum_t e)
// {
//     RTP_ASSERT(header);
//     header->pt = e;
// }

// void rtp_set_ts(rtp_header_t *header, uint32_t ts)
// {
//     RTP_ASSERT(header);
//     header->ts = htonl(ts);
// }

// void rtp_set_seq(rtp_header_t *header, uint16_t seq)
// {
//     RTP_ASSERT(header);
//     header->seq = htons(seq);
// }

Rtp::Rtp() {}

Rtp::~Rtp() {}

void Rtp::HeaderInit(rtp_header_t *header)
{
    RTP_ASSERT(header);
    memset(header, 0, sizeof(rtp_header_t));
    header->v    = 2;
    header->pt   = RTP_PAYLOAD_DEFAULT;
    header->ssrc = htonl(RTP_SSRC_DEFAULT);
}

rtp_packet_t *Rtp::PacketMalloc(rtp_header_t *header, uint8_t *data, uint32_t len)
{
    RTP_ASSERT(header);
    RTP_ASSERT(data);

    RTP_MALLOC_REGISTER(header)
    RTP_REGISTER(RTP_H264, RtpH264PacketMalloc)
    RTP_MALLOC_REGISTER_END()
    return nullptr;
}

void Rtp::PacketFree(rtp_packet_t *pkt)
{
    if (pkt == nullptr) {
        return;
    }
    rtp_packet_t *p = pkt;
    while (p) {
        rtp_packet_t *next = p->next;
        delete p;
        p = next;
    }
    // pkt == NULL;
}

rtp_packet_t *Rtp::PacketNalu(rtp_header_t *header, uint8_t *data, uint32_t len)
{
    RTP_ASSERT(header);
    RTP_ASSERT(data);
    uint32_t payload_size = RTP_MAX_PAYLOAD - sizeof(rtp_header_t);
    if (len > payload_size) {
        return NULL;
    }
    rtp_packet_t *ptk = new rtp_packet_t;
    rtp_header_t *hdr = (rtp_header_t *)ptk->data;
    memset(ptk, 0, sizeof(rtp_packet_t));
    memcpy(hdr, header, sizeof(rtp_header_t));
    memcpy(ptk->data + sizeof(rtp_header_t), data, len);
    hdr->m    = 1;
    hdr->seq  = htons(hdr->seq);
    hdr->ts   = htonl(hdr->ts);
    ptk->next = NULL;
    ptk->len  = len + sizeof(rtp_header_t);
    header->seq++;
    header->ts += RTP_TS_DEFAULT;
    return ptk;
}

rtp_packet_t *Rtp::PacketNaluWithFua(rtp_header_t *header, uint8_t nalu_type, uint8_t *data, uint32_t len)
{
    RTP_ASSERT(header);
    RTP_ASSERT(data);
    uint32_t size         = 0;
    uint32_t payload_size = RTP_MAX_PAYLOAD - sizeof(rtp_header_t) - 2;
    if (len < payload_size) {
        return NULL;
    }
    rtp_packet_t *ptk = new rtp_packet_t;
    rtp_packet_t *cur = ptk;
    memset(ptk, 0, sizeof(rtp_packet_t));
    do {
        memcpy(&cur->nalu.hdr, header, sizeof(rtp_header_t));
        cur->nalu.hdr.seq      = htons(cur->nalu.hdr.seq);
        cur->nalu.hdr.ts       = htonl(cur->nalu.hdr.ts);
        cur->nalu.fu_indicator = (nalu_type & 0xe0) | 28;
        cur->nalu.fu_header    = (nalu_type & 0x1f); // FU-A
        if (cur == ptk) {
            cur->nalu.fu_header |= 0x80; // FU-A
        }
        memcpy(cur->nalu.rbsp, data + size, payload_size);
        size += payload_size;
        len -= payload_size;
        cur->len           = RTP_MAX_PAYLOAD;
        rtp_packet_t *next = new rtp_packet_t;
        memset(next, 0, sizeof(rtp_packet_t));
        cur->next = next;
        cur       = next;
        header->seq++;
    } while (len > payload_size);

    memcpy(&cur->nalu.hdr, header, sizeof(rtp_header_t));
    cur->nalu.hdr.m        = 1;
    cur->nalu.hdr.seq      = htons(cur->nalu.hdr.seq);
    cur->nalu.hdr.ts       = htonl(cur->nalu.hdr.ts);
    cur->nalu.fu_indicator = (nalu_type & 0xe0) | 28;
    cur->nalu.fu_header    = (nalu_type & 0x1f) | 0x40; // FU-A
    memcpy(cur->nalu.rbsp, data + size, len);
    cur->len = len + sizeof(rtp_header_t) + 2;
    header->seq++;
    header->ts += RTP_TS_DEFAULT;
    return ptk;
}

rtp_packet_t *Rtp::RtpH264PacketMalloc(rtp_header_t *header, uint8_t *data, uint32_t len)
{
    uint32_t payload_size = RTP_MAX_PAYLOAD - sizeof(rtp_header_t);
    if (len > payload_size) {
        uint8_t nalu_type  = data[0];
        uint8_t *nalu_rbsp = &data[1];
        uint32_t rbsp_len  = len - 1;
        return PacketNaluWithFua(header, nalu_type, nalu_rbsp, rbsp_len);
    }
    return PacketNalu(header, data, len);
}
