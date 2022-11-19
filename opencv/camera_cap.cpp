#include <iostream>
#include <opencv2/opencv.hpp>
#include <time.h>

#include "spdlog/cfg/env.h"  // suppoop rt for loading levels from the environment variable
#include "spdlog/fmt/ostr.h" // support for user defined types
#include "spdlog/spdlog.h"

int main(int argc, char *argv[])
{
    cv::VideoCapture capture(-1);
    capture.set(cv::CAP_PROP_FRAME_WIDTH, 1920);
    capture.set(cv::CAP_PROP_FRAME_HEIGHT, 1080);
    capture.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'));
    capture.set(cv::CAP_PROP_FPS, 30);//帧数

    // capture.set(cv::CAP_PROP_BRIGHTNESS, 1);//亮度 50
    // capture.set(cv::CAP_PROP_CONTRAST, 40);//对比度 50
    // capture.set(cv::CAP_PROP_SATURATION, 50);//饱和度 50
    // capture.set(cv::CAP_PROP_HUE, 50);//色调 0
    // capture.set(cv::CAP_PROP_EXPOSURE, 50);//曝光 -12

    //打印摄像头参数
    spdlog::debug("width = {}", capture.get(cv::CAP_PROP_FRAME_WIDTH));
    spdlog::debug("height = {}", capture.get(cv::CAP_PROP_FRAME_HEIGHT));
    spdlog::debug("fbs = {}", capture.get(cv::CAP_PROP_FPS));
    spdlog::debug("brightness = {}", capture.get(cv::CAP_PROP_BRIGHTNESS));
    spdlog::debug("contrast = {}", capture.get(cv::CAP_PROP_CONTRAST));
    spdlog::debug("saturation = {}", capture.get(cv::CAP_PROP_SATURATION));
    spdlog::debug("hue = {}", capture.get(cv::CAP_PROP_HUE));
    spdlog::debug("exposure = {}", capture.get(cv::CAP_PROP_EXPOSURE));
    while (1) {
        cv::Mat frame;
        capture >> frame;
        cv::imshow("读取视频", frame);
        cv::waitKey(1);
        spdlog::debug("{}", time(NULL));
    }
    return 0;
}
