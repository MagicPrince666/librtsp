#include <unistd.h>
#include "timer_fd.h"
#include "epoll.h"

TimerFd::TimerFd(/* args */)
{
}

TimerFd::~TimerFd()
{
    // 清理资源
#ifdef __APPLE__
    dispatch_source_cancel(timer_);
    dispatch_release(timer_);
#else
    if (timer_fd_ > 0) {
        MY_EPOLL.EpollDel(timer_fd_);
        close(timer_fd_);
    }
#endif
}

bool TimerFd::InitTimer(void)
{
#ifndef __APPLE__
    // 创建1s定时器fd
    if ((timer_fd_ =
             timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC)) < 0) {
        std::cout << "create timer fd fail" << std::endl;
        return false;
    }
    // 绑定回调函数
    MY_EPOLL.EpollAddRead(timer_fd_, std::bind(&TimerFd::timeOutCallBack, this));
#else
    // 创建一个 dispatch queue
    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);

    // 创建一个定时器
    timer_ = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, queue);

    // 设置定时器的回调函数
    dispatch_source_set_event_handler(timer_, ^{timeOutCallBack();});
#endif
    return true;
}

void TimerFd::AddCallback(std::function<void(void)> handler)
{
    read_function_ = handler;
}

void TimerFd::StartTimer(uint32_t seconds, uint32_t nseconds)
{
#ifdef __APPLE__
    // 设置定时器的触发时间和间隔
    dispatch_source_set_timer(timer_, dispatch_time(DISPATCH_TIME_NOW, 0), seconds * NSEC_PER_SEC + nseconds, 0);

    // 启动定时器
    dispatch_resume(timer_);
#else
    // 设置30s定时器
    struct itimerspec time_intv;
    time_intv.it_value.tv_sec     = seconds; // 设定超时s
    time_intv.it_value.tv_nsec    = nseconds;
    time_intv.it_interval.tv_sec  = time_intv.it_value.tv_sec; // 间隔超时
    time_intv.it_interval.tv_nsec = time_intv.it_value.tv_nsec;
    // 启动定时器
    timerfd_settime(timer_fd_, 0, &time_intv, NULL);
#endif
}

void TimerFd::CancelTimer()
{
    // 取消定时
#ifdef __APPLE__
    dispatch_source_cancel(timer_);
#else
    struct itimerspec time_intv;
    time_intv.it_value.tv_sec     = 0;
    time_intv.it_value.tv_nsec    = 0;
    time_intv.it_interval.tv_sec  = 0; 
    time_intv.it_interval.tv_nsec = 0;
    timerfd_settime(timer_fd_, 0, &time_intv, NULL);
#endif
}

void TimerFd::timeOutCallBack()
{
#ifndef __APPLE__
    uint64_t value;
    read(timer_fd_, &value, sizeof(uint64_t));
#endif
    if (read_function_) {
        read_function_();
    }
}
