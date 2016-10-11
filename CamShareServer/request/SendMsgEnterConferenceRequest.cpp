/*
 * SendMsgEnterConferenceRequest.cpp
 *
 *  Created on: 2016年3月8日
 *      Author: Max.Chiu
 */

#include "SendMsgEnterConferenceRequest.h"

SendMsgEnterConferenceRequest::SendMsgEnterConferenceRequest() {
	// TODO Auto-generated constructor stub
	mpFreeswitch = NULL;
	mpLivechat = NULL;
	mSeq = 0;
	mFromId = "";
	mToId = "";
}

SendMsgEnterConferenceRequest::~SendMsgEnterConferenceRequest() {
	// TODO Auto-generated destructor stub
}

bool SendMsgEnterConferenceRequest::StartRequest() {
	// 构造json协议
	string msg = ParamString();

	// 向LiveChat client发送消息请求
	return mpLivechat->SendMsg(mSeq, mFromId, mToId, msg);
}

void SendMsgEnterConferenceRequest::FinisRequest(bool bSuccess) {
}

void SendMsgEnterConferenceRequest::SetParam(
		FreeswitchClient* freeswitch,
		ILiveChatClient* livechat,
		int seq,
		const string& fromId,
		const string& toId,
		const list<string>& userList
		) {
	mpFreeswitch = freeswitch;
	mpLivechat = livechat;
	mSeq = seq;
	mFromId = fromId;
	mToId = toId;
	std::copy(userList.begin(), userList.end(), std::back_inserter(mUserList));
}

string SendMsgEnterConferenceRequest::ParamString() {
	// 构造json协议
	Json::Value root;
	root["cmd"] = 1;

	Json::Value data;
	data["fromId"] = mFromId;
	data["toId"] = mToId;

	Json::Value userList;
	for(list<string>::const_iterator itr = mUserList.begin(); itr != mUserList.end(); itr++) {
		userList.append(*itr);
	}
	data["userlist"] = userList;

	root["data"] = data;
	Json::FastWriter writer;
	string msg = writer.write(root);

	return msg;
}
