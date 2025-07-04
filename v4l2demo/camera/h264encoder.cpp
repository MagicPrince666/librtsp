#include "h264encoder.h"
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(__linux__)
#include <linux/videodev2.h>
#endif

H264Encoder::H264Encoder(int32_t width, int32_t height, uint32_t pixelformat)
    : video_width_(width),
      video_height_(height),
      pixelformat_(pixelformat)
{
}

H264Encoder::~H264Encoder()
{
    UnInit();
}

int32_t H264Encoder::X264ParamApplyPreset(x264_param_t *param, const char *preset)
{
    char *end;
    int32_t i = strtol(preset, &end, 10);
    if (*end == 0 && i >= 0 && i < (int32_t)(sizeof(x264_preset_names) / sizeof(*x264_preset_names) - 1))
        preset = x264_preset_names[i];

    if (!strcasecmp(preset, "ultrafast")) {
        param->i_frame_reference    = 1;
        param->i_scenecut_threshold = 0;
        param->b_deblocking_filter  = 0;
        param->b_cabac              = 0;
        param->i_bframe             = 0;
        param->i_keyint_max         = 30; // 30帧里面有一个I帧
        // param->i_keyint_min = 30;
        param->analyse.intra              = 0;
        param->analyse.inter              = 0;
        param->analyse.b_transform_8x8    = 0;
        param->analyse.i_me_method        = X264_ME_DIA;
        param->analyse.i_subpel_refine    = 0;
        param->rc.i_aq_mode               = 0;
        param->analyse.b_mixed_references = 0;
        param->analyse.i_trellis          = 0;
        param->i_bframe_adaptive          = X264_B_ADAPT_NONE;
        param->rc.b_mb_tree               = 0;
        param->analyse.i_weighted_pred    = X264_WEIGHTP_NONE;
        param->analyse.b_weighted_bipred  = 0;
        param->rc.i_lookahead             = 0;
    } else if (!strcasecmp(preset, "superfast")) {
        param->analyse.inter              = X264_ANALYSE_I8x8 | X264_ANALYSE_I4x4;
        param->analyse.i_me_method        = X264_ME_DIA;
        param->analyse.i_subpel_refine    = 1;
        param->i_frame_reference          = 1;
        param->analyse.b_mixed_references = 0;
        param->analyse.i_trellis          = 0;
        param->rc.b_mb_tree               = 0;
        param->analyse.i_weighted_pred    = X264_WEIGHTP_SIMPLE;
        param->rc.i_lookahead             = 0;
    } else if (!strcasecmp(preset, "veryfast")) {
        param->analyse.i_me_method        = X264_ME_HEX;
        param->analyse.i_subpel_refine    = 2;
        param->i_frame_reference          = 1;
        param->analyse.b_mixed_references = 0;
        param->analyse.i_trellis          = 0;
        param->analyse.i_weighted_pred    = X264_WEIGHTP_SIMPLE;
        param->rc.i_lookahead             = 10;
    } else if (!strcasecmp(preset, "faster")) {
        param->analyse.b_mixed_references = 0;
        param->i_frame_reference          = 2;
        param->analyse.i_subpel_refine    = 4;
        param->analyse.i_weighted_pred    = X264_WEIGHTP_SIMPLE;
        param->rc.i_lookahead             = 20;
    } else if (!strcasecmp(preset, "fast")) {
        param->i_frame_reference       = 2;
        param->analyse.i_subpel_refine = 6;
        param->analyse.i_weighted_pred = X264_WEIGHTP_SIMPLE;
        param->rc.i_lookahead          = 30;
    } else if (!strcasecmp(preset, "medium")) {
        /* Default is medium */
    } else if (!strcasecmp(preset, "slow")) {
        param->analyse.i_me_method      = X264_ME_UMH;
        param->analyse.i_subpel_refine  = 8;
        param->i_frame_reference        = 5;
        param->i_bframe_adaptive        = X264_B_ADAPT_TRELLIS;
        param->analyse.i_direct_mv_pred = X264_DIRECT_PRED_AUTO;
        param->rc.i_lookahead           = 50;
    } else if (!strcasecmp(preset, "slower")) {
        param->analyse.i_me_method      = X264_ME_UMH;
        param->analyse.i_subpel_refine  = 9;
        param->i_frame_reference        = 8;
        param->i_bframe_adaptive        = X264_B_ADAPT_TRELLIS;
        param->analyse.i_direct_mv_pred = X264_DIRECT_PRED_AUTO;
        param->analyse.inter |= X264_ANALYSE_PSUB8x8;
        param->analyse.i_trellis = 2;
        param->rc.i_lookahead    = 60;
    } else if (!strcasecmp(preset, "veryslow")) {
        param->analyse.i_me_method     = X264_ME_UMH;
        param->analyse.i_subpel_refine = 10;
    } else {
        return -1;
    }

    return 0;
}

void H264Encoder::Init()
{
    encode_.param   = new (std::nothrow) x264_param_t[sizeof(x264_param_t)];
    encode_.picture = new (std::nothrow) x264_picture_t[sizeof(x264_picture_t)];

    x264_param_default(encode_.param);                // 编码器默认设置
    X264ParamApplyPreset(encode_.param, "ultrafast"); // 订制编码器性能

    encode_.param->i_width  = video_width_;  // 设置图像宽度
    encode_.param->i_height = video_height_; // 设置图像高度

    if ((encode_.handle = x264_encoder_open(encode_.param)) == 0) {
        return;
    }
#if 1
    x264_picture_alloc(encode_.picture, X264_CSP_I420, encode_.param->i_width,
                        encode_.param->i_height);
        encode_.picture->img.i_csp = X264_CSP_I420;
#else
    if (pixelformat_ == V4L2_PIX_FMT_NV12) {
        x264_picture_alloc(encode_.picture, X264_CSP_NV12, encode_.param->i_width,
                           encode_.param->i_height);
        encode_.picture->img.i_csp = X264_CSP_NV12;
    } else if (pixelformat_ == V4L2_PIX_FMT_YUYV) {
        x264_picture_alloc(encode_.picture, X264_CSP_YUYV, encode_.param->i_width,
                        encode_.param->i_height);
        encode_.picture->img.i_csp = X264_CSP_YUYV;
    } else if (pixelformat_ == V4L2_PIX_FMT_YUV420) {
        x264_picture_alloc(encode_.picture, X264_CSP_I420, encode_.param->i_width,
                        encode_.param->i_height);
        encode_.picture->img.i_csp = X264_CSP_I420;
    }else if (pixelformat_ == V4L2_PIX_FMT_RGB24) {
        x264_picture_alloc(encode_.picture, X264_CSP_RGB, encode_.param->i_width,
                        encode_.param->i_height);
        encode_.picture->img.i_csp = X264_CSP_RGB;
    }
#endif
    encode_.picture->img.i_plane = 3;
}

void H264Encoder::UnInit()
{
    if (encode_.picture) {
        x264_picture_clean(encode_.picture);
        delete[] encode_.picture;
        encode_.picture = nullptr;
    }
    if (encode_.param) {
        delete[] encode_.param;
        encode_.param = nullptr;
    }
    if (encode_.handle) {
        x264_encoder_close(encode_.handle);
    }
}

bool H264Encoder::CompressFrame(frametype type, uint8_t *in, uint8_t *out, uint64_t &length)
{
    x264_picture_t pic_out;
    int nNal       = -1;
    int result     = 0;
    uint8_t *p_out = out;

#if 1
    char *y = (char *)(encode_.picture->img.plane[0]);
    char *u = (char *)(encode_.picture->img.plane[1]);
    char *v = (char *)(encode_.picture->img.plane[2]);
    if (pixelformat_ == V4L2_PIX_FMT_NV12) {
        int ySize = video_width_ * video_height_;
        int uvSize = ySize / 4;  // 420采样，UV平面是Y平面的1/4
        // NV12的Y平面直接复制
        memcpy(y, in, ySize);
        
        // 处理UV平面（NV12的UV是交错排列的）
        const uint8_t* uvSrc = in + ySize;
        for (int i = 0; i < uvSize; ++i) {
            u[i] = uvSrc[2 * i];      // U分量
            v[i] = uvSrc[2 * i + 1];   // V分量
        }
    } else if (pixelformat_ == V4L2_PIX_FMT_YUYV) {
        int32_t widthStep422 = encode_.param->i_width * 2;
        uint8_t *p422;
        for (int32_t i = 0; i < encode_.param->i_height; i += 2) {
            p422 = in + i * widthStep422;

            for (int32_t j = 0; j < widthStep422; j += 4) {
                *(y++) = p422[j];
                *(u++) = p422[j + 1];
                *(y++) = p422[j + 2];
            }

            p422 += widthStep422;

            for (int32_t j = 0; j < widthStep422; j += 4) {
                *(y++) = p422[j];
                *(v++) = p422[j + 3];
                *(y++) = p422[j + 2];
            }
        }
    } else if (pixelformat_ == V4L2_PIX_FMT_YUV420) {
        memcpy(y, in, video_width_ * video_height_);
        memcpy(u, in + video_width_ * video_height_, video_width_ * video_height_ / 4);
        memcpy(v, in + video_width_ * video_height_ + video_width_ * video_height_ / 4, video_width_ * video_height_ / 4);
    }
#else
    if (pixelformat_ == V4L2_PIX_FMT_NV12) {
        memcpy(encode_.picture->img.plane[0], in, video_width_ * video_height_); // Y 平面
        memcpy(encode_.picture->img.plane[1], in + video_width_ * video_height_, video_width_ * video_height_ / 2); // UV 平面
    } else if (pixelformat_ == V4L2_PIX_FMT_YUYV) {
        memcpy(encode_.picture->img.plane[0], in, video_width_ * video_height_ * 2); // YUYV 数据大小为宽度 * 高度 * 2
    } else if (pixelformat_ == V4L2_PIX_FMT_YUV420) {
        char *y = (char *)(encode_.picture->img.plane[0]);
        char *u = (char *)(encode_.picture->img.plane[1]);
        char *v = (char *)(encode_.picture->img.plane[2]);
        memcpy(y, in, video_width_ * video_height_);
        memcpy(u, in + video_width_ * video_height_, video_width_ * video_height_ / 4);
        memcpy(v, in + video_width_ * video_height_ + video_width_ * video_height_ / 4, video_width_ * video_height_ / 4);
    } else if (pixelformat_ == V4L2_PIX_FMT_RGB24) {
        memcpy(encode_.picture->img.plane[0], in, video_width_ * video_height_ * 3);
    }
#endif
    switch (type) {
    case FRAME_TYPE_P:
        encode_.picture->i_type = X264_TYPE_P;
        break;
    case FRAME_TYPE_IDR:
        encode_.picture->i_type = X264_TYPE_IDR;
        break;
    case FRAME_TYPE_I:
        encode_.picture->i_type = X264_TYPE_I;
        break;
    default:
        encode_.picture->i_type = X264_TYPE_AUTO;
        break;
    }

    int32_t len = x264_encoder_encode(encode_.handle, &(encode_.nal), &nNal, encode_.picture, &pic_out);
    if (len < 0) {
        printf("Encode fail\n");
        return false;
    }

    encode_.picture->i_pts++;

    for (int32_t i = 0; i < nNal; i++) {
        memcpy(p_out, encode_.nal[i].p_payload, encode_.nal[i].i_payload);
        p_out += encode_.nal[i].i_payload;
        result += encode_.nal[i].i_payload;
    }
    length = result;

    return true;
}
