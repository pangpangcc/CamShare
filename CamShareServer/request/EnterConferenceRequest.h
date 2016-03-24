/*
 * EnterConferenceRequest.h
 *
 *  Created on: 2016年3月8日
 *      Author: Max.Chiu
 */

#ifndef EnterConferenceRequest_H_
#define EnterConferenceRequest_H_

#include "BaseRequest.h"

#include <FreeswitchClient.h>

class EnterConferenceRequest : public BaseRequest {
public:
	EnterConferenceRequest();
	virtual ~EnterConferenceRequest();

	void SetParam(const string& fromId, const string& toId, MemberType type);

	MemberType GetMemberType();

private:
	string mFromId;
	string mToId;
	MemberType mType;
};

#endif /* ENTERCONFERENCERequest_H_ */
