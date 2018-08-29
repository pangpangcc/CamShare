/*
 * Rtp2H264VideoTransfer.cpp
 *
 *  Created on: 2016年9月27日
 *      Author: max
 */

#include "Rtp2H264VideoTransfer.h"

#include <string.h>
#include <unistd.h>
#include <netinet/in.h>

#include <common/KLog.h>
#include <common/Arithmetic.h>

#define H264_BUFFER_SIZE (1024 * 128)

Rtp2H264VideoTransfer::Rtp2H264VideoTransfer() {
	// TODO Auto-generated constructor stub
	mbNaluStart = false;
	mpNaluBuffer = new char[H264_BUFFER_SIZE];
	mNaluInUse = 0;
}

Rtp2H264VideoTransfer::~Rtp2H264VideoTransfer() {
	// TODO Auto-generated destructor stub
	if( mpNaluBuffer ) {
		delete[] mpNaluBuffer;
		mpNaluBuffer = NULL;
	}
}

bool Rtp2H264VideoTransfer::GetFrame(const char* src, unsigned int srcLen, char** frame, unsigned int &len) {
	bool bFlag = false;
	bool m = false;

	NaluStatus status = buffer_h264_nalu(src, srcLen, m);
	if (status == NaluStatusRestart)
	{
		// 等待关键帧
		mbNaluStart = false;
		memset(mpNaluBuffer, 0, H264_BUFFER_SIZE);
		mNaluInUse = 0;
	}
	else if (m && mNaluInUse > 0)
	{
		// 重置标志位
		mbNaluStart = false;
		len = mNaluInUse;
		*frame = mpNaluBuffer;
		bFlag = true;
	}

	FileLog("CamshareClient",
			"Rtp2H264VideoTransfer::GetFrame( "
			"this : %p, "
			"srcLen : %u, "
			"mNaluInUse : %u, "
			"mbNaluStart : %s, "
			"m : %s, "
			"status : %d, "
			"bFlag : %s "
			")",
			this,
			srcLen,
			mNaluInUse,
			mbNaluStart?"true":"false",
			m?"true":"false",
			status,
			bFlag?"true":"false"
			);

	return bFlag;
}

void Rtp2H264VideoTransfer::DestroyVideoBuffer() {
//	FileLog("CamshareClient",
//			"Rtp2H264VideoTransfer::DestroyVideoBuffer( "
//			"this : %p "
//			")",
//			this
//			);
	memset(mpNaluBuffer, 0, H264_BUFFER_SIZE);
	mNaluInUse = 0;
	mbNaluStart = false;
}

NaluStatus Rtp2H264VideoTransfer::buffer_h264_nalu(const char* src, unsigned int datalen, bool& m)
{
	unsigned char nalu_type = 0;
	unsigned char *data = (unsigned char *)src;
	unsigned char nalu_hdr = *data;
	unsigned char sync_bytes[] = {0, 0, 0, 1};

	nalu_type = nalu_hdr & 0x1f;
	m = true;

	FileLog("CamshareClient",
			"Rtp2H264VideoTransfer::buffer_h264_nalu( "
			"this : %p, "
			"datalen : %u, "
			"nalu_type : %u "
			")",
			this,
			datalen,
			nalu_type
			);

	/* hack for phones sending sps/pps with frame->m = 1 such as grandstream */
	if ((nalu_type == 7 || nalu_type == 8) && m) m = false;

	if (nalu_type == 28) { // 0x1c FU-A
		int start = *(data + 1) & 0x80;
		int end = *(data + 1) & 0x40;
		m = ((end == 0x40)?true:false);
		FileLog("CamshareClient",
				"Rtp2H264VideoTransfer::buffer_h264_nalu( "
				"this : %p, "
				"start : %u, "
				"end : %u, "
				"m : %s, "
				"mbNaluStart : %s "
				")",
				this,
				start,
				end,
				m?"true":"false",
				mbNaluStart?"true":"false"
				);

		nalu_type = *(data + 1) & 0x1f;

		if (start && end) return NaluStatusRestart;

		if (start) {
			if ( mbNaluStart == true ) {
				DestroyVideoBuffer();
			}
		} else if (end) {
			mbNaluStart = false;
		} else if ( mbNaluStart == false ) {
			return NaluStatusRestart;
		}

		if (start) {
			unsigned char nalu_idc = (nalu_hdr & 0x60) >> 5;
			nalu_type |= (nalu_idc << 5);

			memcpy(mpNaluBuffer + mNaluInUse, sync_bytes, sizeof(sync_bytes));
			mNaluInUse += sizeof(sync_bytes);
			memcpy(mpNaluBuffer + mNaluInUse, &nalu_type, 1);
			mNaluInUse += 1;

			mbNaluStart = true;
		}

		memcpy(mpNaluBuffer + mNaluInUse, (void *)(data + 2), datalen - 2);
		mNaluInUse += datalen - 2;

	} else if (nalu_type == 24) { // 0x18 STAP-A
		unsigned short int nalu_size;
		int left = datalen - 1;

		data++;

	again:
		if (left > 2) {
			nalu_size = ntohs(*(unsigned short int *)data);
			data += 2;
			left -= 2;

			if (nalu_size > left) {
				DestroyVideoBuffer();
			}

			nalu_hdr = *data;
			nalu_type = nalu_hdr & 0x1f;

			memcpy(mpNaluBuffer + mNaluInUse, sync_bytes, sizeof(sync_bytes));
			mNaluInUse += sizeof(sync_bytes);
			memcpy(mpNaluBuffer + mNaluInUse, (void *)data, nalu_size);
			mNaluInUse += nalu_size;

			data += nalu_size;
			left -= nalu_size;
			goto again;
		}
	} else {
		memcpy(mpNaluBuffer + mNaluInUse, sync_bytes, sizeof(sync_bytes));
		mNaluInUse += sizeof(sync_bytes);
		memcpy(mpNaluBuffer + mNaluInUse, data, datalen);
		mNaluInUse += datalen;

		mbNaluStart = false;
	}

	if (m) {
		mbNaluStart = false;
	}

	return NaluStatusTrue;
}
