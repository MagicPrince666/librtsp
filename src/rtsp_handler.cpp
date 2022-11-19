/*
 * @Author: cijliu
 * @Date: 2021-02-01 14:09:37
 * @LastEditTime: 2021-02-26 16:39:45
 */
#include "rtsp_handler.h"
#include "rtsp.h"
#include "rtsp_parse.h"
#include <stdio.h>

RtspParse g_rtsp_parse;

#define EVENT_PARSE(msg)         \
    rtsp_msg_t _rtsp;            \
    g_rtsp_parse.RtspParseMsg(msg, &_rtsp); \
    switch (_rtsp.request.method) {

#define EVENT_END(func) \
    default:            \
        func(_rtsp);    \
        break;          \
        }               \
        return _rtsp;

#define EVENT(m, func, enable) \
    {                          \
    case m: {                  \
        if (enable)            \
            func(_rtsp);       \
        break;                 \
    }                          \
    }

#define RELY_ADD_COND(m, v, msg, args...)   \
    do {                                    \
        if (m == v) {                       \
            char str[2048] = {0};           \
            sprintf(str, ##args);           \
            sprintf(msg, "%s%s", msg, str); \
        }                                   \
    } while (0)

#define RELY_ADD(msg, args...) RELY_ADD_COND(0, 0, msg, ##args)

RtspHandler::RtspHandler() {}

RtspHandler::~RtspHandler() {}

void event_error_handler(rtsp_msg_t rtsp)
{
    printf("undefine rtsp type\n");
}
void event_default_handler(rtsp_msg_t rtsp)
{
    // printf("undefine rtsp type\n");
}

rtsp_msg_t RtspHandler::RtspMsgLoad(const char *msg)
{
    EVENT_PARSE(msg)
    EVENT(OPTIONS, Options, 0);
    EVENT(DESCRIBE, Describe, 0);
    EVENT(SETUP, Setup, 0);
    EVENT(PLAY, Play, 0);
    EVENT(RECORD, Record, 0);
    EVENT(PAUSE, Pause, 0);
    EVENT(TEARDOWN, Teardown, 0);
    EVENT(ANNOUNCE, Announce, 0);
    EVENT(SET_PARAMETER, SetParameter, 0);
    EVENT(GET_PARAMETER, GetParameter, 0);
    EVENT(REDIRECT, Redirect, 0);
    EVENT_END(event_error_handler);
}

int RtspHandler::RtspRelyDumps(rtsp_rely_t rely, char *msg, uint32_t len)
{
    rtsp_method_enum_t method = rely.request.method;
    if (len < 1024)
        return -1;
    memset(msg, 0, len);
    RELY_ADD(msg, "RTSP/1.0 %d %s\r\n", rely.status, rely.desc);
    RELY_ADD(msg, "CSeq: %d\r\n", rely.cseq);
    RELY_ADD(msg, "Date: %s\r\n", rely.datetime);
    RELY_ADD_COND(method, OPTIONS, msg, "Public: %s\r\n", DEFAULT_RTSP_SUPPORT_METHOD);
    RELY_ADD_COND(method, DESCRIBE, msg, "Content-type: application/sdp\r\n");
    RELY_ADD_COND(((method == SETUP) | (method == PLAY) | (method == TEARDOWN)), 1, msg,
                  "Session: %s;timeout=%d\r\n", rely.session, rely.timeout == 0 ? 60 : rely.timeout);
    RELY_ADD_COND(method, SETUP, msg, "Transport: RTP/AVP%s;%scast;client_port=%d-%d;server_port=%d-%d\r\n",
                  rely.tansport.is_tcp ? "/TCP" : "",
                  rely.tansport.is_multicast ? "multi" : "uni",
                  rely.tansport.client_port,
                  rely.tansport.client_port + 1,
                  rely.tansport.server_port,
                  rely.tansport.server_port + 1);
    RELY_ADD_COND(method, PLAY, msg, "Range: npt=0.000-\r\n");
    RELY_ADD_COND(method, PLAY, msg, "RTP-Info: url=rtsp://%s:%d/%s;seq=%d;rtptime=%ld\r\n",
                  rely.request.url.ip,
                  rely.request.url.port,
                  rely.request.url.uri,
                  rely.rtp_seq,
                  (long)rely.rtptime);
    RELY_ADD(msg, "Content-Length: %d\r\n\r\n", rely.sdp_len);
    RELY_ADD_COND(method, DESCRIBE, msg, "%s", rely.sdp);
    // printf("msg:%ld\n%s\n",strlen(msg),msg);
    return 0;
}

rtsp_rely_t RtspHandler::GetRely(rtsp_msg_t rtsp)
{
    rtsp_rely_t rely;
    memset(&rely, 0, sizeof(rtsp_rely_t));
    sprintf(rely.desc, "OK");
    sprintf(rely.session, "%p", &rtsp);
    rely.request = rtsp.request;
    rely.status  = 200;
    rely.cseq    = rtsp.cseq;
    rely.sdp_len = 0;
    memcpy(rely.session, rtsp.session, strlen(rtsp.session));
    rely.tansport = rtsp.tansport;
    return rely;
}
