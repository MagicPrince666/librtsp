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
#include <list>

// #define BUF_SIZE 614400
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

#define CLEAR(x) memset(&(x), 0, sizeof(x))

class V4l2H264hData
{
public:
    V4l2H264hData(std::string dev = "/dev/video0");
    virtual ~V4l2H264hData();

    /**
     * @brief 初始化资源
     */
    void Init();

    /**
     * @brief 开启h264编码线程
     */
    void VideoEncodeThread();

    /**
     * @brief 暂停线程
     * @param pause 是否暂停
     * @return true 暂停
     * @return false 继续
     */
    bool PauseCap(bool pause);

    /**
     * @brief 停止
     */
    void stopCap();

    /**
     * @brief 给live555用
     * @param fTo
     * @param fMaxSize
     * @param fFrameSize
     * @param fNumTruncatedBytes
     * @return int32_t
     */
    int32_t getData(void *fTo, unsigned fMaxSize, unsigned &fFrameSize, unsigned &fNumTruncatedBytes);

private:
    /**
     * @brief 打开文件
     */
    void InitFile();

    /**
     * 关闭文件
     */
    void CloseFile();

    /**
     * 获取并编码
     */
    void RecordAndEncode();

private:
    struct Buffer {
        uint8_t *buf_ptr;
        uint64_t length;
    };

    V4l2VideoCapture *p_capture_;
    H264Encoder *encoder_;
    bool s_b_running_;
    bool s_pause_;

    uint8_t *camera_buf_;
    uint8_t *h264_buf_;

    std::string v4l2_device_;
    std::string h264_file_name_;
    FILE *h264_fp_;

    std::thread video_encode_thread_;
    std::mutex cam_data_lock_; /*互斥锁*/
    std::list<Buffer> h264_buf_list_;
};

inline bool FileExists(const std::string& name);
