/**
 * @file h264_camera.h
 * @author 黄李全 (846863428@qq.com)
 * @brief 获取视频流并编码
 * @version 0.1
 * @date 2022-11-18
 * @copyright Copyright (c) {2021} 个人版权所有
 */
#pragma once

#include "video_capture.h"
#include <mutex>
#include <thread>

#define BUF_SIZE 614400
/*C270 YUV 4:2:2 frame size(char)
160*120*2  = 38400
176*144*2  = 50688
320*176*2  = 112640
320*240*2  = 153600
352*288*2  = 202752
432*240*2  = 207360
544*288*2  = 313344
640*360*2  = 460800
640*480*2  = 614400
752*416*2  = 625664
800*448*2  = 716800
800*600*2  = 960000
864*480*2  = 829440
960*544*2  = 1044480
960*720*2  = 1382400
1024*576*2 = 1179648
1184*656*2 = 1553408
*/
//#define FIFO_NAME "/tmp/my_video.h264"
#define CLEAR(x) memset(&(x), 0, sizeof(x))

#define DelayTime 33 * 1000 //(33us*1000=0.05s 30f/s)

class V4l2H264hData
{
public:
    V4l2H264hData(uint64_t size = BUF_SIZE);
    virtual ~V4l2H264hData();

    void Init();

    void VideoCaptureThread();
    void VideoEncodeThread();

    void startCap();
    void stopCap();

    void EmptyBuffer();
    int32_t getData(void *fTo, unsigned fMaxSize, unsigned &fFrameSize, unsigned &fNumTruncatedBytes);

private:
    void setSource(void *_p)
    {
        s_source_ = _p;
    }

    struct cam_data {
        uint8_t *cam_mbuf; /*缓存区数组5242880=5MB//缓存区数组10485760=10MB//缓存区数组1536000=1.46484375MB,10f*/
        int wpos;
        int rpos; /*写与读的位置*/
        // std::cond captureOK; /*线程采集满一个缓冲区时的标志*/
        // std::cond encodeOK;  /*线程编码完一个缓冲区的标志*/
        std::mutex lock; /*互斥锁*/
    };

    V4l2VideoCapture *p_capture_;
    void *s_source_;
    bool s_b_running_;
    bool s_quit_;
    bool empty_buffer_;
    struct cam_data cam_data_buff_[2];
    bool buff_full_flag_[2];

    uint8_t *h264_buf_;
    uint64_t cam_mbuf_size_;

    std::thread video_capture_thread_;
    std::thread video_encode_thread_;
};
