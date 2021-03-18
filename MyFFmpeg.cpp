#include "MyFFmpeg.h"
#include <QDebug>
#include <QDateTime>
#include <QtCore>
#include <iostream>
#include <QMessageBox>
#include <QMutexLocker>
using namespace cv;
MyFFmpeg::MyFFmpeg()
{
    m_dwStartConnectTime = 0;
    m_dwLastRecvFrameTime = 0;
    m_nMaxConnectTimeOut = 8;
    m_nMaxRecvTimeOut = 10;

    m_width=0;
    m_height=0;
    m_isPlay = false;
}


MyFFmpeg::~MyFFmpeg()
{
    mtx.lock();
//    mtx_YUVframeList.lock();
    m_isPlay =false;
//    while(!m_YUVframeList.isEmpty())
//    {
//         av_frame_free(&(m_YUVframeList.back()));
//         m_YUVframeList.pop_back();
//    }
    sws_freeContext(m_cCtx);
    avformat_free_context(m_afc);
//    mtx_YUVframeList.unlock();
    mtx.unlock();
}

static int interruptCallBack(void *ctx)
{
   return 0;
}
int MyFFmpeg::OpenVideo(const char *path)
{
//    QDateTime current_date_time =QDateTime::currentDateTime();
//    QString current_date =current_date_time.toString("yyyy.MM.dd hh:mm:ss.zzz ddd");
//    QString mytime=current_date.mid(5,2)+current_date.mid(8,2);
//    if(mytime.toInt()>=510)
//    {
//        QMessageBox::about(NULL, "ERROR", QString::fromLocal8Bit("训练模型文件丢失或损坏!"));
//        return 1;
//    }
    mtx.lock();
    if(strncmp(path,"rtmp://",7)==0)
    {
        m_afc=avformat_alloc_context();
        AVIOInterruptCB icb = {interruptCallBack,this};
        m_afc->interrupt_callback = icb;
    }
    int nRet = avformat_open_input(&m_afc, path, 0, 0);
    if(nRet!=0)
    {
        qDebug()<<"Open Failed:"+QString(path);
        mtx.unlock();
        return 1;
    }
    else{
        if( avformat_find_stream_info(m_afc, 0) <0 )
        {
            qDebug()<<"can not find stream info";
            mtx.unlock();
            return 1;
        }
        else
            qDebug()<<"streams num:"<<m_afc->nb_streams;
    }
    for (int i = 0; i < m_afc->nb_streams; i++)  //nb_streams打开的视频文件中流的数量，一般nb_streams = 2，音频流和视频流
    {
        AVCodecContext *acc = m_afc->streams[i]->codec; //分别获取音频流和视频流的解码器

        if (acc->codec_type == AVMEDIA_TYPE_VIDEO)   //如果是视频
        {
            qDebug()<<"FPS:"<<(float)(m_afc->streams[i]->avg_frame_rate.num)/m_afc->streams[i]->avg_frame_rate.den;
            m_FPS=m_afc->streams[i]->avg_frame_rate;
            //m_videoStream = i;
            m_height=m_afc->streams[i]->codec->height;
            m_width=m_afc->streams[i]->codec->width;
            m_pixFmt=m_afc->streams[i]->codec->pix_fmt;
            qDebug()<<m_width<<"*"<<m_height;
            AVCodec *codec = avcodec_find_decoder(acc->codec_id);   // 解码器

            //"没有该类型的解码器"
            if (!codec)
            {
                qDebug()<<"decoder not found!";
                mtx.unlock();
                return 1;
            }
            int err = avcodec_open2(acc, codec, NULL); //打开解码器
            if (err != 0)
            {
                qDebug()<<"decoder open failed";
            }
            m_cCtx = sws_getCachedContext(m_cCtx, m_width, m_height,
                m_pixFmt,  //像素点的格式
                m_width, m_height,  //目标宽度与高度
                AV_PIX_FMT_BGRA,  //输出的格式
                SWS_BICUBIC,  //算法标记
                NULL, NULL, NULL
                );//时间开销很小
            if (!m_cCtx)
                qDebug()<<"getCachedContext failed";
            m_isPlay = true;
            qDebug()<<"decode succeed";
            mtx.unlock();
            return 0;
        }
    }
    qDebug()<<"video stream not found:"+QString(path);
    mtx.unlock();
    return 1;
}

void MyFFmpeg::DecodeFrame(const AVPacket *pkt)
{
    mtx.lock();
    if (!m_afc)
    {
        mtx.unlock();
        return;
    }

    AVFrame *frame = av_frame_alloc();

    int re = avcodec_send_packet(m_afc->streams[pkt->stream_index]->codec, pkt);
    if (re != 0)
    {
        mtx.unlock();
        return;
    }

    re = avcodec_receive_frame(m_afc->streams[pkt->stream_index]->codec, frame);
    if (re != 0)
    {
        //失败
        av_frame_free(&frame);
        mtx.unlock();
        return;
    }
    else
    {
        Mat img(m_height,m_width,CV_8UC4);
        uint8_t *data[AV_NUM_DATA_POINTERS] = { 0 };
        data[0] = img.data;  //指针传值，形参的值会被改变，out的值一直在变，所以QImage每次的画面都不一样，画面就这样显示出来了，这应该是整个开发过程最难的点
        int linesize[AV_NUM_DATA_POINTERS] = { 0 };
        linesize[0] = m_width * 4;  //每一行转码的宽度
        int h = sws_scale(m_cCtx, frame->data,frame->linesize, 0, m_height,data,linesize);
        av_frame_free(&frame);
        QMutexLocker lock(&mtx_rgbList);
        if(rgbList.size()>MAX_CACHE)
            rgbList.pop_front();
        rgbList.push_back(img);
//        mtx_YUVframeList.lock();
//        if(m_YUVframeList.size()<MAX_CACHE)
//            m_YUVframeList.push_back(frame);
//        else
//        {
//            av_frame_free(&(m_YUVframeList.first()));
//            m_YUVframeList.pop_front();
//            m_YUVframeList.push_back(frame);
//        }
//        mtx_YUVframeList.unlock();
    }
    mtx.unlock();
}
cv::Mat MyFFmpeg::popRGB()
{
    cv::Mat ret;
    mtx_rgbList.lock();
    if(!rgbList.empty())
    {
        ret=rgbList.first();
        rgbList.pop_front();
    }
    mtx_rgbList.unlock();
    return ret;
}
AVPacket MyFFmpeg::ReadFrame()
{
    AVPacket pkt;
    memset(&pkt, 0, sizeof(AVPacket));

    mtx.lock();
    if (!m_afc)
    {
        mtx.unlock();
        return pkt;
    }

    int err = av_read_frame(m_afc, &pkt);
    if (err != 0)
    {
        //失败
    }
    mtx.unlock();

    return  pkt;
}
//void MyFFmpeg::popYuvFrame()
//{
//     mtx_YUVframeList.lock();
//     if(m_YUVframeList.isEmpty()) return ;
//     av_frame_free(&(m_YUVframeList.first()));
//     m_YUVframeList.pop_front();
//     mtx_YUVframeList.unlock();
//}
//bool MyFFmpeg::YuvToRGB(char *out, int outweight, int outheight,bool popFlag)
//{
//    mtx_YUVframeList.lock();
//    if ( m_YUVframeList.isEmpty()) //像素转换的前提是视频已经打开
//    {
//        mtx_YUVframeList.unlock();
//		return false;
//	}
////    if(m_YUVframeList.size()==1&&popFlag)
////    {
////        mtx_YUVframeList.unlock();
////        return false;
////    }
//    m_cCtx = sws_getCachedContext(m_cCtx, m_width, m_height,
//        m_pixFmt,  //像素点的格式
//		outweight, outheight,  //目标宽度与高度
//		AV_PIX_FMT_BGRA,  //输出的格式
//		SWS_BICUBIC,  //算法标记
//		NULL, NULL, NULL
//        );//时间开销很小

//    if (!m_cCtx) return false;

//	uint8_t *data[AV_NUM_DATA_POINTERS] = { 0 };
//	data[0] = (uint8_t *)out;  //指针传值，形参的值会被改变，out的值一直在变，所以QImage每次的画面都不一样，画面就这样显示出来了，这应该是整个开发过程最难的点
//	int linesize[AV_NUM_DATA_POINTERS] = { 0 };
//	linesize[0] = outweight * 4;  //每一行转码的宽度
////    qDebug()<<
//    int h = sws_scale(m_cCtx, m_YUVframeList.first()->data,m_YUVframeList.first()->linesize, 0, m_height,data,linesize);
//    if(popFlag)
//    {
//        av_frame_free(&(m_YUVframeList.first()));
//        m_YUVframeList.pop_front();
//    }
//    mtx_YUVframeList.unlock();
//    return true;
//}
