/*
 * author: Samson.Fan
 *   date: 2015-03-30
 *   file: CheckVerTask.h
 *   desc: 检测版本Task实现类
 */


#include "CheckVerTask.h"
#include <amf/AmfParser.h>
#include <json/json/json.h>
#include <common/KLog.h>
#include <common/CheckMomoryLeak.h>

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

//	AmfParser parser;
//	amf_object_handle root = parser.Decode((char*)tp->data, tp->GetDataLength());
//	if (!root.isnull()
//		&& (root->type == DT_FALSE || root->type == DT_TRUE))
//	{
//		m_errType = root->boolValue ? LCC_ERR_SUCCESS : LCC_ERR_CHECKVERFAIL;
//		m_errMsg = "";
//
//		result = true;
//	}

	Json::Value root;
	Json::Reader reader;
	if( reader.parse((char*)tp->data, root, false) ) {
		if( root.isObject() ) {
			if( root["ret"].isInt() && (root["ret"].asInt() == 0) ) {
				result = true;

				m_errType = LCC_ERR_SUCCESS;
			}
		}
	}

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
	Json::Value root(m_version);
	Json::FastWriter writer;
	string json = writer.write(root);

	// 填入buffer
	if (json.length() < dataSize) {
		memcpy(data, json.c_str(), json.length());
		dataLen = json.length();

		result  = true;
	}

	// 打log
	FileLog("LiveChatClient", "CheckVerTask::GetSendData() result:%d, json:%s", result, json.c_str());

	return result;
}

// 获取待发送数据的类型
TASK_PROTOCOL_TYPE CheckVerTask::GetSendDataProtocolType()
{
	return JSON_PROTOCOL;
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
