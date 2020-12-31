#include <myrtmp.h>

MyRTMP::MyRTMP(const int width, const int height, const int channel, const int fps) {
	m_width = width;
	m_height = height;
	m_channel = channel;
	m_fps = fps;

	m_sc = NULL;
	m_frame = NULL;
	m_acc = NULL;
	m_afc = NULL;
	m_codec = NULL;
	m_stream = NULL;

	m_ret = 0;
	m_vpts = 0;
}

// ��ʼ��
// push_add:������ַ
bool MyRTMP::initRTMP(const char* push_addr) {

	try{
		// ��ʼ����ʽת��������
		if (m_channel == 1) {
			m_sc = sws_getCachedContext(m_sc, m_width, m_height, AV_PIX_FMT_GRAY8, m_width, m_height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, 0, 0, 0);
		}
		else {
			m_sc = sws_getCachedContext(m_sc, m_width, m_height, AV_PIX_FMT_BGRA, m_width, m_height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, 0, 0, 0);
		}
		if (!m_sc){
			throw exception("��ʼ����ʽת��������ʧ��");
			return false;
		}
		// ��ʼ��������ݸ�ʽ
		m_frame = av_frame_alloc();
		m_frame->format = AV_PIX_FMT_YUV420P;
		m_frame->width = m_width;
		m_frame->height = m_height;
		m_frame->pts = 0;

		// ����m_frame�ռ�
		m_ret = av_frame_get_buffer(m_frame, 32);
		if (m_ret != 0) {
			printError(m_ret);
			return false;
		}

		// �ҵ�������
		m_codec = avcodec_find_encoder(AV_CODEC_ID_H264);
		if (!m_codec) {
			throw exception("�Ҳ���������");
			return false;
		}
		// ����������������
		m_acc = avcodec_alloc_context3(m_codec);
		if (!m_acc) {
			throw exception("����������������ʧ��");
			return false;
		}

		// ���ñ���������
		m_acc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
		m_acc->codec_id = m_codec->id;
		m_acc->thread_count = 8;

		AVDictionary* param = NULL;
		av_dict_set(&param, "preset", "superfast", 0);
		av_dict_set(&param, "tune", "zerolatency", 0);

		m_acc->bit_rate = 30 * 1024 * 8;	// ѹ����ÿ֡30kb
		m_acc->width = m_width;
		m_acc->height = m_height;
		m_acc->time_base.num = 1;
		m_acc->time_base.den = m_fps;
		m_acc->framerate.num = m_fps;
		m_acc->framerate.den = 1;

		m_acc->qmin = 10;
		m_acc->qmax = 51;

		m_acc->gop_size = 50;

		m_acc->max_b_frames = 0;
		m_acc->pix_fmt = AV_PIX_FMT_YUV420P;

		m_ret = avcodec_open2(m_acc, m_codec, &param);
		if (m_ret != 0) {
			printError(m_ret);
			return false;
		}

		//���������װ��������
		m_ret = avformat_alloc_output_context2(&m_afc, 0, "flv", NULL);
		if (m_ret != 0){
			printError(m_ret);
			return false;
		}

		//�����Ƶ��
		m_stream = avformat_new_stream(m_afc, NULL);
		if (!m_stream)
		{
			throw exception("������ʧ��");
			return false;
		}

		m_stream->codecpar->codec_tag = 0;
		avcodec_parameters_from_context(m_stream->codecpar, m_acc);
		m_stream->time_base.num = 1;
		m_acc->time_base.den = m_fps;
		av_dump_format(m_afc, 0, push_addr, 1);

		//��rtmp���������io
		m_ret = avio_open(&m_afc->pb, push_addr, AVIO_FLAG_WRITE);
		if (m_ret != 0){
			printError(m_ret);
			return false;

		}

		//д���װͷ
		m_ret = avformat_write_header(m_afc, NULL);
		if (m_ret != 0){
			printError(m_ret);
			return false;
		}
		av_init_packet(&m_pack);

		return true;
	}catch (const std::exception&){
		if (m_sc){
			sws_freeContext(m_sc);
			m_sc = NULL;
		}
		if (m_acc){
			avio_closep(&m_afc->pb);
			avcodec_free_context(&m_acc);
		}
		return false;
	}
}

// ��ʼ����
// data:ͼ������
bool MyRTMP::startRTMP(const uchar* data, int ele_size) {
	//��������ݽṹ
	uint8_t* in_data[AV_NUM_DATA_POINTERS] = { 0 };
	in_data[0] = (uint8_t*)data;
	int in_size[AV_NUM_DATA_POINTERS] = { 0 };

	//һ�������ֽ���
	in_size[0] = m_width * ele_size;
	int h = sws_scale(m_sc, in_data, in_size, 0, m_height, m_frame->data, m_frame->linesize);
	if (h <= 0){
		return false;
	}

	//h264����
	m_frame->pts = m_vpts;
	m_vpts++;

	av_init_packet(&m_pack);
	if (m_acc->codec_type == AVMEDIA_TYPE_VIDEO){
		m_ret = avcodec_send_frame(m_acc, m_frame);
	}
	avcodec_receive_packet(m_acc, &m_pack);

	if (m_pack.size == 0 || m_ret != 0){

		return false;
	}

	//����
	m_pack.pts = av_rescale_q(m_pack.pts, m_acc->time_base, m_stream->time_base);
	m_pack.dts = m_pack.pts;
	m_pack.duration = av_rescale_q(m_pack.duration, m_acc->time_base, m_stream->time_base);

	av_interleaved_write_frame(m_afc, &m_pack);

	return true;

}

// ��ӡ������Ϣ
void MyRTMP::printError(int error_code) {
	char buffer[512] = { 0 };
	av_strerror(error_code, buffer, sizeof(buffer) - 1);
	cout << "error_code:" << buffer << endl;
}

MyRTMP::~MyRTMP() {
	if (m_sc) {
		sws_freeContext(m_sc);
		m_sc = NULL;
	}
	if (m_acc) {
		avio_closep(&m_afc->pb);
		avcodec_free_context(&m_acc);
	}
}