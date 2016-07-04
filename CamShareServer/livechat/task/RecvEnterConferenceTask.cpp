/*
 * RecvEnterConferenceTask.cpp
 *  Created on: 2016年3月11日
 *      Author: max
 */

#include "RecvEnterConferenceTask.h"

#include <amf/AmfParser.h>
#include <json/json/json.h>
#include <common/KLog.h>

RecvEnterConferenceTask::RecvEnterConferenceTask(void) {
	mFromId = "";
	mToId = "";
	mbAuth = false;
}

RecvEnterConferenceTask::~RecvEnterConferenceTask(void) {
}
	
// 处理已接收数据
bool RecvEnterConferenceTask::Handle(const TransportProtocol* tp) {
	bool result = false;
		
	AmfParser parser;
	amf_object_handle root = parser.Decode((char*)tp->data, tp->GetDataLength());
	if ( !root.isnull() && root->type == DT_OBJECT ) {

		// mFromId
		amf_object_handle fromIdObject = root->get_child("fromId");
		if ( !fromIdObject.isnull() && fromIdObject->type == DT_STRING ) {
			mFromId = fromIdObject->strValue;
		}

		// mToId
		amf_object_handle toIdObject = root->get_child("toId");
		if ( !toIdObject.isnull() && toIdObject->type == DT_STRING ) {
			mToId = toIdObject->strValue;
		}

		// mbAuth
		amf_object_handle authObject = root->get_child("auth");
		if ( !authObject.isnull() && authObject->type == DT_TRUE ) {
			mbAuth = true;
		}

		if( mFromId.length() > 0 && mToId.length() > 0 ) {
			result = true;
			m_errType = LCC_ERR_SUCCESS;
		}

	}

	// 协议解析失败
	if (!result) {
		m_errType = LCC_ERR_PROTOCOLFAIL;
		m_errMsg = "";
	}

//	Json::Value root;
//	Json::Reader reader;
//	if( reader.parse((char*)tp->data, root, false) ) {
//		if( root.isObject() ) {
//			if( root["userId1"].isString() && root["userId2"].isString() ) {
//				result = true;
//
//				m_userId1 = root["userId1"].asString();
//				m_userId2 = root["userId2"].asString();
//
//				m_errType = LCC_ERR_SUCCESS;
//			}
//		}
//	}

	// 打log
	FileLog("LiveChatClient", "RecvEnterConferenceTask::Handle() "
			"errType:%d, "
			"errMsg:%s, "
			"mFromId:%s, "
			"mToId:%s, "
			"mbAuth:%s",
			m_errType,
			m_errMsg.c_str(),
			mFromId.c_str(),
			mToId.c_str(),
			mbAuth?"true":"false"
			);

	// 通知listener
	if ( NULL != m_listener ) {
		m_listener->OnRecvEnterConference(m_client, GetSeq(), mFromId, mToId, mbAuth, m_errType, m_errMsg);
	}
	
	return result;
}
	
// 获取待发送的数据，可先获取data长度，如：GetSendData(NULL, 0, dataLen);
bool RecvEnterConferenceTask::GetSendData(void* data, unsigned int dataSize, unsigned int& dataLen)
{
	bool result = false;

	// 本协议没有返回
	return result;
}

// 获取命令号
int RecvEnterConferenceTask::GetCmdCode()
{
	return TCMD_RECVENTERCONFERENCE;
}

// 是否需要等待回复。若false则发送后释放(delete掉)，否则发送后会被添加至待回复列表，收到回复后释放
bool RecvEnterConferenceTask::IsWaitToRespond()
{
	return false;
}

// 获取处理结果
void RecvEnterConferenceTask::GetHandleResult(LCC_ERR_TYPE& errType, string& errMsg)
{
	errType = m_errType;
	errMsg = m_errMsg;
}

// 未完成任务的断线通知
void RecvEnterConferenceTask::OnDisconnect()
{
	// 不需要回调
}
