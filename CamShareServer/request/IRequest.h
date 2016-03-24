/*
 * IRequest.h
 *
 *  Created on: 2015-11-14
 *      Author: Max
 */

#ifndef IRequest_H_
#define IRequest_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>
using namespace std;

class IRequest {
public:
	virtual ~IRequest(){};

	/**
	 * 是否需要等待返回
	 */
	virtual bool IsNeedReturn() = 0;


};

#endif /* IREQUEST_H_ */
