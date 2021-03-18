#pragma once
#include <QMutex>
#include <Windows.h>
#include <QList>
#include <opencv.hpp>
extern "C"
{
#include <libavformat/avformat.h>
#include<libswscale/swscale.h>
}
#define MAX_CACHE 80
class MyFFmpeg
{
public:
//    static MyFFmpeg *GetObj()
//    {
//        static MyFFmpeg ff;
//        return &ff;
//    }
    MyFFmpeg();
    virtual ~MyFFmpeg();
//    void popYuvFrame();
    int OpenVideo(const char *path);
    void DecodeFrame(const AVPacket *pkt);
    AVPacket ReadFrame();
    cv::Mat popRGB();
//    bool YuvToRGB(char *out, int outweight, int outheight,bool popFlag=true);
//    bool m_stop_status;
    AVRational m_FPS={0,1};

public:
    bool m_isPlay;
    //AVFrame *m_yuv = NULL; //视频帧数据
    SwsContext *m_cCtx = NULL; //转换器
//    QList<AVFrame *> m_YUVframeList;
    QList<cv::Mat> rgbList;
    DWORD     m_dwStartConnectTime; //开始接收的时间
    DWORD     m_dwLastRecvFrameTime; //上一次收到帧数据的时间，单位：毫秒
    DWORD     m_nMaxRecvTimeOut; //网络接收数据的超时时间，单位：秒
    DWORD     m_nMaxConnectTimeOut; //连接超时，单位：秒

    int m_width, m_height;
    AVPixelFormat m_pixFmt;//记录像素格式
protected:


    AVFormatContext* m_afc = NULL;
    //int m_videoStream = 0;
    QMutex mtx;
//    QMutex mtx_YUVframeList;
    QMutex mtx_rgbList;
};

