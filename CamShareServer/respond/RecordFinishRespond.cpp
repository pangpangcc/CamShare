/*
 * RecordFinishRespond.cpp
 *
 *  Created on: 2016年3月8日
 *      Author: Max.Chiu
 */

#include "RecordFinishRespond.h"

RecordFinishRespond::RecordFinishRespond() {
	// TODO Auto-generated constructor stub
}

RecordFinishRespond::~RecordFinishRespond() {
	// TODO Auto-generated destructor stub
}

int RecordFinishRespond::GetData(char* buffer, int len, bool &more) {
	int ret = 0;
	more = false;

	snprintf(buffer, len, "{\"ret\":1}");
	ret = strlen(buffer);
	return ret;
}
