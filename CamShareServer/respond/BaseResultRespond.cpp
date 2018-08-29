/*
 * BaseResultRespond.cpp
 *
 *  Created on: 2016年3月8日
 *      Author: Max.Chiu
 */

#include "BaseResultRespond.h"

BaseResultRespond::BaseResultRespond() {
	// TODO Auto-generated constructor stub
	mRet = true;
}

BaseResultRespond::~BaseResultRespond() {
	// TODO Auto-generated destructor stub
}

int BaseResultRespond::GetData(char* buffer, int len, bool &more) {
	int ret = 0;
	more = false;

	snprintf(buffer, len, "{\"ret\":%d}", mRet);
	ret = strlen(buffer);
	return ret;
}

void BaseResultRespond::SetParam(bool ret) {
	mRet = ret;
}
