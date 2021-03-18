#include<QDebug>
#include "mdetector.h"
using namespace cv;
using namespace std;

#ifdef MDEBUG
#define CFG_FILE R"(C:\Users\xhg\Desktop\classfication\darknet\build\darknet\x64\cfg\myolov3-tiny_pubg2.cfg)"
#define WEIGHT_FILE R"(C:\Users\xhg\Desktop\classfication\darknet\build\darknet\x64\pubg2\myolov3-tiny_pubg2_final.weights)"
#else
#define CFG_FILE "det/list.f"
#define WEIGHT_FILE "det/pu.w"
#endif
MDetector::MDetector():
    Detector(CFG_FILE,WEIGHT_FILE)
{

}
#include <QTime>
bool MDetector::detectGoal(cv::Mat &img)
{
    bool result= false;
    std::vector<bbox_t> boxes=detect(img,0.74);
    for(bbox_t box:boxes)
    {
        Rect rect(box.x,box.y,box.w,box.h);

        qDebug()<<"ID:"<<box.obj_id<<" prob"<<box.prob;
        rect.y-=box.h/3;
        rect.height+=box.h;
        Mat roi=img(rect);
        if(box.obj_id==0&&confirm(roi))
            result=true;
#ifdef MDEBUG
        rectangle(img,rect,Scalar(255,255,255),1);
#endif
    }
    if(result==true)
        return true;
    else
        return false;
}
bool MDetector::confirm(cv::Mat &img)
{
    Mat roi = img.clone();
    if(roi.channels()==4)
        cvtColor(roi,roi,CV_BGRA2BGR);

    cvtColor(roi, roi, CV_BGR2HSV);
    Mat maskC, maskR, mask1, mask2;
    Scalar min(h1, smin, vmin);
    Scalar max(180, smax, vmax);
    inRange(roi, min, max, mask1);
    min = Scalar(0, smin, vmin);
    max = Scalar(h2, smax, vmax);
    inRange(roi, min, max, mask2);
    maskC = mask1 | mask2;
//    imshow("maskc", maskC);
//    bool ret=isRedContext(maskC);
    bool ret=enoughWhite(maskC);
    qDebug()<<ret;
    if(ret )
        return true;
    else
        return false;
}
bool MDetector::detectGoal(cv::Mat &img,cv::Rect target)
{

//    QTime time;
//    time.start();
    std::vector<bbox_t> boxes=detect(img,0.95);
//    qDebug()<<time.elapsed();
    for(bbox_t box:boxes)
    {
        if(box.obj_id>0) continue;
        qDebug()<<"ID:"<<box.obj_id<<" prob"<<box.prob;
        Rect rect(box.x,box.y,box.w,box.h);
        rectangle(img,rect,Scalar(255,255,255),3);
        if(target.contains(rect.br())&&target.contains(rect.tl()))
            return true;
    }
    return false;
}
bool MDetector::isRedContext(Mat &mask)
{
#define STEP 18
#define W 256
    static unsigned int imgNum = 0;
    ++imgNum;
    Rect rect(mask.cols - 30 - W, 48, W, 2 * STEP);
    int num = 0;
    while (rect.y + rect.height <= mask.rows - 32)
    {
        int whiteNum = 0;
        for (int i = rect.y; i < rect.y + rect.height; ++i)
        {
            uchar* p = mask.ptr<uchar>(i);
            for (int j = rect.x; j < rect.x + rect.width; ++j)
            {
                if (p[j]) ++whiteNum;
            }
        }
        if (whiteNum >150)
        {
            return true;
        }
        rect.y += STEP;
    }
    return false;
}
bool MDetector::enoughWhite(cv::Mat mask)
{
    int num = 0;
    for(int i=0;i<mask.rows;++i)
    {
        uchar* p = mask.ptr<uchar>(i);
        for (int j=0;j<mask.cols;++j)
        {
            if (p[j]) ++num;
        }
    }

    int add=(mask.cols-140)/2;
    if(add<0) add=0;
    else if(add>150) add=150 ;
    qDebug()<<num<<">"<<100+add;
    if(num>100+add) return true;
    else return false;
}
