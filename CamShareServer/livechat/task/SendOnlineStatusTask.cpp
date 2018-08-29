/*
 * SendOnlineStatusTask.cpp
 *  Created on: 2016年3月11日
 *      Author: max
 */

#include "SendOnlineStatusTask.h"
#include <amf/AmfParser.h>
#include <json/json/json.h>
#include <common/KLog.h>

SendOnlineStatusTask::SendOnlineStatusTask(void)
{
	mUserId = "";
	mbOnline = false;
}

SendOnlineStatusTask::~SendOnlineStatusTask(void)
{
}
	
// 处理已接收数据
bool SendOnlineStatusTask::Handle(const TransportProtocol* tp)
{
	bool result = false;

	return result;
}
	
// 获取待发送的数据，可先获取data长度，如：GetSendData(NULL, 0, dataLen);
bool SendOnlineStatusTask::GetSendData(void* data, unsigned int dataSize, unsigned int& dataLen)
{
	bool result = false;

	// 构造json协议
	Json::Value root;
	root["userId"] = mUserId;
	root["login"] = mbOnline;
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

	FileLog("LiveChatClient", "SendOnlineStatusTask::GetSendData() cmd:%d, result:%d, len:%d, json:%s", GetCmdCode(), result, json.length(), json.c_str());

	return result;
}

// 获取命令号
int SendOnlineStatusTask::GetCmdCode()
{
	return TCMD_SEND_ONLINE_STATUS_CHANGE;
}

// 获取处理结果
void SendOnlineStatusTask::GetHandleResult(LCC_ERR_TYPE& errType, string& errmsg)
{
	errType = m_errType;
	errmsg = m_errMsg;
}

// 初始化参数
bool SendOnlineStatusTask::InitParam(
		const string& userId,
		bool online
		)
{
	bool bFlag = false;
	if ( !userId.empty() ) {
		mUserId = userId;
		mbOnline = online;
		bFlag = true;
	}

	return bFlag;
}

bool SendOnlineStatusTask::IsWaitToRespond()
{
	return false;
}

// 未完成任务的断线通知
void SendOnlineStatusTask::OnDisconnect()
{
}
