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
#include <atomic>

#include "ringbuffer.h"
#include "epoll.h"

#include "h264_camera.h"
#include "H264_UVC_Cap.h"
#ifdef USE_RK_HW_ENCODER
#include "rk_mpp_encoder.h"
#endif

std::atomic<bool> g_pause;
ip_t g_ip;

#include <execinfo.h>
#include <csignal>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#define PRINT_SIZE_ 100

void Execute(std::string cmdline, std::string &recv)
{
#if defined(__unix__)
    FILE *stream = NULL;
    char buff[1024];
    char recv_buff[256]      = {0};

    memset(recv_buff, 0, sizeof(recv_buff));

    if ((stream = popen(cmdline.c_str(), "r")) != NULL) {
        while (fgets(buff, 1024, stream)) {
            strcat(recv_buff, buff);
        }
    }
    recv = recv_buff;
    pclose(stream);
#endif
}

void SuperExecute(std::string cmdline, std::string passwd)
{
#if defined(__unix__)
    char cmd[256] = {0};
    int len = snprintf(cmd, sizeof(cmd), "echo %s | sudo -S %s", passwd.c_str(), cmdline.c_str());
    cmd[len] = 0;
    system(cmd);
#endif
}

void Addr2Line(std::string exe, std::vector<std::string>& strs)
{
#if defined(__unix__)
    char str[1024] = {0};
    for (uint32_t i = 0; i < strs.size(); i++) {
        std::string line = strs[i];
        std::string::size_type index = line.find("(+"); // found start stuck
        line = line.substr(index + 1, line.size() - index - 1);
        if (index != std::string::npos) {
            index = line.find(")"); // foud end
            if (index != std::string::npos) {
                line = line.substr(0, index);
                int len = snprintf(str, sizeof(str), "addr2line -e %s %s", exe.c_str(), line.c_str());
                str[len] = 0;
                // std::cout << "Run " << str << std::endl;
                std::string recv;
                Execute(str, recv);
                std::ofstream outfile;
                if (recv.find("??") == std::string::npos) {
                    outfile.open("coredump.log", std::ios::out | std::ios::app);
                    if (outfile.is_open()) {
                        outfile << recv;
                        outfile.close();
                    }
                }
            }
        }
    }
#endif
}

const char *g_exe_name;

static void _signal_handler(int signum)
{
    void *array[PRINT_SIZE_];
    char **strings;

    size_t size = backtrace(array, PRINT_SIZE_);
    strings     = backtrace_symbols(array, size);

    if (strings == nullptr) {
        fprintf(stderr, "backtrace_symbols");
        exit(EXIT_FAILURE);
    }

    switch (signum) {
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
#ifdef BACKTRACE_DEBUG
    std::vector<std::string> strs;
    for (size_t i = 0; i < size; i++) {
        fprintf(stderr, "%ld %s \n", i, strings[i]);
        strs.push_back(strings[i]);
    }
    Addr2Line(g_exe_name, strs);
#else
    std::string path = std::string(g_exe_name) + ".log";
    std::ofstream outfile(path, std::ios::out | std::ios::app);
    if (outfile.is_open()) {
        outfile << "Commit ID: " << GIT_VERSION << std::endl;
        outfile << "Git path: " << GIT_PATH << std::endl;
        outfile << "Compile time: " << __TIME__ << " " << __DATE__ << std::endl;
    }
    for (size_t i = 0; i < size; i++) {
        fprintf(stderr, "%ld %s \n", i, strings[i]);
        if (outfile.is_open()) {
            outfile << strings[i] << std::endl;
        }
    }
    if (outfile.is_open()) {
        outfile.close();
    }
#endif
    free(strings);
    MY_EPOLL.EpoolQuit();
    signal(signum, SIG_DFL); /* 还原默认的信号处理handler */
    fprintf(stderr, "Quit execute now\n");
    fflush(stderr);
    exit(-1);
}

const char *rfc822_datetime_format(time_t time, char *datetime)
{
    int32_t r;
    char *date = asctime(gmtime(&time));
    char mon[8], week[8];
    int32_t year, day, hour, min, sec;
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


void rtp_thread(std::string dev)
{
    ip_t* ipaddr = &g_ip;
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
    
    uint32_t rtptime = 0;
    int32_t idr          = 0;

    Rtp rtp; // rtp通讯类
    H264 h264;// 好64通讯类

    spdlog::info("rtp server init.");

#ifdef USE_RK_HW_ENCODER
    std::unique_ptr<VideoFactory> video_stream_factory(new UvcMppCamera);
#else
    std::unique_ptr<VideoFactory> video_stream_factory(new UvcYuyvCamera);
#endif
    std::unique_ptr<VideoStream> h264_video_(video_stream_factory->createVideoStream(dev, 1920, 1080, 5));

    uint32_t fMaxSize = 1843200;
    uint8_t *h264data = new uint8_t[fMaxSize];

    while (g_pause) {
        uint32_t fFrameSize = 0;
        uint32_t fNumTruncatedBytes = 0;
        h264_video_->getData(h264data, fMaxSize, fFrameSize, fNumTruncatedBytes);
        if(fFrameSize <= 0) {
            continue;
        }

        // fFrameSize = RINGBUF.Read(h264data, fMaxSize);
        // if(fFrameSize <= 0) {
        //     continue;
        // }
        h264_nalu_t *nalu = h264.NalPacketMalloc(h264data, fFrameSize);
        
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
                rtp_packet_t *rtp_ptk = rtp.PacketMalloc(h264_nal->data, h264_nal->len);
                rtp_packet_t *cur     = rtp_ptk;
                while (cur) {
                    udp_server.SendMsg(&udp, ipaddr->ip, ipaddr->port, (uint8_t *)cur->data, cur->len);
                    cur = cur->next;
                }
                rtp.PacketFree(rtp_ptk);
            } else if ((h264_nal->type == H264_NAL_SPS || h264_nal->type == H264_NAL_PPS) && !idr) {
                rtp_packet_t *cur = rtp.PacketMalloc(h264_nal->data, h264_nal->len);
                udp_server.SendMsg(&udp, ipaddr->ip, ipaddr->port, (uint8_t *)cur->data, cur->len);
                rtp.PacketFree(cur);
            }
            h264_nal = h264_nal->next;
        }
        h264.NalPacketFree(nalu);
        
    }

    delete[] h264data;
    udp_server.Deinit(&udp);
    udp_server.Deinit(&rtcp);
    spdlog::info("rtp exit");
}

void rtsp_thread(std::string dev)
{
    ip_t *ipaddr = &g_ip;
    tcp_t tcp;
    int32_t client = 0;
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
    std::thread rtp_thread_create;

    while (true) {
        if (client == 0) {
            fd_set fds;
            struct timeval tv;
            int32_t r;

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
            g_pause = true;
            rtp_thread_create = std::thread(rtp_thread, dev);
            rtp_thread_create.detach();
            // if (rtp_thread_create.joinable()) {
            //     rtp_thread_create.join();
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
            spdlog::info("rtsp client {} close", client);
            tcp_server.CloseClient(&tcp, client);
            client  = 0;
            g_pause = false;
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
    g_exe_name = argv[0];
    signal(SIGPIPE, _signal_handler); // SIGPIPE，管道破裂。
    signal(SIGSEGV, _signal_handler); // SIGSEGV，非法内存访问
    signal(SIGFPE, _signal_handler);  // SIGFPE，数学相关的异常，如被0除，浮点溢出，等等
    signal(SIGABRT, _signal_handler); // SIGABRT，由调用abort函数产生，进程非正常退出

    g_pause = false;
    std::string dev = "/dev/video0";
    if(argc > 1) {
        dev = (char *)argv[1];
    }

    spdlog::info("Use commad: rtsp://{}:8554/live", GetHostIpAddress());

    rtsp_thread(dev);

    return 0;
}
