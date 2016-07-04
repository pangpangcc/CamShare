/*
 * SendMsgTask.cpp
 *  Created on: 2016年3月11日
 *      Author: max
 */

#include "SendMsgTask.h"
#include <amf/AmfParser.h>
#include <json/json/json.h>
#include <common/KLog.h>

SendMsgTask::SendMsgTask(void)
{
	mFromId = "";
	mToId = "";
}

SendMsgTask::~SendMsgTask(void)
{
}
	
// 处理已接收数据
bool SendMsgTask::Handle(const TransportProtocol* tp)
{
	bool result = false;

	return result;
}
	
// 获取待发送的数据，可先获取data长度，如：GetSendData(NULL, 0, dataLen);
bool SendMsgTask::GetSendData(void* data, unsigned int dataSize, unsigned int& dataLen)
{
	bool result = false;

	// 构造json协议
	Json::Value root;
	root["fromId"] = mFromId;
	root["toId"] = mToId;
	root["msg"] = mMsg;
	Json::FastWriter writer;
	string json = writer.write(root);

	// 填入buffer
	if (json.length() < dataSize) {
		// 打log
		FileLog("LiveChatClient", "SendMsgTask::GetSendData() len:%d, json:%s", json.length(), json.c_str());
		if( json.length() > 0 ) {
			memcpy(data, json.c_str(), json.length());
		}
		dataLen = json.length();

		result  = true;
	}

	// 打log
	FileLog("LiveChatClient", "SendMsgTask::GetSendData() result:%d, json:%s", result, json.c_str());

	return result;
}

// 获取命令号
int SendMsgTask::GetCmdCode()
{
	return TCMD_SENDMSG;
}

// 获取处理结果
void SendMsgTask::GetHandleResult(LCC_ERR_TYPE& errType, string& errmsg)
{
	errType = m_errType;
	errmsg = m_errMsg;
}

// 初始化参数
bool SendMsgTask::InitParam(
		const string& fromId,
		const string& toId,
		const string& msg
		)
{
	bool bFlag = false;
	if ( !fromId.empty() && !toId.empty() ) {
		mFromId = fromId;
		mToId = toId;
		mMsg = msg;
		bFlag = true;
	}

	return bFlag;
}

bool SendMsgTask::IsWaitToRespond()
{
	return false;
}

// 未完成任务的断线通知
void SendMsgTask::OnDisconnect()
{
}
