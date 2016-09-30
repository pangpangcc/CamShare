/*
 * DialplanRespond.cpp
 *
 *  Created on: 2016年3月8日
 *      Author: Max.Chiu
 */

#include "DialplanRespond.h"

DialplanRespond::DialplanRespond() {
	// TODO Auto-generated constructor stub
	mRet = false;
}

DialplanRespond::~DialplanRespond() {
	// TODO Auto-generated destructor stub
}

int DialplanRespond::GetData(char* buffer, int len, bool &more) {
	int ret = 0;
	more = false;

	snprintf(buffer, len, "{\"ret\":%d}", mRet);
	ret = strlen(buffer);
	return ret;
}

void DialplanRespond::SetParam(bool ret) {
	mRet = ret;
}
