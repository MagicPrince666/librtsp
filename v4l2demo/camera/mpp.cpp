#include "mpp.h"

MppContext::MppContext() {}

MppContext::~MppContext() {
    MppClose();
}

void MppContext::MppClose()
{
    MPP_RET ret = MPP_OK;
    ret         = mpi->reset(ctx);
    if (ret) {
        printf("mpi->reset failed\n");
    }

    if (ctx) {
        mpp_destroy(ctx);
        ctx = NULL;
    }

    if (frm_buf) {
        mpp_buffer_put(frm_buf);
        frm_buf = NULL;
    }
    free(ctx);
}

void MppContext::InitMpp()
{
    MPP_RET ret              = MPP_OK;
    type       = MPP_VIDEO_CodingAVC;
    fmt        = MPP_FMT_YUV422_YUYV;
    hor_stride = MPP_ALIGN(width, 16);
    ver_stride = MPP_ALIGN(height, 16);
    frame_size = hor_stride * ver_stride * 2;

    ret = mpp_buffer_get(NULL, &(frm_buf), frame_size);
    if (ret) {
        printf("failed to get buffer for input frame ret %d\n", ret);
        goto MPP_INIT_OUT;
    }

    ret = mpp_create(&ctx, &mpi);
    if (ret) {
        printf("mpp_create failed ret %d\n", ret);
        goto MPP_INIT_OUT;
    }

    ret = mpp_init(ctx, MPP_CTX_ENC, type);
    if (ret) {
        printf("mpp_init failed ret %d\n", ret);
        goto MPP_INIT_OUT;
    }
    /*Configure Input Control*/

    prep_cfg.change = MPP_ENC_PREP_CFG_CHANGE_INPUT |
                                    MPP_ENC_PREP_CFG_CHANGE_ROTATION |
                                    MPP_ENC_PREP_CFG_CHANGE_FORMAT;
    prep_cfg.width      = width;
    prep_cfg.height     = height;
    prep_cfg.hor_stride = hor_stride;
    prep_cfg.ver_stride = ver_stride;
    prep_cfg.format     = fmt;
    prep_cfg.rotation   = MPP_ENC_ROT_0;

    ret = mpi->control(ctx, MPP_ENC_SET_PREP_CFG, &prep_cfg);
    if (ret) {
        printf("mpi control enc set prep cfg failed ret %d\n", ret);
        goto MPP_INIT_OUT;
    }

    rc_cfg.change  = MPP_ENC_RC_CFG_CHANGE_ALL;
    rc_cfg.rc_mode = MPP_ENC_RC_MODE_CBR;
    rc_cfg.quality = MPP_ENC_RC_QUALITY_MEDIUM;
    if (rc_cfg.rc_mode == MPP_ENC_RC_MODE_CBR) {
        /* constant bitrate has very small bps range of 1/16 bps */
        rc_cfg.bps_target = bps;
        rc_cfg.bps_max    = bps * 17 / 16;
        rc_cfg.bps_min    = bps * 15 / 16;
    } else if (rc_cfg.rc_mode == MPP_ENC_RC_MODE_VBR) {
        if (rc_cfg.quality == MPP_ENC_RC_QUALITY_CQP) {
            /* constant QP does not have bps */
            rc_cfg.bps_target = -1;
            rc_cfg.bps_max    = -1;
            rc_cfg.bps_min    = -1;
        } else {
            /* variable bitrate has large bps range */
            rc_cfg.bps_target = bps;
            rc_cfg.bps_max    = bps * 17 / 16;
            rc_cfg.bps_min    = bps * 1 / 16;
        }
    }

    /* fix input / output frame rate */
    rc_cfg.fps_in_flex    = 0;
    rc_cfg.fps_in_num     = fps;
    rc_cfg.fps_in_denom  = 1;
    rc_cfg.fps_out_flex   = 0;
    rc_cfg.fps_out_num    = fps;
    rc_cfg.fps_out_denom = 1;
    rc_cfg.gop            = gop;
    rc_cfg.skip_cnt       = 0;

    ret = mpi->control(ctx, MPP_ENC_SET_RC_CFG, &rc_cfg);
    if (ret) {
        printf("mpi control enc set rc cfg failed ret %d\n", ret);
        goto MPP_INIT_OUT;
    }

    codec_cfg.coding = type;
    switch (codec_cfg.coding) {
    case MPP_VIDEO_CodingAVC: {
        codec_cfg.h264.change = MPP_ENC_H264_CFG_CHANGE_PROFILE |
                                              MPP_ENC_H264_CFG_CHANGE_ENTROPY |
                                              MPP_ENC_H264_CFG_CHANGE_TRANS_8x8;
        /*
         * H.264 profile_idc parameter
         * 66  - Baseline profile
         * 77  - Main profile
         * 100 - High profile
         */
        codec_cfg.h264.profile = 100;
        /*
         * H.264 level_idc parameter
         * 10 / 11 / 12 / 13    - qcif@15fps / cif@7.5fps / cif@15fps / cif@30fps
         * 20 / 21 / 22         - cif@30fps / half-D1@@25fps / D1@12.5fps
         * 30 / 31 / 32         - D1@25fps / 720p@30fps / 720p@60fps
         * 40 / 41 / 42         - 1080p@30fps / 1080p@30fps / 1080p@60fps
         * 50 / 51 / 52         - 4K@30fps
         */
        codec_cfg.h264.level               = 31;
        codec_cfg.h264.entropy_coding_mode = 1;
        codec_cfg.h264.cabac_init_idc      = 0;
        codec_cfg.h264.transform8x8_mode   = 1;
    } break;
    case MPP_VIDEO_CodingMJPEG: {
        codec_cfg.jpeg.change = MPP_ENC_JPEG_CFG_CHANGE_QP;
        codec_cfg.jpeg.quant  = 10;
    } break;
    case MPP_VIDEO_CodingVP8:
    case MPP_VIDEO_CodingHEVC:
    default: {
        printf("support encoder coding type %d\n", codec_cfg.coding);
    } break;
    }

    ret = mpi->control(ctx, MPP_ENC_SET_CODEC_CFG, &codec_cfg);
    if (ret) {
        printf("mpi control enc set codec cfg failed ret %d\n", ret);
        goto MPP_INIT_OUT;
    }

    /* optional */
    sei_mode = MPP_ENC_SEI_MODE_ONE_FRAME;
    ret                    = mpi->control(ctx, MPP_ENC_SET_SEI_CFG, &sei_mode);
    if (ret) {
        printf("mpi control enc set sei cfg failed ret %d\n", ret);
        goto MPP_INIT_OUT;
    }

    return;

MPP_INIT_OUT:

    if (ctx) {
        mpp_destroy(ctx);
        ctx = NULL;
    }

    if (frm_buf) {
        mpp_buffer_put(frm_buf);
        frm_buf = NULL;
    }

    printf("init mpp failed!\n");
}

bool MppContext::WriteHeader(SpsHeader *sps_header)
{
    int ret;
    if (type == MPP_VIDEO_CodingAVC) {
        MppPacket packet = NULL;
        ret              = mpi->control(ctx, MPP_ENC_GET_EXTRA_INFO, &packet);
        if (ret) {
            printf("mpi control enc get extra info failed\n");
            return 1;
        }

        /* get and write sps/pps for H.264 */
        if (packet) {
            void *ptr        = mpp_packet_get_pos(packet);
            size_t len       = mpp_packet_get_length(packet);
            sps_header->data = (uint8_t *)malloc(len);
            sps_header->size = len;
            memcpy(sps_header->data, ptr, len);
            packet = NULL;
        }
    }
    return 1;
}

bool MppContext::ProcessImage(const uint8_t *p, const uint32_t size)
{
    MPP_RET ret      = MPP_OK;
    MppFrame frame   = NULL;
    MppPacket packet = NULL;

    void *buf = mpp_buffer_get_ptr(frm_buf);
    // TODO: improve performance here?
    memcpy(buf, p, size);
    ret = mpp_frame_init(&frame);
    if (ret) {
        printf("mpp_frame_init failed\n");
        return 1;
    }

    mpp_frame_set_width(frame, width);
    mpp_frame_set_height(frame, height);
    mpp_frame_set_hor_stride(frame, hor_stride);
    mpp_frame_set_ver_stride(frame, ver_stride);
    mpp_frame_set_fmt(frame, fmt);
    mpp_frame_set_buffer(frame, frm_buf);
    mpp_frame_set_eos(frame, frm_eos);

    ret = mpi->encode_put_frame(ctx, frame);
    if (ret) {
        printf("mpp encode put frame failed\n");
        return 1;
    }

mdddd:
    ret = mpi->encode_get_packet(ctx, &packet);
    if (ret) {
        printf("mpp encode get packet failed\n");
        return 1;
    }

    if (packet) {
        // write packet to file here
        void *ptr             = mpp_packet_get_pos(packet);
        size_t len            = mpp_packet_get_length(packet);
        pkt_eos = mpp_packet_get_eos(packet);
        if (write_frame_)
            if (!write_frame_((uint8_t*)ptr, len))
                printf("------------sendok!\n");

        mpp_packet_deinit(&packet);
        printf("encoded frame %d size %ld\n", frame_count, len);
        stream_size += len;
        frame_count++;

        if (pkt_eos) {
            printf("found last packet\n");
        }
    } else {
        goto mdddd;
    }
    if (num_frames && frame_count >= num_frames) {
        printf("encode max %d frames", frame_count);
        return 0;
    }
    if (frm_eos && pkt_eos) {
        return 0;
    }

    return 1;
}