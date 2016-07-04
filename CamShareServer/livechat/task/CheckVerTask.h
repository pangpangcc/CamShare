/*
 * CheckVerTask.h
 *  Created on: 2016年3月11日
 *      Author: max
 */

#pragma once

#include "BaseTask.h"
#include <string>

using namespace std;

class CheckVerTask : public BaseTask
{
public:
	CheckVerTask(void);
	virtual ~CheckVerTask(void);

// ITask接口函数
public:
	// 处理已接收数据
	virtual bool Handle(const TransportProtocol* tp);
	// 获取待发送的数据，可先获取data长度，如：GetSendData(NULL, 0, dataLen);
	virtual bool GetSendData(void* data, unsigned int dataSize, unsigned int& dataLen);
	TASK_PROTOCOL_TYPE GetSendDataProtocolType();
	// 获取命令号
	virtual int GetCmdCode();
	// 获取处理结果
	virtual void GetHandleResult(LCC_ERR_TYPE& errType, string& errmsg);
	// 未完成任务的断线通知
	virtual void OnDisconnect();

public:
	// 初始化参数
	bool InitParam(const string& version);

private:
	string			m_version;	// 版本号

};
