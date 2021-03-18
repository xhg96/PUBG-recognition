#ifndef MDETECTOR_H
#define MDETECTOR_H
#include "yolo_v2_class.hpp"
#include "opencv.hpp"
#define MDEBUG
class MDetector:public Detector
{
public:
    MDetector();
    bool detectGoal(cv::Mat &img);
    bool detectGoal(cv::Mat &img,cv::Rect target);
    bool isRedContext(cv::Mat &mask);
    bool confirm(cv::Mat &img);
    bool enoughWhite(cv::Mat mask);
private:
    int h1 = 167;
    int h2 = 1;
    int hmin = 167;
    int hmin_Max = 180;
    int hmax = 180;
    int hmax_Max = 180;
    //饱和度
    int smin = 50;
    int smin_Max = 255;
    int smax = 100;
    int smax_Max = 255;
    //亮度
    int vmin = 150;
    int vmin_Max = 255;
    int vmax = 255;
    int vmax_Max = 255;
};

#endif // MDETECTOR_H
