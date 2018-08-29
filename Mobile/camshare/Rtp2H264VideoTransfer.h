/*
 * Rtp2H264VideoTransfer.h
 *
 *  Created on: 2016年9月27日
 *      Author: max
 */

#ifndef SRC_MOD_ENDPOINTS_MOD_WS_RTP2H264VIDEOTRANSFER_H_
#define SRC_MOD_ENDPOINTS_MOD_WS_RTP2H264VIDEOTRANSFER_H_

typedef enum NaluStatus {
	NaluStatusFalse = 0,
	NaluStatusTrue,
	NaluStatusRestart
}NaluStatus;

class Rtp2H264VideoTransfer {
public:
	Rtp2H264VideoTransfer();
	virtual ~Rtp2H264VideoTransfer();

	/**
	 * Get a H264 frame from RTP frame
	 */
	bool GetFrame(const char* src, unsigned int srcLen, char** frame, unsigned int& len);
	void DestroyVideoBuffer();

private:
	NaluStatus buffer_h264_nalu(const char* data, unsigned int datalen, bool& m);

	/**
	 * Video buffer
	 */
	bool 		mbNaluStart;		//
	char*		mpNaluBuffer;		// 帧buffer
	unsigned int mNaluInUse;
};

#endif /* SRC_MOD_ENDPOINTS_MOD_WS_RTP2H264VIDEOTRANSFER_H_ */
