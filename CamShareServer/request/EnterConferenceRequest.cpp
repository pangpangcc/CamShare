/*
 * EnterConferenceRequest.cpp
 *
 *  Created on: 2016年3月8日
 *      Author: Max.Chiu
 */

#include "EnterConferenceRequest.h"

EnterConferenceRequest::EnterConferenceRequest() {
	// TODO Auto-generated constructor stub
	mpFreeswitch = NULL;
	mpLivechat = NULL;
	mSeq = 0;
	mServerId = "";
	mFromId = "";
	mToId = "";
	mType = Member;
	mKey = "";
}

EnterConferenceRequest::~EnterConferenceRequest() {
	// TODO Auto-generated destructor stub
}

bool EnterConferenceRequest::StartRequest() {
	// 向LiveChat client发送进入聊天室内请求
	return mpLivechat->SendEnterConference(mSeq, mServerId, mFromId, mToId, mKey);
}

void EnterConferenceRequest::FinisRequest(bool bSuccess) {
	if( !bSuccess ) {
		// 任务处理失败
		// 断开用户
		mpFreeswitch->KickUserFromConference(mFromId, mToId, "");
	}
}

void EnterConferenceRequest::SetParam(
		FreeswitchClient* freeswitch,
		ILiveChatClient* livechat,
		int seq,
		const string& serverId,
		const string& fromId,
		const string& toId,
		MemberType type,
		const string& key
		) {
	mpFreeswitch = freeswitch;
	mpLivechat = livechat;
	mSeq = seq;
	mServerId = serverId;
	mFromId = fromId;
	mToId = toId;
	mType = type;
	mKey = key;
}

MemberType EnterConferenceRequest::GetMemberType() {
	return mType;
}
