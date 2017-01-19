/*
 * ExecCmdRespond.h
 *
 *  Created on: 2016年3月8日
 *      Author: Max.Chiu
 */

#ifndef EXECCMDRESPOND_H_
#define EXECCMDRESPOND_H_

#include <respond/BaseRespond.h>

class ExecCmdRespond : public BaseRespond {
public:
	ExecCmdRespond();
	virtual ~ExecCmdRespond();

	int GetData(char* buffer, int len, bool &more);
	void SetParam(int ret);

private:
	int mRet;
};

#endif /* EXECCMDRESPOND_H_ */
