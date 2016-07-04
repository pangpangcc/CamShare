/*
 * SendMsgExitConferenceRequest.cpp
 *
 *  Created on: 2016年3月8日
 *      Author: Max.Chiu
 */

#include "SendMsgExitConferenceRequest.h"

SendMsgExitConferenceRequest::SendMsgExitConferenceRequest() {
	// TODO Auto-generated constructor stub
	mpFreeswitch = NULL;
	mpLivechat = NULL;
	mSeq = 0;
	mFromId = "";
	mToId = "";
}

SendMsgExitConferenceRequest::~SendMsgExitConferenceRequest() {
	// TODO Auto-generated destructor stub
}

bool SendMsgExitConferenceRequest::StartRequest() {
	// 构造json协议
	Json::Value root;
	root["cmd"] = 0;

	Json::Value data;
	data["fromId"] = mFromId;
	data["toId"] = mToId;
	root["data"] = data;

	Json::FastWriter writer;
	string msg = writer.write(root);

	// 向LiveChat client发送消息请求
	return mpLivechat->SendMsg(mSeq, mFromId, mToId, msg);
}

void SendMsgExitConferenceRequest::FinisRequest(bool bSuccess) {
}

void SendMsgExitConferenceRequest::SetParam(
		FreeswitchClient* freeswitch,
		ILiveChatClient* livechat,
		int seq,
		const string& fromId,
		const string& toId
		) {
	mpFreeswitch = freeswitch;
	mpLivechat = livechat;
	mSeq = seq;
	mFromId = fromId;
	mToId = toId;
}
