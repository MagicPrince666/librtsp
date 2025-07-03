/*
 * @Author: cijliu
 * @Date: 2021-02-05 15:12:06
 * @LastEditTime: 2021-02-20 14:28:43
 */
#ifndef __RTSP_HANDLER_H__
#define __RTSP_HANDLER_H__

#include "rtsp.h"

class RtspHandler
{
public:
    RtspHandler();
    ~RtspHandler();

    rtsp_msg_t RtspMsgLoad(const char *msg);
    rtsp_rely_t GetRely(rtsp_msg_t rtsp);
    int RtspRelyDumps(rtsp_rely_t rely, char *msg, uint32_t len);

private:
    void Options(rtsp_msg_t rtsp);
    void Describe(rtsp_msg_t rtsp);
    void Setup(rtsp_msg_t rtsp);
    void Play(rtsp_msg_t rtsp);
    void Record(rtsp_msg_t rtsp);
    void Pause(rtsp_msg_t rtsp);
    void Teardown(rtsp_msg_t rtsp);
    void Announce(rtsp_msg_t rtsp);
    void SetParameter(rtsp_msg_t rtsp);
    void GetParameter(rtsp_msg_t rtsp);
    void Redirect(rtsp_msg_t rtsp);
};

#endif
