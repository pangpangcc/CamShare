/*
 * RecvGetOnlineListTask.cpp
 *  Created on: 2016年3月11日
 *      Author: max
 */

#include "RecvGetOnlineListTask.h"

#include <amf/AmfParser.h>
#include <json/json/json.h>
#include <common/KLog.h>

RecvGetOnlineListTask::RecvGetOnlineListTask(void) {
}

RecvGetOnlineListTask::~RecvGetOnlineListTask(void) {
}
	
// 处理已接收数据
bool RecvGetOnlineListTask::Handle(const TransportProtocol* tp) {
	bool result = true;

	m_errType = LCC_ERR_SUCCESS;
	m_errMsg = "";

	// 打log
	FileLog("LiveChatClient", "RecvGetOnlineListTask::Handle() "
			"cmd:%d, "
			"errType:%d, "
			"errMsg:%s",
			GetCmdCode(),
			m_errType,
			m_errMsg.c_str()
			);

	// 通知listener
	if ( NULL != m_listener ) {
		m_listener->OnRecvGetOnlineList(m_client, GetSeq(), m_errType, m_errMsg);
	}
	
	return result;
}
	
// 获取待发送的数据，可先获取data长度，如：GetSendData(NULL, 0, dataLen);
bool RecvGetOnlineListTask::GetSendData(void* data, unsigned int dataSize, unsigned int& dataLen)
{
	bool result = false;

	// 本协议没有返回
	return result;
}

// 获取命令号
int RecvGetOnlineListTask::GetCmdCode()
{
	return TCMD_RECV_GET_ONLINE_LIST;
}

// 是否需要等待回复。若false则发送后释放(delete掉)，否则发送后会被添加至待回复列表，收到回复后释放
bool RecvGetOnlineListTask::IsWaitToRespond()
{
	return false;
}

// 获取处理结果
void RecvGetOnlineListTask::GetHandleResult(LCC_ERR_TYPE& errType, string& errMsg)
{
	errType = m_errType;
	errMsg = m_errMsg;
}

// 未完成任务的断线通知
void RecvGetOnlineListTask::OnDisconnect()
{
	// 不需要回调
}
