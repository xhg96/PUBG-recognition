#include "PlayThread.h"
#include "MyFFmpeg.h"
#include <QtCore>
PlayThread::PlayThread(MyFFmpeg * m_ffmpeg):m_ffmpeg(m_ffmpeg)
{

}

PlayThread::~PlayThread()
{
    this->quit();
    wait();//优雅的关闭线程
}

void PlayThread::run()
{
    while (1)
    {
        static int flag=0;
        if (!(m_ffmpeg->m_isPlay))
        {
            msleep(10);
            continue;
        }
        AVPacket pkt = m_ffmpeg->ReadFrame();
        if (pkt.size <= 0)
        {
            msleep(5);
            continue;
        }
        else
        {
            QTime time;
            time.start();
            m_ffmpeg->DecodeFrame(&pkt);
            av_packet_unref(&pkt);
//            qDebug()<<time.elapsed();
        }
    }
}
