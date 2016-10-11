/*
 * BaseRequest.h
 *
 *  Created on: 2015-12-3
 *      Author: Max
 */

#ifndef BaseRequest_H_
#define BaseRequest_H_

#include "IRequest.h"

class BaseRequest : public IRequest {
public:
	BaseRequest();
	virtual ~BaseRequest();

	virtual bool IsNeedReturn();

	virtual string ToString();
};

#endif /* BASEREQUEST_H_ */
