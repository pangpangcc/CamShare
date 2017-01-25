/*
 * H264Encoder.h
 *
 *  Created on: 2016年12月08日
 *      Author: Alex
 */

#ifndef H264ENCODER_H_
#define H264ENCODER_H_

#include "Rtp2H264VideoTransfer.h"

#include <string>
using namespace std;

struct AVCodec;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;

class H264Encoder {
public:
	H264Encoder();
	virtual ~H264Encoder();

	static void GobalInit();

	bool Create();
	void Destroy();

	bool EncodeFrame(
			const unsigned char* srcData,
			unsigned int dataLen,
			unsigned char* desData,
			int& desDataLen,
			bool& isKeyFrame,
			int width,
			int height
			);

	void SetRecordFile(const string& filePath);

	// 设置采集参数
	void SetCaptureParam(int width, int heigt, int format);
	// 设置采集高宽
	bool SetCaptureVideoSize(int width, int heigt);
	// 设置采集率
	bool SetCaptureVideoFramerate(int rate);
	// 设置发送视频高宽
	bool SetVideoSize(int width, int heigt);

	bool yuv420torgb565(unsigned char* yuv, int width, int heigt, unsigned char* rgb, int& frameLen);


private:
	// 编码器
	AVCodec *mCodec;
	// 编码器上下文
	AVCodecContext *mContext;
	int mGotPicture, mLen;

	// 采集的宽
	int mCaptureWidth;
	// 采集的高
	int mCaptureHeight;
	// 采集格式
	int mCaptureFormat;
	// 采集率
	int mFramerate;

	// 编码的宽
	int mEncodeWidth;
	// 编码的高
	int mEncodeHeight;
	// 编码格式
	int mEncodeId;
	// yuv 中的y（明亮度）的长度，一般是宽*高
	int my_length;
	// yuv （uv是色度）中的长度，没有uv就是黑白了
	int muv_length;

	Rtp2H264VideoTransfer mRtp2H264VideoTransfer;

	string mFilePath;
};

#endif /* H264DECODER_H_ */
