/*
 * CheckVerTask.cpp
 *  Created on: 2016年3月11日
 *      Author: max
 */

#include "CheckVerTask.h"

#include <amf/AmfParser.h>
#include <json/json/json.h>
#include <common/KLog.h>

CheckVerTask::CheckVerTask(void)
{
	m_version = "";
}

CheckVerTask::~CheckVerTask(void)
{
}
	
// 处理已接收数据
bool CheckVerTask::Handle(const TransportProtocol* tp)
{
	bool result = false;

	AmfParser parser;
	amf_object_handle root = parser.Decode((char*)tp->data, tp->GetDataLength());
	if ( !root.isnull() && (root->type == DT_FALSE || root->type == DT_TRUE) ) {
		m_errType = root->boolValue ? LCC_ERR_SUCCESS : LCC_ERR_CHECKVERFAIL;
		m_errMsg = "";

		result = true;
	}

//	Json::Value root;
//	Json::Reader reader;
//	if( reader.parse((char*)tp->data, root, false) ) {
//		if( root.isObject() ) {
//			if( root["ret"].isInt() && (root["ret"].asInt() == 0) ) {
//				result = true;
//
//				m_errType = LCC_ERR_SUCCESS;
//			}
//		}
//	}

	// 协议解析失败
	if (!result) {
		m_errType = LCC_ERR_PROTOCOLFAIL;
		m_errMsg = "";
	}

	// 打log
	FileLog("LiveChatClient", "CheckVerTask::Handle() errType:%d, errMsg:%s"
			, m_errType, m_errMsg.c_str());

	return result;
}
	
// 获取待发送的数据，可先获取data长度，如：GetSendData(NULL, 0, dataLen);
bool CheckVerTask::GetSendData(void* data, unsigned int dataSize, unsigned int& dataLen)
{
	bool result = false;

	// 构造json协议
	Json::Value root;
	Json::FastWriter writer;
	root["cmd"] = GetCmdCode();
	root["data"] = m_version;
	string json = writer.write(root);

	// 填入buffer
	if (json.length() + 1 < dataSize) {
		if( json.length() > 0 ) {
			memcpy(data, json.c_str(), json.length());
		}
		((char*)data)[json.length()] = 0;
		dataLen = json.length() + 1;

		result  = true;
	}

	FileLog("LiveChatClient", "CheckVerTask::GetSendData() result:%d, len:%d, json:%s", result, json.length(), json.c_str());

	return result;
}

TASK_PROTOCOL_TYPE CheckVerTask::GetSendDataProtocolType()
{
	return NOHEAD_PROTOCOL;
}

// 获取命令号
int CheckVerTask::GetCmdCode()
{
	return TCMD_CHECKVER;	
}

// 获取处理结果
void CheckVerTask::GetHandleResult(LCC_ERR_TYPE& errType, string& errmsg)
{
	errType = m_errType;
	errmsg = m_errMsg;
}

// 初始化参数
bool CheckVerTask::InitParam(const string& version)
{
	if (!version.empty()) {
		m_version = version;
	}
	return !m_version.empty();
}

// 未完成任务的断线通知
void CheckVerTask::OnDisconnect()
{
	// 不需要通知上层
}
