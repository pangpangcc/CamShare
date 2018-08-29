/*
 * ReSendErrorRecordRespond.h
 *
 *  Created on: 2016年3月8日
 *      Author: Max.Chiu
 */

#ifndef RESENDERRORRECORDRESPOND_H_
#define RESENDERRORRECORDRESPOND_H_

#include "BaseRespond.h"

class ReSendErrorRecordRespond : public BaseRespond {
public:
	ReSendErrorRecordRespond();
	virtual ~ReSendErrorRecordRespond();

	int GetData(char* buffer, int len, bool &more);
	void SetParam(bool ret);
	void SetDataZeroMemory(const char* data);

private:
	bool mRet;
	const char* mpData;
};

#endif /* RESENDERRORRECORDRESPOND_H_ */
