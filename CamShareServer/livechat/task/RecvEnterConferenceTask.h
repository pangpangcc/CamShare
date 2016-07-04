/*
 * RecvEnterConferenceTask.h
 *  Created on: 2016年3月11日
 *      Author: max
 */

#pragma once

#include "BaseTask.h"
#include <string>

using namespace std;

class RecvEnterConferenceTask : public BaseTask
{
public:
	RecvEnterConferenceTask(void);
	virtual ~RecvEnterConferenceTask(void);

// ITask接口函数
public:
	// 处理已接收数据
	virtual bool Handle(const TransportProtocol* tp);
	// 获取待发送的数据，可先获取data长度，如：GetSendData(NULL, 0, dataLen);
	virtual bool GetSendData(void* data, unsigned int dataSize, unsigned int& dataLen);
	// 获取命令号
	virtual int GetCmdCode();
	// 是否需要等待回复。若false则发送后释放(delete掉)，否则发送后会被添加至待回复列表，收到回复后释放
	virtual bool IsWaitToRespond();
	// 获取处理结果
	virtual void GetHandleResult(LCC_ERR_TYPE& errType, string& errMsg);
	// 未完成任务的断线通知
	virtual void OnDisconnect();

private:
	string			mFromId;	// 发送用户Id（对方）
	string			mToId;
	bool			mbAuth;
};
