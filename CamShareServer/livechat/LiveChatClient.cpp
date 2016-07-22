/*
 * author: Samson.Fan
 *   date: 2015-03-19
 *   file: LiveChatClient.cpp
 *   desc: LiveChat客户端实现类
 */

#include "LiveChatClient.h"
#include "TaskManager.h"
#include <common/KLog.h>
#include <common/CommonFunc.h>
#include <common/IAutoLock.h>

// task include
#include "task/CheckVerTask.h"
#include "task/SendEnterConferenceTask.h"
#include "task/SendMsgTask.h"

CLiveChatClient::CLiveChatClient()
{
	m_taskManager = NULL;
	m_listener = NULL;
	m_bInit = false;
	m_isHearbeatThreadRun = false;
	m_hearbeatThread = NULL;

	m_siteId = "";
	m_svrPort = -1;

	m_svrName = "";
	m_connectName = "";
	m_groupName = "";

	m_bConnectForbidden = false;
	m_pConnectLock = NULL;
	m_pConnectLock = IAutoLock::CreateAutoLock();
	if (NULL != m_pConnectLock) {
		m_pConnectLock->Init();
	}
}

CLiveChatClient::~CLiveChatClient()
{
	FileLog("LiveChatClient", "CLiveChatClient::~CLiveChatClient()");
	delete m_taskManager;
	m_taskManager = NULL;

	IAutoLock::ReleaseAutoLock(m_pConnectLock);
	m_pConnectLock = NULL;

	FileLog("LiveChatClient", "CLiveChatClient::~CLiveChatClient() end");
}

// ------------------------ ILiveChatClient接口函数 -------------------------
// 调用所有接口函数前需要先调用Init
bool CLiveChatClient::Init(const list<string>& svrIPs, unsigned int svrPort, ILiveChatClientListener* listener)
{
	bool result = false;

	// 初始化 TaskManager
	if (NULL == m_taskManager) {
		m_taskManager = new CTaskManager();
		if (NULL != m_taskManager) {
			result = m_taskManager->Init(svrIPs, svrPort, this, listener, this);
		}

		// 初始化 seq计数器
		if (result) {
			result = m_seqCounter.Init(1);
		}

		if (result) {
			// 所有初始化都成功，开始赋值
			m_listener = listener;
		}

		m_bInit = result;
	}

	return result;
}

// 判断是否无效seq
bool CLiveChatClient::IsInvalidSeq(int seq)
{
	return m_seqCounter.IsInvalidValue(seq);
}

// 获取计数器
int CLiveChatClient::GetSeq() {
	return m_seqCounter.GetAndIncrement();
}

// 是否已经连接服务器
bool CLiveChatClient::IsConnected() {
	return m_taskManager->IsConnected();
}

// 连接服站点
bool CLiveChatClient::ConnectServer(string siteId, string name) {
	bool result = false;

//	FileLog("LiveChatClient", "CLiveChatClient::ConnectServer( type : %d, name : %s ) begin", type, name.c_str());

	m_pConnectLock->Lock();
	if( !m_bConnectForbidden ) {
		m_pConnectLock->Unlock();

		FileLog("LiveChatClient", "CLiveChatClient::ConnectServer( siteId : %s, name : %s )", siteId.c_str(), name.c_str());
		if ( ConnectServer() )
		{
			m_bConnectForbidden = true;
			m_siteId = siteId;
			m_svrName = name;

			char tmp[64];
			// 服务器连接名[应用名_站点]
			sprintf(tmp, "%s_%s", m_svrName.c_str(), m_siteId.c_str());
			m_connectName = tmp;

			// 服务器组名[应用名]
			m_groupName = m_svrName;

			result = true;
		}

	} else {
		m_pConnectLock->Unlock();
	}

//	FileLog("LiveChatClient", "CLiveChatClient::ConnectServer( type : %d, name : %s ) end", type, name.c_str());

	return result;
}

// 连接服务器
bool CLiveChatClient::ConnectServer()
{
	bool result = false;

	FileLog("LiveChatClient", "CLiveChatClient::ConnectServer() begin");

	if (m_bInit) {
		if (NULL != m_taskManager) {
			if (m_taskManager->IsStart()) {
				m_taskManager->Stop();
			}
			result = m_taskManager->Start();
			FileLog("LiveChatClient", "CLiveChatClient::ConnectServer(), result: %d", result);
		}
	}

	FileLog("LiveChatClient", "CLiveChatClient::ConnectServer() end");

	return result;
}

// 注销站点
bool CLiveChatClient::Disconnect()
{
	bool result = false;

	FileLog("LiveChatClient", "CLiveChatClient::Disconnect() begin, m_taskManager:%p, siteId : %s", m_taskManager, m_siteId.c_str());

	if (m_taskManager) {
		FileLog("LiveChatClient", "CLiveChatClient::Disconnect() m_taskManager->Stop(), m_taskManager:%p, siteId : %s", m_taskManager, m_siteId.c_str());
		result = m_taskManager->Stop();

		if (result) {
			m_siteId = "";
		}
	}

	FileLog("LiveChatClient", "CLiveChatClient::Disconnect() end, siteId : %s", m_siteId.c_str());

	return result;
}

// 进入聊天室
bool CLiveChatClient::SendEnterConference(int seq, const string& serverId, const string& fromId, const string& toId, const string& key) {
	bool result = false;
	FileLog("LiveChatClient", "CLiveChatClient::SendEnterConference() begin");
	if (NULL != m_taskManager
		&& m_taskManager->IsStart())
	{
		SendEnterConferenceTask* task = new SendEnterConferenceTask();
		FileLog("LiveChatClient", "CLiveChatClient::SendEnterConference() task:%p, siteId : %s", task, m_siteId.c_str());
		if (NULL != task) {
			result = task->Init(this, m_listener);
			result = result && task->InitParam(m_connectName, fromId, toId, key);

			if (result) {
//				int seq = m_seqCounter.GetCount();
				task->SetServerId(serverId);
				task->SetSeq(seq);
				result = m_taskManager->HandleRequestTask(task);
			}
		}
		FileLog("LiveChatClient", "CLiveChatClient::SendEnterConference() task:%p end, siteId : %s", task, m_siteId.c_str());
	}
	FileLog("LiveChatClient", "CLiveChatClient::SendEnterConference() end, siteId : %s", m_siteId.c_str());
	return result;
}

// 发送消息到客户端
bool CLiveChatClient::SendMsg(int seq, const string& fromId, const string& toId, const string& msg) {
	bool result = false;
	FileLog("LiveChatClient", "CLiveChatClient::SendMsg() begin");
	if (NULL != m_taskManager
		&& m_taskManager->IsStart())
	{
		SendMsgTask* task = new SendMsgTask();
		FileLog("LiveChatClient", "CLiveChatClient::SendMsg() task:%p, siteId : %s", task, m_siteId.c_str());
		if (NULL != task) {
			result = task->Init(this, m_listener);
			result = result && task->InitParam(fromId, toId, msg);

			if (result) {
//				int seq = m_seqCounter.GetCount();
				task->SetSeq(seq);
				result = m_taskManager->HandleRequestTask(task);
			}
		}
		FileLog("LiveChatClient", "CLiveChatClient::SendMsg() end, task:%p, siteId : %s", task, m_siteId.c_str());
	}
	FileLog("LiveChatClient", "CLiveChatClient::SendMsg() end, siteId : %s", m_siteId.c_str());
	return result;
}

const string& CLiveChatClient::GetSiteId() {
	return m_siteId;
}

// ------------------------ ITaskManagerListener接口函数 -------------------------
// 连接成功回调
void CLiveChatClient::OnConnect(bool success)
{
	FileLog("LiveChatClient", "CLiveChatClient::OnConnect() success: %d, siteId : %s", success, m_siteId.c_str());

	if (success) {
		FileLog("LiveChatClient", "CLiveChatClient::OnConnect) CheckVersionProc(), siteId : %s", m_siteId.c_str());
		// 连接服务器成功，检测版本号
		CheckVersionProc();
//		// 启动发送心跳包线程
//		HearbeatThreadStart();
	}
	else {
		FileLog("LiveChatClient", "CLiveChatClient::OnConnect() LCC_ERR_CONNECTFAIL, m_listener:%p, siteId : %s", m_listener, m_siteId.c_str());
		m_listener->OnConnect(this, LCC_ERR_CONNECTFAIL, "");
	}

	m_pConnectLock->Lock();
	m_bConnectForbidden = success;
	m_pConnectLock->Unlock();

	FileLog("LiveChatClient", "CLiveChatClient::OnConnect() end, siteId : %s", m_siteId.c_str());
}

// 连接失败回调(listUnsentTask：未发送的task列表)
void CLiveChatClient::OnDisconnect(const TaskList& listUnsentTask)
{
	TaskList::const_iterator iter;
	for (iter = listUnsentTask.begin();
		iter != listUnsentTask.end();
		iter++)
	{
		(*iter)->OnDisconnect();
	}

//	// 停止心跳线程
//	if (NULL != m_hearbeatThread) {
//		m_isHearbeatThreadRun = false;
//		m_hearbeatThread->WaitAndStop();
//		IThreadHandler::ReleaseThreadHandler(m_hearbeatThread);
//		m_hearbeatThread = NULL;
//	}

	m_listener->OnDisconnect(this, LCC_ERR_CONNECTFAIL, "");

	m_pConnectLock->Lock();
	m_bConnectForbidden = false;
	m_pConnectLock->Unlock();
}

// 已完成交互的task
void CLiveChatClient::OnTaskDone(ITask* task)
{
	if (NULL != task) {
		// 需要LiveChatClient处理后续相关业务逻辑的task（如：检测版本）
		switch (task->GetCmdCode()) {
		case TCMD_CHECKVER:
			OnCheckVerTaskDone(task);
			break;
		}
	}
}

// 检测版本已经完成
void CLiveChatClient::OnCheckVerTaskDone(ITask* task)
{
	LCC_ERR_TYPE errType = LCC_ERR_FAIL;
	string errMsg("");
	task->GetHandleResult(errType, errMsg);
	if (LCC_ERR_SUCCESS == errType) {
		// 检测版本成功，进行登录操作
		m_listener->OnConnect(this, LCC_ERR_SUCCESS, errMsg);
	}
	else {
		// 检测版本失败，回调给上层
		m_listener->OnConnect(this, errType, errMsg);
	}
}

// ------------------------ 操作处理函数 ------------------------------
// 检测版本号
bool CLiveChatClient::CheckVersionProc()
{
	bool result = false;
	CheckVerTask* checkVerTask = new CheckVerTask();
	if (NULL != checkVerTask) {
		checkVerTask->Init(this, m_listener);

		char siteId[64];
		// 服务器连接名[应用名_站点]/站点/服务器组名[应用名]
		sprintf(siteId, "1.1.0.0X%sX%sX%sX7", m_connectName.c_str(), m_siteId.c_str(), m_groupName.c_str());
		checkVerTask->InitParam(siteId);

		m_seqCounter.GetAndIncrement();
		checkVerTask->SetSeq(0);
		result = m_taskManager->HandleRequestTask(checkVerTask);
	}
	return result;
}
