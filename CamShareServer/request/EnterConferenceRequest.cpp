/*
 * EnterConferenceRequest.cpp
 *
 *  Created on: 2016年3月8日
 *      Author: Max.Chiu
 */

#include "EnterConferenceRequest.h"

#define ACTIVE_KEY "ACTIVE"
#define TIMER_KEY "TIMER"

EnterConferenceRequest::EnterConferenceRequest() {
	// TODO Auto-generated constructor stub
	mpFreeswitch = NULL;
	mpLivechat = NULL;
	mSeq = 0;
	mServerId = "";
	mFromId = "";
	mToId = "";
	mType = Member;
	mCheckType = Timer;
    mChatType = ENTERCONFERENCETYPE_CAMSHARE;
}

EnterConferenceRequest::~EnterConferenceRequest() {
	// TODO Auto-generated destructor stub
}

bool EnterConferenceRequest::StartRequest() {
	// 向LiveChat client发送进入聊天室内请求
	string key = GetKey();
	return mpLivechat->SendEnterConference(mSeq, mServerId, mFromId, mToId, key, mChatType);
}

void EnterConferenceRequest::FinisRequest(bool bSuccess) {
	if( !bSuccess ) {
		// 任务处理失败
		if( mCheckType != Timer ) {
			// 不是定时检测
			if( mpFreeswitch ) {
				// 踢出用户
				mpFreeswitch->KickUserFromConference(mFromId, mToId, "");
			}
		}
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
		EnterConferenceRequestCheckType checkType,
        ENTERCONFERENCETYPE chatType
		) {
	mpFreeswitch = freeswitch;
	mpLivechat = livechat;
	mSeq = seq;
	mServerId = serverId;
	mFromId = fromId;
	mToId = toId;
	mType = type;
	mCheckType = checkType;
    mChatType = chatType;
}

MemberType EnterConferenceRequest::GetMemberType() {
	return mType;
}

string EnterConferenceRequest::GetKey() {
	string key = TIMER_KEY;
	switch( mCheckType ) {
	case Timer:{
		key = TIMER_KEY;
	}break;
	case Active:{
		key = ACTIVE_KEY;
	}break;
	default:break;
	}
	return key;
}

string EnterConferenceRequest::GetIdentify(
		const string& fromId,
		const string& toId,
		const string& key
		) {
	char identify[2048] = {'\0'};
	snprintf(identify, sizeof(identify), "%s_%s_%s", fromId.c_str(), toId.c_str(), key.c_str());
	return identify;
}
