#ifndef __TIMER_FD_H__
#define __TIMER_FD_H__

#ifdef __APPLE__
#include <dispatch/dispatch.h>
#else
#include <linux/input.h>
#include <sys/timerfd.h>
#endif
#include <iostream>
#include <functional>

class TimerFd
{
private:
#ifdef __APPLE__
    dispatch_source_t timer_;
#else
    int timer_fd_;
#endif
    std::function<void(void)> read_function_; // 接收回调

    /**
     * @brief 定时器回调
     * @return int
     */
    void timeOutCallBack();

public:
    TimerFd(/* args */);
    ~TimerFd();

    /**
     * @brief 初始化定时器
     * @return bool
     */
    bool InitTimer(void);

    /**
     * @brief 启动定时
     */
    void StartTimer(uint32_t seconds, uint32_t nseconds = 0);

    /**
     * @brief 定时取消
     */
    void CancelTimer();

    /**
     * @brief 回调注册
     * @param handler
     */
    void AddCallback(std::function<void(void)> handler);

    /**
     * 清除回调
     */
    void RemoveCallback()
    {
        read_function_ = nullptr;
    }
};

#endif
