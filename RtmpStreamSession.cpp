#include "RtmpStreamSession.h"
#include <sstream>


#define ONE_AUDIO_FRAME_SIZE 192000

//string to_string(int n)
//{
//	std::ostringstream stm;
//	string str;
//	stm << n;
//	str = stm.str();
//	//std::cout << str << std::endl;
//	return str;
//}



//////////////////////////////////////////////////////////////

RtmpStreamSession::RtmpStreamSession()
{
    m_stop_status = false;

    m_videoStreamIndex	= -1;
	m_audioStreamIndex	= -1;
	m_hReadThread		= nullptr;
	m_bInited			= FALSE;

	m_bsfcH264		= nullptr;
    m_bsfcAAC       = nullptr;

	coded_width = coded_height = 0;
    m_frame_rate = 25;
	m_pfd = nullptr;

	m_inputAVFormatCxt = nullptr;
	m_nVideoFramesNum = 0;

	m_dwStartConnectTime = 0;
	m_dwLastRecvFrameTime = 0;
	m_nMaxConnectTimeOut = 8;
	m_nMaxRecvTimeOut = 10;

	m_bAudioDecoderInited = FALSE;
	m_bVideoDecoderInited = FALSE;
	m_pframe = NULL;
	m_pVideoCBFunc = NULL;
	m_pAudioCBFunc = NULL;

	m_pAudioCodecCtx = NULL;
	m_pAudioCodec = NULL;
	m_pSamples = NULL; 
	m_pAudioFrame = NULL;

}

RtmpStreamSession::~RtmpStreamSession()
{
	ReleaseCodecs();
}


void RtmpStreamSession::StopStream()
{
    m_stop_status = true;

	if (m_hReadThread != NULL) 
	{
        WaitForSingleObject(m_hReadThread, INFINITE);
		CloseHandle(m_hReadThread);
		m_hReadThread = NULL;
	}

    closeInputStream();
}


void RtmpStreamSession::InitData()
{
	coded_width = coded_height = 0;

	m_nVideoFramesNum = 0;
    m_bInited = FALSE;
	m_stop_status = false;
	m_bAudioDecoderInited = FALSE;
	m_bVideoDecoderInited = FALSE;
}



//开始接收RTSP/RTMP流
//url -- URL路径
//record_file_path -- 录制的文件路径
//
BOOL   RtmpStreamSession::StartStream(string url,  string  record_file_path)
{
	m_InputUrl   = url;

    m_videoStreamIndex = -1;
	m_audioStreamIndex = -1;
	m_bInited = FALSE;

	if(!record_file_path.empty())
	{
		m_DestSinkInfo.nType = 0x11;
		strcpy(m_DestSinkInfo.szOutputFile, record_file_path.c_str());
	}
	else
	{
		m_DestSinkInfo.nType = 0x10;
		memset(m_DestSinkInfo.szOutputFile, 0, sizeof(m_DestSinkInfo.szOutputFile));
	}


	InitData();

	do
	{
		   
		//if(!openInputStream())
		//{
		//	break;
		//}

		//if(!openOutputStream())
		//{
		//	break;
		//}

   		DWORD threadID = 0;
		m_hReadThread = CreateThread(NULL, 0, ReadingThrd, this, 0, &threadID);

		return TRUE;

	}while(0);

	closeInputStream();
	closeOutputStream();
	ReleaseCodecs();


	return FALSE;
}


DWORD WINAPI RtmpStreamSession::ReadingThrd(void * pParam)
{
	RtmpStreamSession * pTask = (RtmpStreamSession *) pParam;

	pTask->run();

 //   OutputDebugString("ReadingThrd exited\n");

	return 0;
}

void RtmpStreamSession::run()
{
   
    do
    {
	    m_stop_status = false;

	    if(!openInputStream())
		{
			break;
		}

		if(!openOutputStream())
		{
			break;
		}

        readAndMux();
       
    }while(0);


    closeInputStream();
	closeOutputStream();
    
	 m_stop_status = true;

}

static int interruptCallBack(void *ctx)
{
    RtmpStreamSession * pSession = (RtmpStreamSession*) ctx;

   //once your preferred time is out you can return 1 and exit from the loop
    if(pSession->CheckTimeOut(GetTickCount()))
    {
      return 1;
    }

   //continue 
   return 0;

}

BOOL   RtmpStreamSession::CheckTimeOut(DWORD dwCurrentTime)
{
	if(dwCurrentTime < m_dwLastRecvFrameTime) //CPU时间回滚
	{
		return FALSE;
	}

	if(m_stop_status)
		return TRUE;

	if(m_dwLastRecvFrameTime > 0)
	{
		if((dwCurrentTime - m_dwLastRecvFrameTime)/1000 > m_nMaxRecvTimeOut) //接收过程中超时
		{
		    return TRUE;
		}
	}
	else
	{
		if((dwCurrentTime - m_dwStartConnectTime)/1000 > m_nMaxConnectTimeOut) //连接超时
		{
			return TRUE;
		}
	}
	return FALSE;
}

BOOL RtmpStreamSession::openInputStream()
{
    if (m_inputAVFormatCxt)
    {
        qDebug()<<("already has input avformat \n");
		return FALSE;
    }


    int res = 0;

    bool bIsNetPath = false;

	if(_strnicmp(m_InputUrl.c_str(), "rtsp://", 7) == 0 || _strnicmp(m_InputUrl.c_str(), "RTSP://", 7) == 0)
	{
		bIsNetPath = true;
	}
	else if(_strnicmp(m_InputUrl.c_str(), "rtmp://", 7) == 0 || _strnicmp(m_InputUrl.c_str(), "RTMP://", 7) == 0)
	{
		bIsNetPath = true;
	}
	else
	{
		bIsNetPath = false;
	}

	if(bIsNetPath) //从网络接收
	{
		//Initialize format context
		m_inputAVFormatCxt = avformat_alloc_context();

		//Initialize intrrupt callback
		AVIOInterruptCB icb = {interruptCallBack,this};
		m_inputAVFormatCxt->interrupt_callback = icb;
	}


	m_dwLastRecvFrameTime = 0;
	m_dwStartConnectTime = GetTickCount();

	//m_inputAVFormatCxt->flags |= AVFMT_FLAG_NONBLOCK;

    AVDictionary* options = nullptr;   
    //av_dict_set(&options, "rtsp_transport", "tcp", 0); 
    av_dict_set(&options, "stimeout", "3000000", 0);  //设置超时断开连接时间  

	//m_inputAVFormatCxt->max_analyze_duration = 2000000;
    // m_inputAVFormatCxt->fps_probe_size = 30;

	DWORD dwTick1 = GetTickCount();
	DWORD dwTick2 = 0;

    res = avformat_open_input(&m_inputAVFormatCxt, m_InputUrl.c_str(), 0, &options);
    
	dwTick2 = GetTickCount();

    qDebug()<<("avformat_open_input takes time: %ld ms \n", dwTick2 - dwTick1);

    if(res < 0)
    {
        string strError = "can not open file:" + m_InputUrl + ",errcode:" + to_string(res) + ",err msg:" + av_make_error_string(m_tmpErrString, AV_ERROR_MAX_STRING_SIZE, res);
        qDebug()<<("%s \n", strError.c_str());
		return FALSE;
    }

    qDebug()<<("%d, %d, %d \n", m_inputAVFormatCxt->max_analyze_duration, m_inputAVFormatCxt->probesize, m_inputAVFormatCxt->fps_probe_size);

    if (avformat_find_stream_info(m_inputAVFormatCxt, 0) < 0)
    {
        qDebug()<<("can not find stream info \n");
		return FALSE;
    }
    av_dump_format(m_inputAVFormatCxt, 0, m_InputUrl.c_str(), 0);
    for (int i = 0; i < m_inputAVFormatCxt->nb_streams; i++)
    {
        AVStream *in_stream = m_inputAVFormatCxt->streams[i];

		if (in_stream->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			m_videoStreamIndex = i;

			coded_width = in_stream->codec->width;
			coded_height = in_stream->codec->height;

			if(in_stream->avg_frame_rate.den != 0 && in_stream->avg_frame_rate.num != 0)
			{
			  m_frame_rate = in_stream->avg_frame_rate.num/in_stream->avg_frame_rate.den;//每秒多少帧 
			}

            qDebug()<<("video stream index: %d, width: %d, height: %d, FrameRate: %d\n", m_videoStreamIndex, in_stream->codec->width, in_stream->codec->height, m_frame_rate);
		}
		else if (in_stream->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			m_audioStreamIndex = i;
		}
    }

	 //m_bsfcAAC = av_bitstream_filter_init("aac_adtstoasc");
  //  if(!m_bsfcAAC)
  //  {
  //      qDebug()<<("can not create aac_adtstoasc filter \n");
  //  }

	m_bsfcH264 = av_bitstream_filter_init("h264_mp4toannexb");
    if(!m_bsfcH264)
    {
        qDebug()<<("can not create h264_mp4toannexb filter \n");
    }

	return TRUE;
}

void RtmpStreamSession::closeInputStream()
{
    if (m_inputAVFormatCxt)
    {
        avformat_close_input(&m_inputAVFormatCxt);
		m_inputAVFormatCxt = NULL;
    }
    //if(m_bsfcAAC)
    //{
    //    av_bitstream_filter_close(m_bsfcAAC);
    //    m_bsfcAAC = nullptr;
    //}
	if(m_bsfcH264)
	{
	    av_bitstream_filter_close(m_bsfcH264);
        m_bsfcH264 = nullptr;
	}

}


BOOL RtmpStreamSession::openOutputStream()
{
	if(m_DestSinkInfo.nType & 0x01)
	{
		m_pfd = fopen(m_DestSinkInfo.szOutputFile, "wb");
	}

	return TRUE;
}
   
void RtmpStreamSession::closeOutputStream()
{

	if (m_pfd != nullptr)
	{
		fclose(m_pfd);
		m_pfd = nullptr;
	}
}


void RtmpStreamSession::ReleaseCodecs()
{

	if (m_pfd != nullptr)
	{
		fclose(m_pfd);
		m_pfd = nullptr;
	}

	if(m_pframe)
	{
		av_frame_free(&m_pframe);
		m_pframe = NULL;
	}

	m_bVideoDecoderInited = FALSE;

	if(m_pAudioCodecCtx != NULL)
	{
		avcodec_close(m_pAudioCodecCtx);
		av_free(m_pAudioCodecCtx);

        m_pAudioCodecCtx = NULL;
		m_pAudioCodec = NULL;
	}

	if(m_pAudioFrame != NULL)
	{
		av_free(m_pAudioFrame);
		m_pAudioFrame = NULL;
	}

	if(m_pSamples != NULL)
	{
		av_free(m_pSamples);
		m_pSamples = NULL;
	}


	if(m_aacADTSBuf != NULL)
	{
		delete m_aacADTSBuf;
		m_aacADTSBuf = NULL;
	}


	 m_stop_status = true;

}


void RtmpStreamSession::readAndMux()
{
	int nVideoFramesNum = 0;
	int64_t  first_pts_time = 0;

	DWORD start_time = GetTickCount(); 

    AVPacket pkt;
	av_init_packet(&pkt);

    while(1)
    {
        if(m_stop_status == true)
        {
            break;
        }

        int res;
       
        res = av_read_frame(m_inputAVFormatCxt, &pkt);
        if (res < 0)  //读取错误或读到了文件尾
        {
			if(AVERROR_EOF == res)
			{
                qDebug()<<("End of file \n");

				break;
			}
			else
			{
                qDebug()<<("av_read_frame() got error: %d, pkt.size = %d \n", res, pkt.size);
			  
			  //string strError = av_make_error_string(m_tmpErrString, AV_ERROR_MAX_STRING_SIZE, res);
              //qDebug()<<("ErrMsg: %s \n", strError.c_str());

			  if(res == -11)
			  {
				 Sleep(10);
			     continue;
			  }

			  break;
			}

			//break;  
        }

		AVStream *in_stream = m_inputAVFormatCxt->streams[pkt.stream_index];

  //      if(pkt.stream_index == m_audioStreamIndex && in_stream->codec->codec_id != AV_CODEC_ID_AMR_NB)
		//{
		//	continue;
		//}

        Demuxer(in_stream, pkt);
		av_free_packet(&pkt);

		m_dwLastRecvFrameTime = GetTickCount();

    }//while


	ReleaseCodecs();
}


int RtmpStreamSession::Demuxer(AVStream *pStream, AVPacket & pkt)
{
	//if(avcodec_parameters_to_context(m_pCodecCtx, pStream->codecpar) < 0)
	//	return 0;

	if(pStream->codec->codec_type != AVMEDIA_TYPE_VIDEO 
		&& pStream->codec->codec_type != AVMEDIA_TYPE_AUDIO)
	{
		return 0;
	}

	if(pkt.pts < 0)
		pkt.pts = 0;

	int64_t pts_time = (pkt.pts)*90000*pStream->time_base.num/pStream->time_base.den; //转成90KHz为单位

	if(pStream->codec->codec_type == AVMEDIA_TYPE_VIDEO)  //视频
	{
		if(pStream->codec->codec_id == AV_CODEC_ID_H264)
		{
			m_nVideoFramesNum++;

			if(!(pkt.data[0] == 0x0 && pkt.data[1] == 0x0 && pkt.data[2] == 0x0 && pkt.data[3] == 0x01))
			{
                //qDebug()<<("Not H264 StartCode!\n");

				AVPacket tempPack; 
				av_init_packet(&tempPack);
				//av_copy_packet(&tempPack, &pkt); 

				int nRet = av_bitstream_filter_filter(m_bsfcH264, pStream->codec, NULL, &tempPack.data, &tempPack.size, pkt.data, pkt.size, 0);

				if(nRet >= 0)
				{
					if(m_pfd != NULL) //保存视频数据
					{
						fwrite( tempPack.data, tempPack.size, 1, m_pfd);		
					}


					DecodeVideo(pStream, tempPack);

					if(tempPack.data != NULL)
					{
						av_free(tempPack.data); //一定要加上这句，否则会有内存泄漏
						tempPack.data = NULL;
					}

				}
                else
				{
                    qDebug()<<("av_bitstream_filter_filter got error: %d \n", nRet);
				}

                //qDebug()<<("FrameNo: %d,  size: %d \n", m_nVideoFramesNum,  pkt.size);
			}
			else
			{

				if(m_pfd != NULL) //保存视频数据
				{
					fwrite( pkt.data, pkt.size, 1, m_pfd);		
				}

				DecodeVideo(pStream, pkt);
			}


		}

		//int nSecs = pkt.pts*in_stream->time_base.num/in_stream->time_base.den;
        //qDebug()<<("Frame time: %02d:%02d \n", nSecs/60, nSecs%60);


	}
	else if(pStream->codec->codec_type == AVMEDIA_TYPE_AUDIO) //音频
	{

		AVPacket tempPack; 
		av_init_packet(&tempPack);

		if(pStream->codec->codec_id == AV_CODEC_ID_AAC)
		{
			int nOutAACLen = 0;

			AAC_TO_ADTS(pkt.data, pkt.size, pStream->codec->sample_rate, m_aacADTSBuf, ONE_AUDIO_FRAME_SIZE, &nOutAACLen);

			tempPack.data = m_aacADTSBuf;
			tempPack.size = nOutAACLen;
			tempPack.pts = pkt.pts;
		}
		else
		{
			av_copy_packet(&tempPack, &pkt);
		}

#if 0
		
		if (!m_bAudioDecoderInited)
		{
			if (avcodec_open2(pStream->codec, avcodec_find_decoder(pStream->codec->codec_id), NULL) < 0)
			{
                qDebug()<<("Could not open audio codec.（无法打开解码器）\n");
				return -10;
			}

			m_bAudioDecoderInited = TRUE;
		}

		int encode_audio = 1;
		int dec_got_frame_a = 0;
		int ret;

		AVFrame *input_frame = av_frame_alloc();
		if (!input_frame)
		{
			ret = AVERROR(ENOMEM);
			return ret;
		}
		//解码为PCM音频
		if ((ret = avcodec_decode_audio4(pStream->codec, input_frame, &dec_got_frame_a, &tempPack)) < 0)
		{
            qDebug()<<("Could not decode audio frame\n");
			av_frame_free(&input_frame);

			return ret;
		}

		if (dec_got_frame_a)
		{


		}

		av_frame_free(&input_frame);
#else
		DecodeAudio(tempPack.data, tempPack.size, pStream->codec->codec_id, TRUE);
#endif

	}

	return 0;
}

 
int RtmpStreamSession::AAC_TO_ADTS(unsigned char * bufIn, int len,  int audioSamprate, unsigned char* pBufOut, const int nBufSize, int* pOutLen)
{
    //qDebug()<<("AAC Header: %02x%02x%02x%02x \n", bufIn[0], bufIn[1], bufIn[2], bufIn[3]);

    unsigned char ADTS[] = {0xFF, 0xF9, 0x00, 0x00, 0x00, 0x00, 0xFC}; 
    int audioChannel = 2;//音频声道 1或2
    int audioBit = 16;//16位 固定
    //unsigned int framelen;
	const int header = 7; // header = sizeof(ADTS);
   
	unsigned char * pBufSrc = bufIn;
    unsigned char * pBufDest = pBufOut;

	*pOutLen = 0;

    switch(audioSamprate) //音频采样率
    {
	case  8000:
	    ADTS[2] = 0x6C;
		break;
    case  16000:
        ADTS[2] = 0x60;
        break;
    case  32000:
        ADTS[2] = 0x54;//0xD4
        break;
    case  44100:
        ADTS[2] = 0x50;
        break;
    case  48000:
        ADTS[2] = 0x4C;
        break;
    case  96000:
        ADTS[2] = 0x40;
        break;
	case 11025:
		ADTS[2] = 0x68;
		break;
	case 12000:
		ADTS[2] = 0xE4;
		break;
	case 22050:
		ADTS[2] = 0xDC;
		break;
    default:
        break;
    }
    ADTS[3] = (audioChannel==2)?0x80:0x40;

#if 0
	{
		unsigned short au_sizes_array[32] = {0};
		unsigned int framelen = 0;
	
		int au_size = pBufSrc[1]>>3;
		int au_num = au_size>>1;
        Q_ASSERT(au_num < 32);

		pBufSrc+=2;

		for(int i=0; i<au_num; i++)
		{
		  framelen =  ((unsigned int)pBufSrc[0] << 5) & 0x1FE0;
		  framelen |= ((pBufSrc[1] >> 3) & 0x1f);
          au_sizes_array[i] = framelen;

		  pBufSrc += 2;
		}

        qDebug()<<("Au_Num = %d \n", au_num);

		for(int i=0; i<au_num; i++)
		{
			framelen = au_sizes_array[i];

			ADTS[3] |= ((framelen+header) & 0x1800) >> 11;
			ADTS[4] = ((framelen+header) & 0x1FF8) >> 3;
			ADTS[5] = ((framelen+header) & 0x0007) << 5;
			ADTS[5] |= 0x1F;

			if (*pOutLen + framelen + header > nBufSize)
				break;

			memcpy(pBufDest, ADTS, sizeof(ADTS));
			memcpy(pBufDest+header, pBufSrc, framelen);


			pBufDest += framelen+header;
			*pOutLen += framelen+header;
			pBufSrc += framelen;
		}

	}
#else
	unsigned int framelen = 0;
	{

		framelen = len;

		ADTS[3] |= ((framelen+header) & 0x1800) >> 11;
		ADTS[4] = ((framelen+header) & 0x1FF8) >> 3;
		ADTS[5] = ((framelen+header) & 0x0007) << 5;
		ADTS[5] |= 0x1F;

		memcpy(pBufDest, ADTS, sizeof(ADTS));
		memcpy(pBufDest+header, pBufSrc, framelen);
	
		pBufDest += framelen+header;
		*pOutLen += framelen+header;
	}
#endif


    return 1;
}

void  RtmpStreamSession::SetVideoCaptureCB(VideoCaptureCB pFuncCB)
{
	m_pVideoCBFunc = pFuncCB;
}
	
void  RtmpStreamSession::SetAudioCaptureCB(AudioCaptureCB pFuncCB)
{
	m_pAudioCBFunc = pFuncCB;
}

int RtmpStreamSession::DecodeVideo(AVStream * st, AVPacket & dec_pkt)
{
	if(!m_bVideoDecoderInited)
	{
		if (avcodec_open2(st->codec, avcodec_find_decoder(st->codec->codec_id), NULL) < 0)
		{
            qDebug()<<("Could not open video codec.（无法打开解码器）\n");
			return -1;
		}

		if(m_pframe == NULL)
		{
			m_pframe = av_frame_alloc();
		}

		if (!m_pframe) 
		{
			//ret = AVERROR(ENOMEM);
			return -3;
		}

		m_bVideoDecoderInited = TRUE;
	}

	int dec_got_frame = 0;
	int ret = avcodec_decode_video2(st->codec, m_pframe, &dec_got_frame, &dec_pkt);
	if (ret < 0) 
	{
        qDebug()<<("Decoding failed------------\n");
		return -4;
	}
	if (dec_got_frame)
	{
        if(m_pVideoCBFunc)
		{
            m_pVideoCBFunc(st, (PixelFormat)(st->codec->pix_fmt), m_pframe, /*av_gettime() - m_start_time*/ dec_pkt.pts, st->codec, st->codec->width, st->codec->height);
		}

	}

	return 0;
}


bool RtmpStreamSession:: DecodeAudio(PBYTE pData, int nDataLen, AVCodecID audioID,  BOOL bPlayAudio)
{

	if ( m_pAudioCodecCtx == NULL )
	{			
		//avcodec_register_all();
		m_pAudioCodecCtx = avcodec_alloc_context3(NULL);
		if ( m_pAudioCodec == NULL )
		{
			m_pAudioCodec = avcodec_find_decoder(audioID);
			if ( !m_pAudioCodec )
			{
//				OutputDebugString(_T("Error: avcodec_find_decoder() failed, Can not find this Audio m_pVideoCodec!!!! \n"));
                Q_ASSERT(0);
				av_free(m_pAudioCodecCtx);
				m_pAudioCodecCtx = NULL;
				return false;
			}
		}
		

		if ( avcodec_open2(m_pAudioCodecCtx, m_pAudioCodec, NULL) != 0 )
		{
//			OutputDebugString(_T("Error: avcodec_open() failed! \n"));
			av_free(m_pAudioCodecCtx);
			m_pAudioCodec = NULL;
			m_pAudioCodecCtx = NULL;
			return false;	
		}
	}

	if( m_pSamples == NULL)
	{
		m_pSamples = (short*)av_mallocz(ONE_AUDIO_FRAME_SIZE);  //存放一帧音频的缓冲区
	}

	int nLenOut = ONE_AUDIO_FRAME_SIZE; //单帧音频的最大长度

	AVPacket pkt;
	av_init_packet(&pkt);
	pkt.data = pData;
	pkt.size = nDataLen;	

	if (m_pAudioFrame == NULL )
        m_pAudioFrame =  av_frame_alloc();

	//if ( !m_fp )
	//{
	//	m_fp = fopen("D:\\tmp1.mp3", "wb+");
	//}
    //Q_ASSERT( m_fp!= NULL );
	//fwrite(pkt.data, 1, pkt.size, m_fp);

	int nDecodeTimes = 0; 
	int nTotalBytes = 0;
	int len = 0;

	while(pkt.size > 0)
	{
		nLenOut = ONE_AUDIO_FRAME_SIZE;

		nDecodeTimes++;

#if 0
		memset(m_pSamples, 0, nLenOut);
		len = avcodec_decode_audio3(m_pAudioCodecCtx, m_pSamples, &nLenOut, &pkt);
		if ( len <= 0 )
		{
            qDebug()<<("decoder audio failed \r\n");
			return false;
		}
#else
		int nGot = 0;
		len = avcodec_decode_audio4(m_pAudioCodecCtx, m_pAudioFrame, &nGot, &pkt);
		if (len <= 0 || !nGot)
		{
			OutputDebugStringA("decoder audio failed\n");
			return false;
		}
#endif

        Q_ASSERT(nGot > 0);

		memset(m_pSamples, 0, m_pAudioFrame->nb_samples*sizeof(int16_t));

		//Note: m_pAudioFrame->nb_samples 是一个声道的Sample个数

        if(m_pAudioCodecCtx->channels == 1) //单声道
		{
			switch(m_pAudioCodecCtx->sample_fmt)
			{
			case AV_SAMPLE_FMT_FLT:
			case AV_SAMPLE_FMT_FLTP:
				{
					float * sample_buffer1 = (float*)m_pAudioFrame->data[0];

                    //Q_ASSERT(m_pAudioFrame->linesize[0] == m_pAudioFrame->nb_samples*sizeof(float));

					float sample = 0.0f;
					for(int i=0; i< m_pAudioFrame->nb_samples; i++)
					{
						sample = *sample_buffer1++;
						if(sample < -1.0f)
							sample = -1.0f;
						else if(sample > 1.0f)
							sample = 1.0f;

						m_pSamples[i]  = (int16_t)(sample*32767.0f);
					}
					nLenOut = m_pAudioFrame->nb_samples*sizeof(int16_t);
				}
				break;
			case AV_SAMPLE_FMT_S16:
			case AV_SAMPLE_FMT_S16P:
				{
				    int16_t * sample_buffer1 = (int16_t*)m_pAudioFrame->data[0];

                    Q_ASSERT(m_pAudioFrame->linesize[0] == m_pAudioFrame->nb_samples*sizeof(int16_t));
	
		   			for(int i=0; i< m_pAudioFrame->nb_samples; i++)
					{
						m_pSamples[i]   = sample_buffer1[i];
					}

					nLenOut = m_pAudioFrame->nb_samples*sizeof(int16_t);
				}
				break;
			}
		}
		else if(m_pAudioCodecCtx->channels == 2) //双声道
		{
			switch(m_pAudioCodecCtx->sample_fmt)
			{
			case AV_SAMPLE_FMT_FLT:
				{
                    Q_ASSERT(m_pAudioFrame->nb_samples*sizeof(float)*2 == m_pAudioFrame->linesize[0]);

					float * sample_buffer1 = (float*)m_pAudioFrame->data[0];
					float sample = 0.0f;
					for(int i=0; i< m_pAudioFrame->nb_samples*2; i++)
					{
						sample = *sample_buffer1++;
						if(sample < -1.0f)
							sample = -1.0f;
						else if(sample > 1.0f)
							sample = 1.0f;

						m_pSamples[i]  = (int16_t)(sample*32767.0f);
					}
					nLenOut = m_pAudioFrame->nb_samples*sizeof(int16_t)*2;
				}
				break;
			case AV_SAMPLE_FMT_FLTP: //平面结构
				{
					float * sample_buffer1 = (float*)m_pAudioFrame->data[0];
					float * sample_buffer2 = (float*)m_pAudioFrame->data[1];
	
                    Q_ASSERT(m_pAudioFrame->nb_samples*sizeof(float)*2 == m_pAudioFrame->linesize[0] + m_pAudioFrame->linesize[1]);

					float sample1 = 0.0f;
					float sample2 = 0.0f;

					for(int i=0; i<  m_pAudioFrame->nb_samples; i++)
					{
						sample1 = *sample_buffer1++;
						if(sample1 < -1.0f)
							sample1 = -1.0f;
						else if(sample1 > 1.0f)
							sample1 = 1.0f;

						sample2 = *sample_buffer2++;
						if(sample2 < -1.0f)
							sample2 = -1.0f;
						else if(sample2 > 1.0f)
							sample2 = 1.0f;

						m_pSamples[i*2]    = (int16_t)((sample1)*32767.0f);
						m_pSamples[i*2+1]  = (int16_t)((sample2)*32767.0f);
					}

					nLenOut = m_pAudioFrame->nb_samples*sizeof(int16_t)*2;
				}
				break;
			case AV_SAMPLE_FMT_S16:
				{
					int16_t * sample_buffer1 = (int16_t*)m_pAudioFrame->data[0];

                    Q_ASSERT( m_pAudioFrame->nb_samples*sizeof(int16_t)*2 == m_pAudioFrame->linesize[0]);

					for(int i=0; i< m_pAudioFrame->nb_samples*2; i++)
					{
						m_pSamples[i]     = sample_buffer1[i];
					}
					nLenOut = m_pAudioFrame->linesize[0];
				}
				break;
			case AV_SAMPLE_FMT_S16P:  //平面结构
				{
					int16_t * sample_buffer1 = (int16_t*)m_pAudioFrame->data[0];
					int16_t * sample_buffer2 = (int16_t*)m_pAudioFrame->data[1];

                    Q_ASSERT(m_pAudioFrame->linesize[0] *2 ==  m_pAudioFrame->nb_samples*sizeof(int16_t)*2);

		   			for(int i=0; i< m_pAudioFrame->nb_samples; i++) 
					{
						m_pSamples[i*2]   = sample_buffer1[i];
						m_pSamples[i*2+1] = sample_buffer2[i];

					}
					nLenOut = m_pAudioFrame->nb_samples*sizeof(int16_t)*2;
				}
				break;
			}

		}


		pkt.data += len;
		pkt.size -= len;


		if(bPlayAudio)
		{

			//PlayPCM((BYTE*)m_pSamples, nLenOut); //读者自己实现播放PCM音频

		}


		nTotalBytes += nLenOut;

	}//while

	return nTotalBytes > 0;
}


