#pragma once

#include <QOpenGLWidget>
#include <opencv.hpp>
#include <QTimer>
#include "MyFFmpeg.h"
#include "imgdealthread.h"
class VideoViewWidget : public QOpenGLWidget
{
    Q_OBJECT

public:
    VideoViewWidget(QWidget *parent);
    ~VideoViewWidget();
//    void setFFmpeg(MyFFmpeg *ffmpeg);
    void setImgTread(imgDealThread *imgThread);
    void deleteFFmpeg();
    void paintEvent(QPaintEvent *e);
    void timerEvent(QTimerEvent *e);
private:
    MyFFmpeg *m_ffmpeg=NULL;
    imgDealThread *m_imgTread=NULL;
    QTimer *m_timer;
    QTimer *m_timer_mat;
private slots:
    void DisplayMat();
    void dealFFmpeg();
};
