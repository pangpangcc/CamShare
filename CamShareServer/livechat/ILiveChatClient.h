/*
 * author: Samson.Fan
 *   date: 2015-03-19
 *   file: ILiveChatClient.h
 *   desc: LiveChat客户端接口类
 */

#pragma once

#include "ILiveChatClientDef.h"

class ILiveChatClient;
// LiveChat客户端监听接口类
class ILiveChatClientListener
{
public:
	ILiveChatClientListener() {};
	virtual ~ILiveChatClientListener() {}

public:
	// 客户端主动请求
	// 回调函数的参数在 err 之前的为请求参数，在 errmsg 之后为返回参数
	virtual void OnConnect(ILiveChatClient* client, LCC_ERR_TYPE err, const string& errmsg) = 0;
	virtual void OnDisconnect(ILiveChatClient* client, LCC_ERR_TYPE err, const string& errmsg) = 0;
	virtual void OnSendEnterConference(ILiveChatClient* client, int seq, const string& fromId, const string& toId, const string& key, LCC_ERR_TYPE err, const string& errmsg) = 0;
	// 服务器主动请求
	virtual void OnRecvEnterConference(ILiveChatClient* client, int seq, const string& fromId, const string& toId, const string& key, bool bAuth, LCC_ERR_TYPE err, const string& errmsg) = 0;
	virtual void OnRecvKickUserFromConference(ILiveChatClient* client, int seq, const string& fromId, const string& toId, LCC_ERR_TYPE err, const string& errmsg) = 0;
	virtual void OnRecvGetOnlineList(ILiveChatClient* client, int seq, LCC_ERR_TYPE err, const string& errmsg) = 0;
};

// LiveChat客户端接口类
class ILiveChatClient
{
public:
	static ILiveChatClient* CreateClient();
	static void ReleaseClient(ILiveChatClient* client);

public:
	ILiveChatClient(void) {};
	virtual ~ILiveChatClient(void) {};

public:
	// 调用所有接口函数前需要先调用Init
	virtual bool Init(const list<string>& svrIPs, unsigned int svrPort, ILiveChatClientListener* listener) = 0;
	// 判断是否无效seq
	virtual bool IsInvalidSeq(int seq) = 0;
	// 获取计数器
	virtual int GetSeq() = 0;
	// 是否已经连接服务器
	virtual bool IsConnected() = 0;
	// 连接服务器
	virtual bool ConnectServer(string siteId, string name) = 0;
	// 断开服务器连接
	virtual bool Disconnect() = 0;
	// 进入聊天室
	virtual bool SendEnterConference(int seq, const string& serverId, const string& fromId, const string& toId, const string& key, ENTERCONFERENCETYPE type) = 0;
	// 发送消息到客户端
	virtual bool SendMsg(int seq, const string& fromId, const string& toId, const string& msg) = 0;
	// 发送用户在线状态改变
	virtual bool SendOnlineStatus(int seq, const string& userId, bool login) = 0;
	// 发送用户在线列表
	virtual bool SendOnlineList(int seq, const list<string>& userList) = 0;

public:
	// 获取站点类型
	virtual const string& GetSiteId() = 0;
};
