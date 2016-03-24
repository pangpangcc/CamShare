/*
 * BaseTask.h
 *
 *  Created on: 2016年3月11日
 *      Author: max
 */

#ifndef LIVECHAT_TASK_BASETASK_H_
#define LIVECHAT_TASK_BASETASK_H_

#include "ITask.h"

class BaseTask : public ITask {
public:
	BaseTask();
	virtual ~BaseTask();

	// 初始化
	bool Init(ILiveChatClient* client, ILiveChatClientListener* listener);

	// 设置seq
	void SetSeq(unsigned int seq);

	// 获取seq
	unsigned int GetSeq();

	// 是否需要等待回复
	bool IsWaitToRespond();

protected:
	ILiveChatClient*			m_client;
	ILiveChatClientListener*	m_listener;

	LCC_ERR_TYPE	m_errType;	// 服务器返回的处理结果
	string			m_errMsg;	// 服务器返回的结果描述

private:
	unsigned int	m_seq;

};

#endif /* LIVECHAT_TASK_BASETASK_H_ */
