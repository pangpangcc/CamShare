/*
 * ReSendErrorRecordRespond.cpp
 *
 *  Created on: 2016年3月8日
 *      Author: Max.Chiu
 */

#include "ReSendErrorRecordRespond.h"

ReSendErrorRecordRespond::ReSendErrorRecordRespond() {
	// TODO Auto-generated constructor stub
	mRet = true;
	mpData = NULL;
}

ReSendErrorRecordRespond::~ReSendErrorRecordRespond() {
	// TODO Auto-generated destructor stub
}

int ReSendErrorRecordRespond::GetData(char* buffer, int len, bool &more) {
	int ret = 0;
	more = false;

	snprintf(buffer, len, "{\"ret\":%d,\"respond\":\"%s\"}", mRet, mpData?mpData:"");
	ret = strlen(buffer);
	return ret;
}

void ReSendErrorRecordRespond::SetParam(bool ret) {
	mRet = ret;
}

void ReSendErrorRecordRespond::SetDataZeroMemory(const char* data) {
	mpData = data;
}
