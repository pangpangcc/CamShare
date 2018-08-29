/*
 * SendEnterConferenceTask.cpp
 *  Created on: 2016年3月11日
 *      Author: max
 */

#include "SendEnterConferenceTask.h"
#include <amf/AmfParser.h>
#include <json/json/json.h>
#include <common/KLog.h>

SendEnterConferenceTask::SendEnterConferenceTask(void)
{
	mServer = "";
	mFromId = "";
	mToId = "";
	mKey = "";
}

SendEnterConferenceTask::~SendEnterConferenceTask(void)
{
}
	
// 处理已接收数据
bool SendEnterConferenceTask::Handle(const TransportProtocol* tp)
{
	bool result = false;

	return result;
}
	
// 获取待发送的数据，可先获取data长度，如：GetSendData(NULL, 0, dataLen);
bool SendEnterConferenceTask::GetSendData(void* data, unsigned int dataSize, unsigned int& dataLen)
{
	bool result = false;

	// 构造json协议
	Json::Value root;
	root["fromId"] = mFromId;
	root["toId"] = mToId;
	root["server"] = mServer;
	root["key"] = mKey;
	Json::FastWriter writer;
	string json = writer.write(root);

	// 填入buffer
	if (json.length() < dataSize) {
		if( json.length() > 0 ) {
			memcpy(data, json.c_str(), json.length());
		}
		dataLen = json.length();

		result  = true;
	}

	// 打log
	FileLog("LiveChatClient", "SendEnterConferenceTask::GetSendData() cmd:%d, result:%d, len:%d, json:%s", GetCmdCode(), result, json.length(), json.c_str());

	return result;
}

// 获取命令号
int SendEnterConferenceTask::GetCmdCode()
{
	return TCMD_SENDENTERCONFERENCE;
}

// 获取处理结果
void SendEnterConferenceTask::GetHandleResult(LCC_ERR_TYPE& errType, string& errmsg)
{
	errType = m_errType;
	errmsg = m_errMsg;
}

// 初始化参数
bool SendEnterConferenceTask::InitParam(
		const string& server,
		const string& fromId,
		const string& toId,
		const string& key
		)
{
	bool bFlag = false;
	if ( !server.empty() && !fromId.empty() && !toId.empty() ) {
		mServer = server;
		mFromId = fromId;
		mToId = toId;
		mKey = key;
		bFlag = true;
	}

	return bFlag;
}

bool SendEnterConferenceTask::IsWaitToRespond()
{
	return false;
}

// 未完成任务的断线通知
void SendEnterConferenceTask::OnDisconnect()
{
	if (NULL != m_listener) {
		m_listener->OnSendEnterConference(m_client, GetSeq(), mFromId, mToId, mKey, LCC_ERR_CONNECTFAIL, "");
	}
}
