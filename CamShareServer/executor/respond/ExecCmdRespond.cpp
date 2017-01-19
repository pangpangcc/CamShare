/*
 * ExecCmdRespond.cpp
 *
 *  Created on: 2016年3月8日
 *      Author: Max.Chiu
 */

#include "ExecCmdRespond.h"

ExecCmdRespond::ExecCmdRespond() {
	// TODO Auto-generated constructor stub
	mRet = -1;
}

ExecCmdRespond::~ExecCmdRespond() {
	// TODO Auto-generated destructor stub
}

int ExecCmdRespond::GetData(char* buffer, int len, bool &more) {
	int ret = 0;
	more = false;

	snprintf(buffer, len, "{\"ret\":%d}", mRet);
	ret = strlen(buffer);
	return ret;
}

void ExecCmdRespond::SetParam(int ret) {
	mRet = ret;
}
