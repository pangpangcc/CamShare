/*
 * Rtp2H264VideoTransfer.h
 *
 *  Created on: 2016年9月27日
 *      Author: max
 */

#ifndef SRC_MOD_ENDPOINTS_MOD_WS_RTP2H264VIDEOTRANSFER_H_
#define SRC_MOD_ENDPOINTS_MOD_WS_RTP2H264VIDEOTRANSFER_H_

#include <switch.h>

class Rtp2H264VideoTransfer {
public:
	Rtp2H264VideoTransfer();
	virtual ~Rtp2H264VideoTransfer();

	/**
	 * Get a H264 frame from RTP frame
	 */
	bool GetFrame(switch_frame_t *frame, const char** data, switch_size_t *len);
	void DestroyVideoBuffer();

private:
	switch_status_t buffer_h264_nalu(switch_frame_t *frame, switch_buffer_t* nalu_buffer, switch_bool_t& nalu_28_start);

	/**
	 * 内存池
	 */
	switch_memory_pool_t *mpPool;

	/**
	 * Video buffer
	 */
	switch_bool_t 		mbNaluStart;		//
	switch_buffer_t*	mpNaluBuffer;		// 帧buffer

};

#endif /* SRC_MOD_ENDPOINTS_MOD_WS_RTP2H264VIDEOTRANSFER_H_ */
