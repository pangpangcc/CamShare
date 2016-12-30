/*
 * H264Decoder.h
 *
 *  Created on: 2016年10月28日
 *      Author: max
 */

#ifndef H264DECODER_H_
#define H264DECODER_H_

#include "Rtp2H264VideoTransfer.h"

#include <string>
using namespace std;

struct AVCodec;
struct AVCodecContext;
struct AVFrame;

class H264Decoder {
public:
	H264Decoder();
	virtual ~H264Decoder();

	static void GobalInit();

	bool Create();
	void Destroy();

	bool DecodeFrame(
			const char* srcData,
			unsigned int dataLen,
			char* frame,
			int& frameLen,
			int& width,
			int& height
			);

	void SetRecordFile(const string& filePath);

private:
	AVCodec *mCodec;
	AVCodecContext *mContext;


	int mGotPicture, mLen;

	Rtp2H264VideoTransfer mRtp2H264VideoTransfer;

	string mFilePath;
};

#endif /* H264DECODER_H_ */
