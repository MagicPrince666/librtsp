/*
 * @Author: cijliu
 * @Date: 2021-02-04 16:04:16
 * @LastEditTime: 2021-02-26 17:01:59
 */
#include "h264.h"
#include "net.h"
#include "rtp.h"
#include "rtsp_handler.h"
#include "rtsp_parse.h"

#include "spdlog/spdlog.h"
#include "spdlog/cfg/env.h"  // support for loading levels from the environment variable
#include "spdlog/fmt/ostr.h" // support for user defined types

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <netdb.h> /* struct hostent */
#include <arpa/inet.h> /* inet_ntop */
#include <thread>

#include "ringbuffer.h"

#define SOFT_H264 1
#if SOFT_H264
#include "h264encoder.h"
#include "video_capture.h"
#include "h264_camera.h"
#else
#include "H264_UVC_Cap.h"
#endif

typedef struct {
    unsigned char *data;
    int len;
} file_t;

int g_pause = 0;
ip_t g_ip;

#define BACKTRACE_DEBUG 0

#if BACKTRACE_DEBUG
#include <signal.h>
#include <execinfo.h>

#define PRINT_SIZE_ 100

static void _signal_handler(int signum)
{
    void *array[PRINT_SIZE_];
    char **strings;

    size_t size = backtrace(array, PRINT_SIZE_);
    strings = backtrace_symbols(array, size);

    if (strings == nullptr) {
	   fprintf(stderr, "backtrace_symbols");
	   exit(EXIT_FAILURE);
    }

    switch(signum) {
        case SIGSEGV:
        fprintf(stderr, "widebright received SIGSEGV! Stack trace:\n");
        break;

        case SIGPIPE:
        fprintf(stderr, "widebright received SIGPIPE! Stack trace:\n");
        break;

        case SIGFPE:
        fprintf(stderr, "widebright received SIGFPE! Stack trace:\n");
        break;

        case SIGABRT:
        fprintf(stderr, "widebright received SIGABRT! Stack trace:\n");
        break;

        default:
        break;
    }

    for (size_t i = 0; i < size; i++) {
        fprintf(stderr, "%zu %s \n", i, strings[i]);
    }

    free(strings);
    signal(signum, SIG_DFL); /* 还原默认的信号处理handler */

    exit(1);
}
#endif

const char *rfc822_datetime_format(time_t time, char *datetime)
{
    int r;
    char *date = asctime(gmtime(&time));
    char mon[8], week[8];
    int year, day, hour, min, sec;
    sscanf(date, "%s %s %d %d:%d:%d %d", week, mon, &day, &hour, &min, &sec, &year);
    r = sprintf(datetime, "%s, %02d %s %04d %02d:%02d:%02d GMT",
                 week, day, mon, year, hour, min, sec);
    return r > 0 && r < 32 ? datetime : NULL;
}

uint32_t rtsp_get_reltime(void)
{
    struct timespec tp;
    uint64_t ts;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    ts = (tp.tv_sec * 1000000 + tp.tv_nsec / 1000);
    return ts;
}

int open_h264_file(char *filename, file_t *file)
{
    struct stat info;
    stat(filename, &info);
    FILE *fp   = fopen(filename, "rb");
    if (fp == nullptr) {
        spdlog::error("file {} open fail", filename);
        return -1;
    }
    file->data = (unsigned char *)malloc(info.st_size);
    memset(file->data, 0, info.st_size);
    fread(file->data, 1, info.st_size, fp);
    file->len = info.st_size;
    fclose(fp);
    return 0;
}

void close_h264_file(file_t *file)
{
    if (file->data) {
        free(file->data);
        file->data = NULL;
    }
}


void rtp_thread(void *args)
{
    ip_t *ipaddr = &g_ip;
    udp_t udp, rtcp;

    UdpServer udp_server;
    if (udp_server.Init(&udp, 45504)) {
        spdlog::error("udp server init fail.");
        return;
    }
    if (udp_server.Init(&rtcp, 45505)) {
        spdlog::error("udp server init fail.");
        return;
    }
    const char *filename = "../example/test.h264";
    file_t file;
    uint32_t rtptime = 0;
    int idr          = 0;
    rtp_header_t header;
    Rtp rtp;
    rtp.HeaderInit(&header);
    header.seq = 0;
    header.ts  = 0;
    if (open_h264_file((char *)filename, &file) < 0) {
        return;
    }
    h264_nalu_t *nalu = H264FUN.NalPacketMalloc(file.data, file.len);
    spdlog::info("rtp server init.");

    while (g_pause) {
        h264_nalu_t *h264_nal = nalu;
        while (h264_nal && g_pause) {
            if (h264_nal->type == H264_NAL_IDR || h264_nal->type == H264_NAL_PFRAME) {
                if (rtptime == 0) {
                    rtptime = rtsp_get_reltime();
                } else {
                    while ((rtptime + 40000) > rtsp_get_reltime()) {
                        usleep(100);
                    }
                    // printf("sleep:%d us\n", rtsp_get_reltime() - rtptime);
                    rtptime = rtsp_get_reltime();
                }
                idr                   = 1;
                rtp_packet_t *rtp_ptk = rtp.PacketMalloc(&header, h264_nal->data, h264_nal->len);
                rtp_packet_t *cur     = rtp_ptk;
                while (cur) {
                    udp_server.SendMsg(&udp, ipaddr->ip, ipaddr->port, (unsigned char *)cur->data, cur->len);
                    cur = cur->next;
                }
                rtp.PacketFree(rtp_ptk);
            } else if ((h264_nal->type == H264_NAL_SPS || h264_nal->type == H264_NAL_PPS) && !idr) {
                rtp_packet_t *cur = rtp.PacketMalloc(&header, h264_nal->data, h264_nal->len);
                udp_server.SendMsg(&udp, ipaddr->ip, ipaddr->port, (unsigned char *)cur->data, cur->len);
                rtp.PacketFree(cur);
            }
            h264_nal = h264_nal->next;
        }
    }
    H264FUN.NalPacketFree(nalu);
    close_h264_file(&file);
    udp_server.Deinit(&udp);
    udp_server.Deinit(&rtcp);
    spdlog::debug("rtp exit");
}

void rtsp_thread(void *args)
{
#if SOFT_H264
    V4l2H264hData softh264;
    softh264.Init();
#else
    H264UvcCap h264_camera("/dev/video0");
    h264_camera.InitH264Camera();
#endif
    ip_t *ipaddr = (ip_t *)args;
    tcp_t tcp;
    int client = 0;
    RtspHandler rtsp_handler;
    TcpServer tcp_server;
    if (tcp_server.Init(&tcp, 8554)) {
        spdlog::error("tcp server init fail.");
        return;
    }

    char msg[2048];
    char sdp[2048] = "v=0\n"
                     "o=- 16409863082207520751 16409863082207520751 IN IP4 0.0.0.0\n"
                     "c=IN IP4 0.0.0.0\n"
                     "t=0 0\n"
                     "a=range:npt=0-1.4\n"
                     "a=recvonly\n"
                     "m=video 0 RTP/AVP 97\n"
                     "a=rtpmap:97 H264/90000\n";
    std::thread rtp_thread_test;
    while (1) {
        if (client == 0) {
            fd_set fds;
            struct timeval tv;
            int r;

            FD_ZERO(&fds);
            FD_SET(tcp.sock, &fds);
            tv.tv_sec  = 2;
            tv.tv_usec = 0;
            r          = select(tcp.sock + 1, &fds, NULL, NULL, &tv);
            if (-1 == r || 0 == r) {
                continue;
            }
            client = tcp_server.WaitClient(&tcp);
            sprintf(ipaddr->ip, "%s", inet_ntoa(tcp.addr.sin_addr));
            ipaddr->port = ntohs(tcp.addr.sin_port);
            spdlog::info("rtsp client ip:{} port:{}", inet_ntoa(tcp.addr.sin_addr), ntohs(tcp.addr.sin_port));
        }
        char recvbuffer[2048];
        tcp_server.ReceiveMsg(&tcp, client, (uint8_t *)recvbuffer, sizeof(recvbuffer));
        rtsp_msg_t rtsp = rtsp_handler.RtspMsgLoad(recvbuffer);
        char datetime[30];
        rfc822_datetime_format(time(NULL), datetime);
        rtsp_rely_t rely = rtsp_handler.GetRely(rtsp);
        memcpy(rely.datetime, datetime, strlen(datetime));
        switch (rtsp.request.method) {
        case SETUP:
            rely.tansport.server_port = 45504;
            rtsp_handler.RtspRelyDumps(rely, msg, 2048);
            sprintf(g_ip.ip, "%s", ipaddr->ip);
            g_ip.port = rtsp.tansport.client_port;
            g_pause = 1;
            rtp_thread_test = std::thread(rtp_thread, &g_ip);
            rtp_thread_test.detach();
            // if (rtp_thread_test.joinable()) {
            //     rtp_thread_test.join();
            // }
            spdlog::info("rtp client ip:{} port:{}", g_ip.ip, g_ip.port);
            break;
        case DESCRIBE:
            rely.sdp_len = strlen(sdp);
            memcpy(rely.sdp, sdp, rely.sdp_len);
            rtsp_handler.RtspRelyDumps(rely, msg, 2048);
            break;
        case TEARDOWN:
            rtsp_handler.RtspRelyDumps(rely, msg, 2048);
            tcp_server.SendMsg(&tcp, client, msg, strlen(msg));
            tcp_server.CloseClient(&tcp, client);
            client  = 0;
            g_pause = 0;
            continue;
        default:
            rtsp_handler.RtspRelyDumps(rely, msg, 2048);
            break;
        }
        tcp_server.SendMsg(&tcp, client, msg, strlen(msg));
        usleep(1000);
    }
    tcp_server.Deinit(&tcp);
}

std::string GetHostIpAddress() {
    std::string Ip;
	char name[256];
	gethostname(name, sizeof(name));

	struct hostent* host = gethostbyname(name);
	char ipStr[32];
	const char* ret = inet_ntop(host->h_addrtype, host->h_addr_list[0], ipStr, sizeof(ipStr));
	if (nullptr == ret) {
		spdlog::error("hostname transform to ip failed");
		return "";
	}
	Ip = ipStr;
	return Ip;
}

int main(int argc, char *argv[])
{
#if BACKTRACE_DEBUG
    signal(SIGPIPE, _signal_handler);  // SIGPIPE，管道破裂。
    signal(SIGSEGV, _signal_handler);  // SIGSEGV，非法内存访问
    signal(SIGFPE, _signal_handler);  // SIGFPE，数学相关的异常，如被0除，浮点溢出，等等
    signal(SIGABRT, _signal_handler);  // SIGABRT，由调用abort函数产生，进程非正常退出
#endif
    spdlog::info("Use commad: rtsp://{}:8554/live", GetHostIpAddress());

    std::thread rtsp_thread_test(rtsp_thread, &g_ip);
    if (rtsp_thread_test.joinable()) {
        rtsp_thread_test.join();
    }

    while (1) {
        sleep(1);
    }
}
