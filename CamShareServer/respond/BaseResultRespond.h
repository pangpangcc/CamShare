/*
 * BaseResultRespond.h
 *
 *  Created on: 2016年3月8日
 *      Author: Max.Chiu
 */

#ifndef BASERESULTRESPOND_H_
#define BASERESULTRESPOND_H_

#include "BaseRespond.h"

class BaseResultRespond : public BaseRespond {
public:
	BaseResultRespond();
	virtual ~BaseResultRespond();

	int GetData(char* buffer, int len, bool &more);
	void SetParam(bool ret);

private:
	bool mRet;
};

#endif /* BASERESULTRESPOND_H_ */
