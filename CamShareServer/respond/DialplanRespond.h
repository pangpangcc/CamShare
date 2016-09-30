/*
 * DialplanRespond.h
 *
 *  Created on: 2016年3月8日
 *      Author: Max.Chiu
 */

#ifndef DIALPLANRESPOND_H_
#define DIALPLANRESPOND_H_

#include "BaseRespond.h"

class DialplanRespond : public BaseRespond {
public:
	DialplanRespond();
	virtual ~DialplanRespond();

	int GetData(char* buffer, int len, bool &more);
	void SetParam(bool ret);

private:
	bool mRet;
};

#endif /* DIALPLANRESPOND_H_ */
