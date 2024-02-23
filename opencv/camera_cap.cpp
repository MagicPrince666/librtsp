#include <iostream>
#include <opencv2/opencv.hpp>
#include <time.h>

#include "spdlog/spdlog.h"

int main(int argc, char *argv[])
{
    cv::VideoCapture capture(0);
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
    spdlog::info("width = {}", capture.get(cv::CAP_PROP_FRAME_WIDTH));
    spdlog::info("height = {}", capture.get(cv::CAP_PROP_FRAME_HEIGHT));
    spdlog::info("fbs = {}", capture.get(cv::CAP_PROP_FPS));
    spdlog::info("brightness = {}", capture.get(cv::CAP_PROP_BRIGHTNESS));
    spdlog::info("contrast = {}", capture.get(cv::CAP_PROP_CONTRAST));
    spdlog::info("saturation = {}", capture.get(cv::CAP_PROP_SATURATION));
    spdlog::info("hue = {}", capture.get(cv::CAP_PROP_HUE));
    spdlog::info("exposure = {}", capture.get(cv::CAP_PROP_EXPOSURE));
    while (1) {
        cv::Mat frame;
        capture >> frame;
        cv::imshow("read frame", frame);
        cv::waitKey(1);
        spdlog::info("{}", time(NULL));
    }
    return 0;
}
