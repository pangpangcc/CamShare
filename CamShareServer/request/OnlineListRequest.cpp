/*
 * OnlineListRequest.cpp
 *
 *  Created on: 2016年3月8日
 *      Author: Max.Chiu
 */

#include "OnlineListRequest.h"

OnlineListRequest::OnlineListRequest() {
	// TODO Auto-generated constructor stub
	mpFreeswitch = NULL;
	mpLivechat = NULL;
	mSeq = 0;
}

OnlineListRequest::~OnlineListRequest() {
	// TODO Auto-generated destructor stub
}

bool OnlineListRequest::StartRequest() {
	// 向LiveChat client发送消息请求
	return mpLivechat->SendOnlineList(mSeq, mUserList);
}

void OnlineListRequest::FinisRequest(bool bSuccess) {
}

void OnlineListRequest::SetParam(
		FreeswitchClient* freeswitch,
		ILiveChatClient* livechat,
		int seq,
		const list<string>& userList
		) {
	mpFreeswitch = freeswitch;
	mpLivechat = livechat;
	mSeq = seq;
	std::copy(userList.begin(), userList.end(), std::back_inserter(mUserList));
}
