#pragma once

#include "common.h"


extern "C"{
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/mathematics.h>
#include <libavutil/opt.h>
#include <libavutil/time.h>
#include <libavutil/timestamp.h>
#include <libswscale/swscale.h>
}

#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avformat.lib")

typedef unsigned char uchar;

class MyRTMP {
private:
	//���ظ�ʽת��������
	SwsContext* m_sc;
	//������ݽṹ
	AVFrame* m_frame;
	//������������
	AVCodecContext* m_acc;
	//rtmp flv ��װ��
	AVFormatContext* m_afc;

	AVPacket m_pack;
	//���������
	AVCodec* m_codec;
	//������Ƶ��
	AVStream* m_stream;

	// ֡��
	int m_fps;

	// ͼ����ߣ�ͨ����
	int m_width;
	int m_height;
	int m_channel;

	//״̬��ʶ��
	int m_ret ;
	int m_vpts;

public:
	MyRTMP(const int width, const int height, const int channel, const int fps=24);

	// ��ʼ��
	// push_add:������ַ
	bool initRTMP(const char* push_add);

	// ��ʼ����
	// data:ͼ������
	bool startRTMP(const uchar* data, int ele_size);

	// ��ӡ������Ϣ
	void printError(int error_code);

	~MyRTMP();
};