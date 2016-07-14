/*
 * RecordFinishRespond.h
 *
 *  Created on: 2016年3月8日
 *      Author: Max.Chiu
 */

#ifndef RECORDFINISHRESPOND_H_
#define RECORDFINISHRESPOND_H_

#include "BaseRespond.h"

class RecordFinishRespond : public BaseRespond {
public:
	RecordFinishRespond();
	virtual ~RecordFinishRespond();

	int GetData(char* buffer, int len, bool &more);

};

#endif /* RECORDFINISHRESPOND_H_ */
