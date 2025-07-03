/*
 * @Author: cijliu
 * @Date: 2021-02-05 14:32:30
 * @LastEditTime: 2021-02-26 16:38:36
 */
#ifndef __RTSP_PARSE_H__
#define __RTSP_PARSE_H__

#include "rtsp.h"

#define RTSP_METHOD_PARSE(msg, val) \
    do {                            \
        if (strcmp(opt, #val) == 0) \
            return val;             \
    } while (0)
#define FUNC_CHECK(func, num) \
    do {                      \
        if (num != func)      \
            return -1;        \
    } while (0)

#define RTSP_LINE(msg, key, line)                   \
    do {                                            \
        char *str = strstr(msg, key);               \
        if (str == NULL) {                          \
            return -1;                              \
        }                                           \
        FUNC_CHECK(sscanf(str, "%[^\n]", line), 1); \
    } while (0)

#define RTSP_PARSE(msg, key, num, args...)     \
    do {                                       \
        char line[DEFAULT_STRING_MAX_LEN];     \
        RTSP_LINE(msg, key, line);             \
        FUNC_CHECK(sscanf(line, ##args), num); \
    } while (0)

// const char *method[] = {
//     "OPTIONS",
//     "DESCRIBE",
//     "SETUP",
//     "PLAY",
//     "RECORD",
//     "PAUSE",
//     "TEARDOWN",
//     "ANNOUNCE",
//     "SET_PARAMETER",
//     "GET_PARAMETER",
//     "REDIRECT",
//     "BUTT",
// };

class RtspParse
{
public:
    RtspParse();
    ~RtspParse();
    int RtspParseMsg(const char *msg, rtsp_msg_t *rtsp);

private:
    rtsp_method_enum_t RtspParseMethod(const char *opt);
    int RtspParseSession(const char *msg, char *session);
    int RtspParseCseq(const char *msg, int *cseq);
    int RtspParseTransport(const char *msg, rtsp_transport_t *trans);
    int RtspParseRequest(const char *msg, rtsp_request_t *req);
};

#endif
