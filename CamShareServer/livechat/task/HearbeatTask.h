/*
 * HearbeatTask.h
 *  Created on: 2016年7月28日
 *      Author: max
 */

#pragma once

#include "BaseTask.h"
#include <string>

using namespace std;

class HearbeatTask : public BaseTask
{
public:
	HearbeatTask(void);
	virtual ~HearbeatTask(void);

// ITask接口函数
public:
	// 处理已接收数据
	bool Handle(const TransportProtocol* tp);
	// 获取待发送的数据，可先获取data长度，如：GetSendData(NULL, 0, dataLen);
	bool GetSendData(void* data, unsigned int dataSize, unsigned int& dataLen);
	// 获取待发送数据的类型
	TASK_PROTOCOL_TYPE GetSendDataProtocolType();
	// 获取命令号
	int GetCmdCode();
	// 获取处理结果
	void GetHandleResult(LCC_ERR_TYPE& errType, string& errMsg);
	// 未完成任务的断线通知
	void OnDisconnect();
	// 是否需要等待回复。若false则发送后释放(delete掉)，否则发送后会被添加至待回复列表，收到回复后释放
	bool IsWaitToRespond();
};
