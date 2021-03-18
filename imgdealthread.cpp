#include "imgdealthread.h"
#include <QtCore>
#include <string>
#include <QApplication>
#include<QRadioButton>
extern vector<QRadioButton*> radioGroup;
imgDealThread::imgDealThread(MyFFmpeg *ffmpeg,QSpinBox *spinBox,QCheckBox *checkBox)
{
    spotP=Point(0,0);
    m_ffmpeg=ffmpeg;
    dealFPS =spinBox;
    dealFlag=checkBox;
}
imgDealThread::~imgDealThread()
{
    this->quit();
    wait();

}
void imgDealThread::run()
{
    while (1)
    {
        static bool runFirst=true;
        if(runFirst)
        {
            detector=new MDetector();
            runFirst=false;
        }
        if(m_ffmpeg->rgbList.size()==0) {msleep(5);continue;}
        static int flag=0;
        ++flag;
        int temp=(int)(m_ffmpeg->m_FPS.num/m_ffmpeg->m_FPS.den/dealFPS->value());
        if(flag%temp==0) flag=0;
        if(flag)
        {
            m_ffmpeg->popRGB();
            continue;
        }
        int height=m_ffmpeg->m_height,width=m_ffmpeg->m_width;
        if(height==0||width==0) continue ;
        Mat &&img=m_ffmpeg->popRGB();
        if(dealFlag->checkState())
            dealImg(img);
        if(img.channels()==4)
        {
            rectangle(img,Rect(spotP.x, spotP.y, img.cols*BLOOD_W, img.rows*BLOOD_H),Scalar(0,255,0,0),2);
//            rectangle(img,Rect(Point(img.cols*(1-WIDTH_SCALE), 0),Point(img.cols - 1, img.cols*HEIGHT_SCALE)),Scalar(0,255,0,0),2);
        }
        if(pushShowPic)
            push_list(img);
    }
}
Mat imgDealThread::pop_list()
{
    Mat ret;
    mtx_matlist.lock();
    if(!matlist.isEmpty())
    {
        ret=matlist[0];
        matlist.pop_front();
    }
    mtx_matlist.unlock();
    return ret;
}
void imgDealThread::push_list(Mat img)
{
    mtx_matlist.lock();
    if(matlist.size()<MAX_MAT_CACHE)
        matlist.push_back(img);
    mtx_matlist.unlock();
}
void imgDealThread::dectDeal()
{
    char key;
    if(radioGroup[0]->isChecked())
        key='1';
    else if(radioGroup[1]->isChecked())
        key='2';
    else if(radioGroup[2]->isChecked())
        key='3';
    else if(radioGroup[3]->isChecked())
        key='4';
    else if(radioGroup[4]->isChecked())
        key='5';
    else if(radioGroup[5]->isChecked())
        key='6';
    else if(radioGroup[6]->isChecked())
        key='7';
    else if(radioGroup[7]->isChecked())
        key='8';
    else if(radioGroup[8]->isChecked())
        key='9';
    keybd_event(key,0,0,0);
    keybd_event(key,0,KEYEVENTF_KEYUP,0);
}
void imgDealThread::dealImg(Mat &img)
{
    static int dealDelay=0;
    Mat bloodMat = img(Rect(spotP.x, spotP.y, img.cols*BLOOD_W, img.rows*BLOOD_H));
//    if (dealDelay==0)
    {
        bool ret=bloodDetect(bloodMat);
        if(ret) dealDelay=45;
    }
    if(dealDelay==0) return;
    else if(dealDelay--%3!=0) return;
    Point cornor1(img.cols*(1-WIDTH_SCALE), 0), cornor2(img.cols - 1, img.cols*HEIGHT_SCALE);
    Mat img2 = img(Rect(cornor1, cornor2));
//    if(img2.channels()==4)
//        cvtColor(img2,img2,CV_BGRA2BGR);
    bool ret=detector->detectGoal(img2);
    if(ret)
    {
        dectDeal();
        emit dectSignal();
        dealDelay=0;
        dealFlag->setChecked(false);
        qDebug()<<"!!!!!!!!!!!!!!";
        static int n=0;
        imwrite(to_string(n++) + ".png", img);
    }
}
void imgDealThread::saveVideo(Mat &img)
{
    if(!saveFlag&&writer==NULL) return;
    else if(saveFlag&&writer==NULL)
    {
        writer=new VideoWriter("VideoTest.avi", CV_FOURCC('M', 'J', 'P', 'G'), 30, Size(img.cols, img.rows), true);
    }
    else if(!saveFlag&&writer!=NULL)
    {
        writer->release();
     //   delete writer;
        writer==NULL;
    }
    else
        *writer<<img;
}
bool imgDealThread::bloodDetect(Mat bloodMat)
{
   // imshow("blood",bloodMat);
//    waitKey(1);
    if (bloodMat.channels() == 4)
        cvtColor(bloodMat, bloodMat, CV_BGRA2BGR);
    Mat mask,mask1,mask2;
    inRange(bloodMat, Scalar(0, 0, 210), Scalar(25, 25, 255),mask1);
    mask=mask1;
    vector<vector<Point>> contours;
    vector<Vec4i> hierarchy;
    findContours(mask, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    for (auto contour : contours)
    {
        Rect rect = boundingRect(contour);
        qDebug()<<rect.height<<bloodMat.rows*0.58;
        qDebug()<<rect.x<<15;
        qDebug()<<rect.y<<bloodMat.rows*0.3;
        if (rect.height >= bloodMat.rows * 0.58 && rect.x <= 15 && rect.y <= bloodMat.rows * 0.4)
        {
            qDebug()<<"!!!!!!!!blood red!";
            return true;
        }
    }
    return false;
}
//bool imgDealThread::bloodDetect(Mat bloodMat)
//{
//   // imshow("blood",bloodMat);
////    waitKey(1);
//    if (bloodMat.channels() == 4)
//        cvtColor(bloodMat, bloodMat, CV_BGRA2BGR);
//    Mat mask,mask1,mask2;
//    inRange(bloodMat, Scalar(0, 0, 210), Scalar(25, 25, 255),mask1);
//    inRange(bloodMat, Scalar(45, 45, 210), Scalar(95, 95, 255),mask2);
//    mask=mask1|mask2;
//    imshow("blood",mask);
//    vector<vector<Point>> contours;
//    vector<Vec4i> hierarchy;
//    findContours(mask, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
//    for (auto contour : contours)
//    {
//        Rect rect = boundingRect(contour);
//        qDebug()<<rect.height<<bloodMat.rows*0.58;
//        qDebug()<<rect.x<<15;
//        qDebug()<<rect.y<<bloodMat.rows*0.3;
//        if (rect.height >= bloodMat.rows * 0.58 && rect.x <= 15 && rect.y <= bloodMat.rows * 0.4)
//        {
//            qDebug()<<"!!!!!!!!blood red!";
//            return true;
//        }
//    }
//    Mat mask3;
//    contours.clear();
//    hierarchy.clear();
//    inRange(bloodMat, Scalar(155, 210, 225), Scalar(192, 240, 255),mask3);
//    findContours(mask3, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
//    for (auto contour : contours)
//    {
//        Rect rect = boundingRect(contour);
//        if (rect.height >= bloodMat.rows * 2 / 3 && rect.x <= 15 && rect.y <= bloodMat.rows * 1 / 3)
//        {
//            qDebug()<<"!!!!!!!!blood red!";
//            return true;
//        }
//    }
//    return false;
//}
void imgDealThread::picDetect(std::string filename)
{
   Mat img=imread(filename);
   Mat bloodMat = img(Rect(img.cols*BLOOD_X,img.rows*BLOOD_Y, img.cols*BLOOD_W, img.rows*BLOOD_H));
   if(bloodDetect(bloodMat)) qDebug()<<"red";
   rectangle(img,Rect(img.cols*BLOOD_X,img.rows*BLOOD_Y, img.cols*BLOOD_W, img.rows*BLOOD_H),Scalar(0,255,0,0),2);
   Point cornor1(img.cols*(1-WIDTH_SCALE), 0), cornor2(img.cols - 1, img.cols*HEIGHT_SCALE);
   Mat img2 = img(Rect(cornor1, cornor2));
   if(img.cols<500||img.rows<500) img2=img;
   if(img2.channels()==4)
     cvtColor(img2,img2,CV_BGRA2BGR);
   bool ret=detector->detectGoal(img2);
   if(ret)
   {
       dealFlag->setChecked(false);
       qDebug()<<"!!!!!!!!!!!!!!";
   }
//   namedWindow("img",0);
//   imshow("img",img2);
   push_list(img);
}
