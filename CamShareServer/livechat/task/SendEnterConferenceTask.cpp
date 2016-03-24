/*
 * author: Samson.Fan
 *   date: 2015-03-30
 *   file: SendEnterConferenceTask.h
 *   desc: 检测版本Task实现类
 */


#include "SendEnterConferenceTask.h"
#include <amf/AmfParser.h>
#include <json/json/json.h>
#include <common/KLog.h>
#include <common/CheckMomoryLeak.h>

SendEnterConferenceTask::SendEnterConferenceTask(void)
{
	mFromId = "";
	mToId = "";
}

SendEnterConferenceTask::~SendEnterConferenceTask(void)
{
}
	
// 处理已接收数据
bool SendEnterConferenceTask::Handle(const TransportProtocol* tp)
{
	bool result = false;

	Json::Value root;
	Json::Reader reader;
	if( reader.parse((char*)tp->data, root, false) ) {
		if( root.isObject() ) {
			if( root["ret"].isInt() ) {
				result = true;

				if( root["ret"].asInt() == 0 ) {
					m_errType = LCC_ERR_SUCCESS;
				}
			}
		}
	}

	// 协议解析失败
	if (!result) {
		m_errType = LCC_ERR_PROTOCOLFAIL;
		m_errMsg = "";
	}

	// 打log
	FileLog("LiveChatClient", "SendEnterConferenceTask::Handle() errType:%d, errMsg:%s"
			, m_errType, m_errMsg.c_str());

	// 通知listener
	if (NULL != m_listener) {
		m_listener->OnSendEnterConference(m_client, GetSeq(), mFromId, mToId, m_errType, m_errMsg);
	}

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
	Json::FastWriter writer;
	string json = writer.write(root);

	// 填入buffer
	if (json.length() < dataSize) {
		memcpy(data, json.c_str(), json.length());
		dataLen = json.length();

		result  = true;
	}

	// 打log
	FileLog("LiveChatClient", "SendEnterConferenceTask::GetSendData() result:%d, json:%s", result, json.c_str());

	return result;
}

// 获取待发送数据的类型
TASK_PROTOCOL_TYPE SendEnterConferenceTask::GetSendDataProtocolType()
{
	return JSON_PROTOCOL;
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
		const string& fromId,
		const string& toId
		)
{
	bool bFlag = false;
	if ( !fromId.empty() && !toId.empty() ) {
		mFromId = fromId;
		mToId = toId;
		bFlag = true;
	}

	return bFlag;
}

// 未完成任务的断线通知
void SendEnterConferenceTask::OnDisconnect()
{
	if (NULL != m_listener) {
		m_listener->OnSendEnterConference(m_client, GetSeq(), mFromId, mToId, LCC_ERR_CONNECTFAIL, "");
	}
}
