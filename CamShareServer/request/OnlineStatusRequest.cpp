/*
 * OnlineStatusRequest.cpp
 *
 *  Created on: 2016年3月8日
 *      Author: Max.Chiu
 */

#include "OnlineStatusRequest.h"

OnlineStatusRequest::OnlineStatusRequest() {
	// TODO Auto-generated constructor stub
	mpFreeswitch = NULL;
	mpLivechat = NULL;
	mSeq = 0;
	mUserId = "";
	mbOnline = false;
}

OnlineStatusRequest::~OnlineStatusRequest() {
	// TODO Auto-generated destructor stub
}

bool OnlineStatusRequest::StartRequest() {
	// 向LiveChat client发送消息请求
	return mpLivechat->SendOnlineStatus(mSeq, mUserId, mbOnline);
}

void OnlineStatusRequest::FinisRequest(bool bSuccess) {
}

void OnlineStatusRequest::SetParam(
		FreeswitchClient* freeswitch,
		ILiveChatClient* livechat,
		int seq,
		const string& userId,
		bool online
		) {
	mpFreeswitch = freeswitch;
	mpLivechat = livechat;
	mSeq = seq;
	mUserId = userId;
	mbOnline = online;
}
