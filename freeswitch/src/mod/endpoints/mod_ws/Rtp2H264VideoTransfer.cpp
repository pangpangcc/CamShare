/*
 * Rtp2H264VideoTransfer.cpp
 *
 *  Created on: 2016年9月27日
 *      Author: max
 */

#include "Rtp2H264VideoTransfer.h"

#define H264_BUFFER_SIZE (1024 * 128)

Rtp2H264VideoTransfer::Rtp2H264VideoTransfer() {
	// TODO Auto-generated constructor stub
	// 创建内存池
	switch_core_new_memory_pool(&mpPool);
	switch_buffer_create(mpPool, &mpNaluBuffer, H264_BUFFER_SIZE);
	mbNaluStart = SWITCH_FALSE;
}

Rtp2H264VideoTransfer::~Rtp2H264VideoTransfer() {
	// TODO Auto-generated destructor stub
	switch_buffer_destroy(&mpNaluBuffer);
	switch_core_destroy_memory_pool(&mpPool);
}

bool Rtp2H264VideoTransfer::GetFrame(switch_frame_t *frame, const char** data, switch_size_t *len) {
	bool bFlag = false;

	switch_status_t status = buffer_h264_nalu(frame, mpNaluBuffer, mbNaluStart);
	if (status == SWITCH_STATUS_RESTART)
	{
		// 等待关键帧
		switch_set_flag(frame, SFF_WAIT_KEY_FRAME);
		switch_buffer_zero(mpNaluBuffer);
		mbNaluStart = SWITCH_FALSE;
	}
	else if (frame->m && switch_buffer_inuse(mpNaluBuffer) > 0)
	{
		// 重置标志位
		*len = switch_buffer_inuse(mpNaluBuffer);
		switch_buffer_peek_zerocopy(mpNaluBuffer, (const void **)data);
		mbNaluStart = SWITCH_FALSE;
		bFlag = true;
	}

	return bFlag;
}

void Rtp2H264VideoTransfer::DestroyVideoBuffer() {
	switch_buffer_zero(mpNaluBuffer);
	mbNaluStart = SWITCH_FALSE;
}

switch_status_t Rtp2H264VideoTransfer::buffer_h264_nalu(switch_frame_t *frame, switch_buffer_t* nalu_buffer, switch_bool_t& nalu_28_start)
{
	uint8_t nalu_type = 0;
	uint8_t *data = (uint8_t *)frame->data;
	uint8_t nalu_hdr = *data;
	uint8_t sync_bytes[] = {0, 0, 0, 1};
	switch_buffer_t *buffer = nalu_buffer;

	nalu_type = nalu_hdr & 0x1f;

	/* hack for phones sending sps/pps with frame->m = 1 such as grandstream */
	if ((nalu_type == 7 || nalu_type == 8) && frame->m) frame->m = SWITCH_FALSE;

	if (nalu_type == 28) { // 0x1c FU-A
		int start = *(data + 1) & 0x80;
		int end = *(data + 1) & 0x40;

		nalu_type = *(data + 1) & 0x1f;

		if (start && end) return SWITCH_STATUS_RESTART;

		if (start) {
			if ( nalu_28_start == SWITCH_TRUE ) {
				nalu_28_start = SWITCH_FALSE;
				switch_buffer_zero(buffer);
			}
		} else if (end) {
			nalu_28_start = SWITCH_FALSE;
		} else if ( nalu_28_start == SWITCH_FALSE ) {
			return SWITCH_STATUS_RESTART;
		}

		if (start) {
			uint8_t nalu_idc = (nalu_hdr & 0x60) >> 5;
			nalu_type |= (nalu_idc << 5);

			switch_buffer_write(buffer, sync_bytes, sizeof(sync_bytes));
			switch_buffer_write(buffer, &nalu_type, 1);
			nalu_28_start = SWITCH_TRUE;
		}

		switch_buffer_write(buffer, (void *)(data + 2), frame->datalen - 2);
	} else if (nalu_type == 24) { // 0x18 STAP-A
		uint16_t nalu_size;
		int left = frame->datalen - 1;

		data++;

	again:
		if (left > 2) {
			nalu_size = ntohs(*(uint16_t *)data);
			data += 2;
			left -= 2;

			if (nalu_size > left) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "INVALID PACKET\n");
				switch_buffer_zero(buffer);
				return SWITCH_STATUS_FALSE;
			}

			nalu_hdr = *data;
			nalu_type = nalu_hdr & 0x1f;

			switch_buffer_write(buffer, sync_bytes, sizeof(sync_bytes));
			switch_buffer_write(buffer, (void *)data, nalu_size);
			data += nalu_size;
			left -= nalu_size;
			goto again;
		}
	} else {
		switch_buffer_write(buffer, sync_bytes, sizeof(sync_bytes));
		switch_buffer_write(buffer, frame->data, frame->datalen);
		nalu_28_start = SWITCH_FALSE;
	}

	if (frame->m) {
		nalu_28_start = SWITCH_FALSE;
	}

	return SWITCH_STATUS_SUCCESS;
}
