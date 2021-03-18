#ifndef IMGDEALTHREAD_H
#define IMGDEALTHREAD_H
#include <QThread>
#include "MyFFmpeg.h"
#include "mdetector.h"
#include <opencv.hpp>
#include <QList>
#include <QMutex>
#include <QTimer>
#include <QSpinBox>
#include <QCheckBox>
#define MAX_MAT_CACHE 10
#define WIDTH_SCALE 0.24
#define HEIGHT_SCALE 0.2
#define GAP_SCALE 0.05
#define BLOOD_X 0.393
#define BLOOD_Y 0.952
#define BLOOD_H 0.02
#define BLOOD_W 0.0214
using namespace cv;
using namespace std;
class imgDealThread : public QThread
{
    Q_OBJECT
public:
    bool saveFlag=false;
    imgDealThread(MyFFmpeg *ffmpeg,QSpinBox *spinBox,QCheckBox *checkBox);
    void picDetect(std::string filename);
    ~imgDealThread();
    void run();  
    Mat pop_list();
    void push_list(Mat img);
    bool pushShowPic=true;
    void dealImg(Mat &img);
    bool dectectFlag =true;
    Point spotP;
private:
    QTimer timer;
    MyFFmpeg *m_ffmpeg=NULL;
   // uchar *imgbuff;
    QMutex mtx_matlist;
    QList<Mat> matlist;
    void saveVideo(Mat &img);
    bool bloodDetect(Mat bloodMat);
    VideoWriter *writer=NULL;
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
    QSpinBox *dealFPS;
    QCheckBox *dealFlag;
    MDetector *detector;
    void dectDeal();
signals:
    void dectSignal();
};

#endif // IMGDEALTHREAD_H
