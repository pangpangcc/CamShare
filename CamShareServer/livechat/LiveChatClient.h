/*
 * author: Samson.Fan
 *   date: 2015-03-19
 *   file: LiveChatClient.h
 *   desc: LiveChat客户端实现类
 */

#pragma once

#include "ILiveChatClient.h"
#include "ITaskManager.h"
#include "IThreadHandler.h"
#include "Counter.h"

class IAutoLock;
class CLiveChatClient : public ILiveChatClient
					  , private ITaskManagerListener
{
public:
	CLiveChatClient();
	virtual ~CLiveChatClient();

// ILiveChatClient接口函数
public:
	// 调用所有接口函数前需要先调用Init
	bool Init(const list<string>& svrIPs, unsigned int svrPort, ILiveChatClientListener* listener);
	// 判断是否无效seq
	bool IsInvalidSeq(int seq);
	// 获取计数器
	int GetSeq();
	// 是否已经连接服务器
	bool IsConnected();
	// 连接服务器
	bool ConnectServer(string siteId, string name);
	// 断开连接
	bool Disconnect();
	// 进入聊天室
	bool SendEnterConference(int seq, const string& serverId, const string& fromId, const string& toId, const string& key);
	// 发送消息到客户端
	bool SendMsg(int seq, const string& fromId, const string& toId, const string& msg);
	// 获取站点类型
	const string& GetSiteId();

private:
	// 连接服务器
	bool ConnectServer();
	// 检测版本号
	bool CheckVersionProc();
	// 启动发送心跳包线程
	void HearbeatThreadStart();

// ITaskManagerListener接口函数
private:
	// 连接成功回调
	virtual void OnConnect(bool success);
	// 连接失败回调(listUnsentTask：未发送/未回复的task列表)
	virtual void OnDisconnect(const TaskList& list);
	// 已完成交互的task
	virtual void OnTaskDone(ITask* task);

// 处理完成交互的task函数
private:
	void OnCheckVerTaskDone(ITask* task);

protected:
	static TH_RETURN_PARAM HearbeatThread(void* arg);
	void HearbeatProc();

private:
	bool			m_bInit;	// 初始化标志
	list<string>	m_svrIPs;	// 服务器ip列表
	string			m_svrIp;	// 当前连接的服务器IP
	unsigned int	m_svrPort;	// 服务器端口

	string		m_siteId;	// 站点Id

	Counter			m_seqCounter;	// seq计数器

	ITaskManager*	m_taskManager;	// 任务管理器
	ILiveChatClientListener*	m_listener;	// 监听器

	bool			m_isHearbeatThreadRun;	// 心跳线程运行标志
	IThreadHandler*		m_hearbeatThread;	// 心跳线程

	IAutoLock*		m_pConnectLock;		// 锁
	bool 			m_bConnectForbidden;

	string 			m_svrName;		// 应用名
	string 			m_connectName;	// 服务器连接名
	string 			m_groupName;	// 服务器组名
};
