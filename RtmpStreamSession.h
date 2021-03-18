#ifndef RTMPSTREAMSESSION_H
#define RTMPSTREAMSESSION_H


#include <stdint.h>
#include <Windows.h>
#include <string>
#include <QtCore>
using namespace std;
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}


typedef struct _StreamSinkInfo_
{
	BYTE  nType; //0x01--只录制文件，
	char  szOutputFile[256];

}StreamSinkInfo;

//图像、音频包解码后调用的回调函数
typedef LRESULT (CALLBACK* VideoCaptureCB)(AVStream * input_st, enum PixelFormat pix_fmt, AVFrame *pframe, INT64 lTimeStamp, AVCodecContext *codec, int width, int height);
typedef LRESULT (CALLBACK* AudioCaptureCB)(AVStream * input_st, AVFrame *pframe, INT64 lTimeStamp);


class RtmpStreamSession
{
public:
    RtmpStreamSession();
    virtual ~RtmpStreamSession();

    BOOL   StartStream(string url, string  record_file_path); //开始接收RTSP/RTMP流
    void   StopStream();

	void GetVideoSize(long & width, long & height)  //获取视频分辨率
	{
		width  = coded_width;
		height = coded_height;
	}


	BOOL   CheckTimeOut(DWORD dwCurrentTime); //检查超时

	//设置视频图像的回调函数
	void  SetVideoCaptureCB(VideoCaptureCB pFuncCB);

	//设置音频的回调函数
	void  SetAudioCaptureCB(AudioCaptureCB pFuncCB);

private:
	BOOL openInputStream();
    void closeInputStream();

    BOOL openOutputStream();
    void closeOutputStream();

	int  Demuxer(AVStream *pStream, AVPacket & pkt);

	int   DecodeVideo(AVStream * st, AVPacket & dec_pkt);
    bool  DecodeAudio(PBYTE pData, int nDataLen, AVCodecID audioID, BOOL bPlayAudio);

	void InitData();

	void ReleaseCodecs();

    void run();
    void readAndMux();


	static DWORD WINAPI ReadingThrd(void * pParam);

    int  AAC_TO_ADTS(unsigned char * bufIn, int len,  int audioSamprate, unsigned char* pBufOut, const int nBufSize, int* pOutLen);


private:

    std::string m_InputUrl;

	AVFormatContext* m_inputAVFormatCxt;

	AVBitStreamFilterContext* m_bsfcAAC;
	AVBitStreamFilterContext* m_bsfcH264;

	int m_videoStreamIndex;
	int m_audioStreamIndex;

    AVFormatContext* m_outputAVFormatCxt;


    char m_tmpErrString[64];
    bool m_stop_status;

	HANDLE m_hReadThread;
	BOOL   m_bInited;

	int coded_width, coded_height; //视频分辨率
    int   m_nVideoFramesNum; //视频帧号
	int   m_frame_rate; //帧率

	FILE*    m_pfd; //写文件的句柄


	StreamSinkInfo  m_DestSinkInfo;

    DWORD     m_dwStartConnectTime; //开始接收的时间
	DWORD     m_dwLastRecvFrameTime; //上一次收到帧数据的时间，单位：毫秒
	DWORD     m_nMaxRecvTimeOut; //网络接收数据的超时时间，单位：秒
	DWORD     m_nMaxConnectTimeOut; //连接超时，单位：秒

    
	BOOL         m_bVideoDecoderInited; //是否已经初始化解码器
    AVFrame *     m_pframe;

	VideoCaptureCB  m_pVideoCBFunc; //视频数据回调函数指针
	AudioCaptureCB  m_pAudioCBFunc; //音频数据回调函数指针


	//音频解码器变量
	AVCodecContext *m_pAudioCodecCtx;
	AVCodec *m_pAudioCodec;
     AVFrame * m_pAudioFrame;
	int16_t *m_pSamples; //音频解码后的临时缓冲区

	BOOL m_bAudioDecoderInited;

	BYTE *    m_aacADTSBuf;

public:
  
};

#endif // RTMPSTREAMSESSION_H
