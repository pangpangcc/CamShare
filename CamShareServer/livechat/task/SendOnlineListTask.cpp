/*
 * SendOnlineListTask.cpp
 *  Created on: 2016年3月11日
 *      Author: max
 */

#include "SendOnlineListTask.h"
#include <amf/AmfParser.h>
#include <json/json/json.h>
#include <common/KLog.h>

SendOnlineListTask::SendOnlineListTask(void)
{
}

SendOnlineListTask::~SendOnlineListTask(void)
{
}
	
// 处理已接收数据
bool SendOnlineListTask::Handle(const TransportProtocol* tp)
{
	bool result = false;

	return result;
}
	
// 获取待发送的数据，可先获取data长度，如：GetSendData(NULL, 0, dataLen);
bool SendOnlineListTask::GetSendData(void* data, unsigned int dataSize, unsigned int& dataLen)
{
	bool result = false;

	// 构造json协议
	Json::Value root;
	for(list<string>::iterator itr = mUserList.begin(); itr != mUserList.end(); itr++) {
		Json::Value user;
		user["userId"] = *itr;
		root.append(user);
	}

	string json = "\"[]\"";
	if( !root.empty() ) {
		Json::FastWriter writer;
		json = writer.write(root);

		// 专门为livechat转义
		// 例如 [{\"userId\":\"womanId\"},{\"userId\":\"manId\"}}]
		Json::Value tmp;
		tmp = json;
		json = writer.write(tmp);
	}

	// 填入buffer
	if (json.length() < dataSize) {
		if( json.length() > 0 ) {
			memcpy(data, json.c_str(), json.length());
		}
		dataLen = json.length();

		result  = true;
	}

	FileLog("LiveChatClient", "SendOnlineListTask::GetSendData() cmd:%d, result:%d, len:%d, json:%s", GetCmdCode(), result, json.length(), json.c_str());

	return result;
}

// 获取命令号
int SendOnlineListTask::GetCmdCode()
{
	return TCMD_SEND_ONLINE_LIST;
}

// 获取处理结果
void SendOnlineListTask::GetHandleResult(LCC_ERR_TYPE& errType, string& errmsg)
{
	errType = m_errType;
	errmsg = m_errMsg;
}

// 初始化参数
bool SendOnlineListTask::InitParam(
		const list<string>& userList
		)
{
	bool bFlag = true;

	if ( !userList.empty() ) {
		std::copy(userList.begin(), userList.end(), std::back_inserter(mUserList));
	}

	return bFlag;
}

bool SendOnlineListTask::IsWaitToRespond()
{
	return false;
}

// 未完成任务的断线通知
void SendOnlineListTask::OnDisconnect()
{
}
