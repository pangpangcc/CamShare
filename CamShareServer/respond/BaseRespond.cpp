/*
 * BaseRespond.cpp
 *
 *  Created on: 2016年3月11日
 *      Author: max
 */

#include "BaseRespond.h"

BaseRespond::BaseRespond() {
	// TODO Auto-generated constructor stub

}

BaseRespond::~BaseRespond() {
	// TODO Auto-generated destructor stub
}

int BaseRespond::GetData(char* buffer, int len, bool &more) {
	int ret = 0;
	more = false;

	snprintf(buffer, len, "{\"ret\":1}");
	ret = strlen(buffer);
	return ret;
}
