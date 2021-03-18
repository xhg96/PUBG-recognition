#pragma once

#include <QThread>
#include "MyFFmpeg.h"
#include <list>
using namespace std;
class PlayThread : public QThread
{
    Q_OBJECT

public:
//    static PlayThread* GetObj()
//    {
//        static PlayThread pt(MyFFmpeg::GetObj());
//        return &pt;
//    }

    ~PlayThread();
    PlayThread(MyFFmpeg * m_ffmpeg);
    void run();
private:
    MyFFmpeg* m_ffmpeg;
//    list<AVPacket> g_videos;



};
