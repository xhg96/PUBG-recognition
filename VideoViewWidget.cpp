#include "VideoViewWidget.h"
#include "MyFFmpeg.h"
#include "PlayThread.h"
#include <QPainter>
#include <iostream>
#include <list>
#include <QtCore>
using namespace std;

extern "C"
{
    #include <libavformat/avformat.h>
    #include<libswscale/swscale.h>
}

static list<AVPacket> g_videos;


VideoViewWidget::VideoViewWidget(QWidget *parent)
    : QOpenGLWidget(parent)
{
    startTimer(20);
   m_timer=new QTimer();
   m_timer->setTimerType(Qt::PreciseTimer);
   connect(m_timer, SIGNAL(timeout()), this, SLOT(dealFFmpeg()));
   m_timer_mat=new QTimer();
   m_timer_mat->setTimerType(Qt::PreciseTimer);
   connect(m_timer_mat, SIGNAL(timeout()), this, SLOT(DisplayMat()));
}

VideoViewWidget::~VideoViewWidget()
{
    delete m_timer;
    delete m_timer_mat;
}

void VideoViewWidget::paintEvent(QPaintEvent *e)
{
//    static QImage *image;
//    static int start,end;
//    if (image == NULL)
//    {
//        uchar *buf = new uchar[width() * height() * 4];
//        image = new QImage(buf, width(), height(), QImage::Format_ARGB32);
//    }
//    if(m_ffmpeg!=NULL)
//    {
//        bool ret = m_ffmpeg->YuvToRGB((char *)(image->bits()), width(), height());
//        if(ret==false) return;
//        start=GetTickCount();
//        QPainter painter;
//        painter.begin(this);
//        painter.drawImage(QPoint(0, 0), *image);
//        painter.end();
//        end=GetTickCount();


//       // qDebug()<<"size:"<<m_ffmpeg->m_YUVframeList.size()<<"time:"<<end-start;

//    }
}
void VideoViewWidget::timerEvent(QTimerEvent *e)
{
    this->update();
}

void VideoViewWidget::setImgTread(imgDealThread *imgThread)
{
    m_ffmpeg=NULL;
    m_imgTread=imgThread;
    m_timer_mat->start(33);
    m_timer->stop();
}
void VideoViewWidget::deleteFFmpeg()
{
    m_timer->stop();
    m_ffmpeg=NULL;
}
void VideoViewWidget::dealFFmpeg()//处理程序一定要和paintEvent分开
{
//    static QImage *image;
// //  static int64 start,end;
//    if (image == NULL)
//    {
//        uchar *buf = new uchar[width() * height() * 4];
//        image = new QImage(buf, width(), height(), QImage::Format_ARGB32);
//    }
//    if(m_ffmpeg!=NULL)
//    {
//        bool ret = m_ffmpeg->YuvToRGB((char *)(image->bits()), width(), height(),false);
//        if(ret==false) return;

//        QPainter painter;
//        painter.begin(this);
//        painter.drawImage(QPoint(0, 0), *image);
//        painter.end();
//    }
////    this->update();
//  //  end=GetTickCount();
//    //qDebug()<<(end-start);
//  //  start=GetTickCount();

}
void VideoViewWidget::DisplayMat()
{
    cv::Mat rgb;
    QImage img;
    if(m_imgTread==NULL) return;
    Mat image=m_imgTread->pop_list();
    if(image.empty()) return ;
    if(image.channels() == 4)
    {
        cvtColor(image,rgb,CV_BGR2RGB);
        img = QImage((const unsigned char*)(rgb.data),
                     rgb.cols,rgb.rows,rgb.cols*rgb.channels(),//rgb.cols*rgb.channels()可以替换为image.step
                     QImage::Format_RGB888);
    }
    else if(image.channels() == 3)
    {
        cvtColor(image,rgb,CV_BGR2RGB);
        img = QImage((const unsigned char*)(rgb.data),
                     rgb.cols,rgb.rows,rgb.cols*rgb.channels(),//rgb.cols*rgb.channels()可以替换为image.step
                     QImage::Format_RGB888);
    }
    else
    {
        img = QImage((const unsigned char*)(image.data),
                     image.cols,image.rows,image.cols*image.channels(),
                     QImage::Format_Grayscale8);
    }
    QImage showImg=img.scaled(size());
    QPainter painter;
    painter.begin(this);
    painter.drawImage(QPoint(0, 0), showImg);
    painter.end();
   // this->update();
}
