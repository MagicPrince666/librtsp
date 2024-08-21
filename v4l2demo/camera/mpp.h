#ifndef _MPP_H
#define _MPP_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include <rockchip/rk_mpi.h>
#include <functional>
#include <iostream>

#define MPP_ALIGN(x, a)         (((x)+(a)-1)&~((a)-1))

typedef struct {
        uint8_t *data;
        uint32_t size;
} SpsHeader;

class MppContext
{
public:
        MppContext();
        ~MppContext();
        // paramter for resource malloc
        RK_U32 width;
        RK_U32 height;
        RK_U32 hor_stride;
        RK_U32 ver_stride;
        // rate control runtime parameter
        RK_S32 gop;
        RK_S32 fps;
        RK_S32 bps;

        //call back function
        std::function<bool(uint8_t *, int)> write_frame_;
        //function pointer
        bool process_image(uint8_t *p, int size);
        bool write_header(SpsHeader *sps_header);
        void mpp_close();
private:
        // global flow control flag
        RK_U32 frm_eos;
        RK_U32 pkt_eos;
        RK_U32 frame_count;
        RK_U64 stream_size;

        // base flow context
        MppCtx ctx;
        MppApi *mpi;
        MppEncPrepCfg prep_cfg;
        MppEncRcCfg rc_cfg;
        MppEncCodecCfg codec_cfg;

        // input / output
        MppBuffer frm_buf;
        MppEncSeiMode sei_mode;

        MppFrameFormat fmt;
        MppCodingType type;
        RK_U32 num_frames;

        // resources
        size_t frame_size;
        /* NOTE: packet buffer may overflow */
        size_t packet_size;

        
        FILE *fp_output;
        FILE *fp_outputx;
        void init_mpp();
};


#endif /* !_MPP_H */