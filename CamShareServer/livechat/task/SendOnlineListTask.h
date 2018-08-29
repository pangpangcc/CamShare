/*
 * SendOnlineListTask.h
 *  Created on: 2018年6月22日
 *      Author: max
 * 发送用户在线列表命令
 */

#pragma once

#include "BaseTask.h"
#include <string>

using namespace std;

class SendOnlineListTask : public BaseTask
{
public:
	SendOnlineListTask(void);
	virtual ~SendOnlineListTask(void);

// ITask接口函数
public:
	// 处理已接收数据
	bool Handle(const TransportProtocol* tp);
	// 获取待发送的数据，可先获取data长度，如：GetSendData(NULL, 0, dataLen);
	bool GetSendData(void* data, unsigned int dataSize, unsigned int& dataLen);
	// 获取命令号
	int GetCmdCode();
	// 获取处理结果
	void GetHandleResult(LCC_ERR_TYPE& errType, string& errmsg);
	// 未完成任务的断线通知
	void OnDisconnect();
	bool IsWaitToRespond();

public:
	// 初始化参数
	bool InitParam(const list<string>& userList);

private:
	list<string> mUserList;
};
