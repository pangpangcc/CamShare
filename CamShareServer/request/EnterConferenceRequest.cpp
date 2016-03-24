/*
 * EnterConferenceRequest.cpp
 *
 *  Created on: 2016年3月8日
 *      Author: Max.Chiu
 */

#include "EnterConferenceRequest.h"

EnterConferenceRequest::EnterConferenceRequest() {
	// TODO Auto-generated constructor stub
	mFromId = "";
	mToId = "";
	mType = Member;
}

EnterConferenceRequest::~EnterConferenceRequest() {
	// TODO Auto-generated destructor stub
}

void EnterConferenceRequest::SetParam(
		const string& fromId,
		const string& toId,
		MemberType type
		) {
	mFromId = fromId;
	mToId = toId;
	mType = type;
}

MemberType EnterConferenceRequest::GetMemberType() {
	return mType;
}
