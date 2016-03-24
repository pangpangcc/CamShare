/*
 * author: Samson.Fan
 *   date: 2015-04-01
 *   file: RecvDisconnectUserVideo.cpp
 *   desc: 接收用户编辑聊天消息Task实现类
 */

#include <amf/AmfParser.h>
#include <json/json/json.h>
#include <common/CheckMomoryLeak.h>
#include "RecvDisconnectUserVideo.h"

RecvDisconnectUserVideo::RecvDisconnectUserVideo(void)
{
	m_listener = NULL;

	m_errType = LCC_ERR_FAIL;
	m_errMsg = "";

	m_userId1 = "";
	m_userId2 = "";
}

RecvDisconnectUserVideo::~RecvDisconnectUserVideo(void)
{
}

// 初始化
bool RecvDisconnectUserVideo::Init(ILiveChatClientListener* listener)
{
	bool result = false;
	if (NULL != listener)
	{
		m_listener = listener;
		result = true;
	}
	return result;
}
	
// 处理已接收数据
bool RecvDisconnectUserVideo::Handle(const TransportProtocol* tp)
{
	bool result = false;
		
//	AmfParser parser;
//	amf_object_handle root = parser.Decode((char*)tp->data, tp->GetDataLength());
//	if (!root.isnull()
//		&& root->type == DT_STRING)
//	{
//		m_fromId = root->strValue;
//		result = true;
//	}

	Json::Value root;
	Json::Reader reader;
	if( reader.parse((char*)tp->data, root, false) ) {
		if( root.isObject() ) {
			if( root["userId1"].isString() && root["userId2"].isString() ) {
				result = true;

				m_userId1 = root["userId1"].asString();
				m_userId2 = root["userId2"].asString();

				m_errType = LCC_ERR_SUCCESS;
			}
		}
	}

	// 通知listener
	if (NULL != m_listener 
		&& result) 
	{
		m_listener->OnRecvDisconnectUserVideo(m_client, GetSeq(), m_userId1, m_userId2, m_errType, m_errMsg);
	}
	
	return result;
}
	
// 获取待发送的数据，可先获取data长度，如：GetSendData(NULL, 0, dataLen);
bool RecvDisconnectUserVideo::GetSendData(void* data, unsigned int dataSize, unsigned int& dataLen)
{
	bool result = false;
	// 本协议没有返回
	return result;
}

// 获取待发送数据的类型
TASK_PROTOCOL_TYPE RecvDisconnectUserVideo::GetSendDataProtocolType()
{
	return JSON_PROTOCOL;
}
	
// 获取命令号
int RecvDisconnectUserVideo::GetCmdCode()
{
	return TCMD_RECVDISCONNECTUSERVIDEO;
}

// 是否需要等待回复。若false则发送后释放(delete掉)，否则发送后会被添加至待回复列表，收到回复后释放
bool RecvDisconnectUserVideo::IsWaitToRespond()
{
	return false;
}

// 获取处理结果
void RecvDisconnectUserVideo::GetHandleResult(LCC_ERR_TYPE& errType, string& errMsg)
{
	errType = m_errType;
	errMsg = m_errMsg;
}

// 未完成任务的断线通知
void RecvDisconnectUserVideo::OnDisconnect()
{
	// 不需要回调
}
