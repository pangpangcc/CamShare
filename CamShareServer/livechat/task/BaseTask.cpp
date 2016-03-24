/*
 * BaseTask.cpp
 *
 *  Created on: 2016年3月11日
 *      Author: max
 */

#include "BaseTask.h"

BaseTask::BaseTask() {
	// TODO Auto-generated constructor stub
	m_listener = NULL;
	m_client = NULL;

	m_seq = 0;
	m_errType = LCC_ERR_FAIL;
	m_errMsg = "";

}

BaseTask::~BaseTask() {
	// TODO Auto-generated destructor stub
}

// 初始化
bool BaseTask::Init(ILiveChatClient* client, ILiveChatClientListener* listener)
{
	bool result = false;
	if (NULL != listener)
	{
		m_listener = listener;
		m_client = client;
		result = true;
	}
	return result;
}

// 设置seq
void BaseTask::SetSeq(unsigned int seq)
{
	m_seq = seq;
}

// 获取seq
unsigned int BaseTask::GetSeq()
{
	return m_seq;
}

// 是否需要等待回复。若false则发送后释放(delete掉)，否则发送后会被添加至待回复列表，收到回复后释放
bool BaseTask::IsWaitToRespond()
{
	return true;
}
