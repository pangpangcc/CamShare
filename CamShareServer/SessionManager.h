/*
 * SessionManager.h
 *
 *  Created on: 2016年4月21日
 *      Author: max
 */

#ifndef SESSIONMANAGER_H_
#define SESSIONMANAGER_H_

#include "Session.h"

#include <request/IRequest.h>
#include <respond/IRespond.h>

#include <livechat/ILiveChatClient.h>

// livechat client -> session
typedef KSafeMap<ILiveChatClient*, Session*> LiveChat2SessionMap;

class SessionManagerListener {
public:
	virtual ~SessionManagerListener() {};
};

class SessionManager {
public:
	SessionManager();
	virtual ~SessionManager();

	/**
	 * 设置事件监听
	 */
	void SetSessionManagerListener(SessionManagerListener* listener);

	/**
	 * 设置任务超时
	 * @param 	超时时间(秒)
	 */
	void SetRequestTimeout(unsigned int timeout);

	/**
	 * 会话交互(Session), 客户端发起会话
	 * @param livechat		LiveChat client
	 * @param request		请求实例
	 * @param seq			LiveChat client seq
	 * @return true:需要Finish / false:不需要Finish
	 */
	bool StartSessionBySeq(
			ILiveChatClient* livechat,
			IRequest* request,
			int seq
			);

	/**
	 * 会话交互(Session), LiveChat返回会话
	 * @param livechat		LiveChat client
	 * @param seq			LiveChat client seq
	 * @return 请求实例
	 */
	IRequest* FinishSessionBySeq(
			ILiveChatClient* livechat,
			int seq
			);

	/**
	 * 会话交互(Session), 客户端发起会话
	 * @param livechat			LiveChat client
	 * @param request			请求实例
	 * @param customIdentify	自定义 identify 唯一标识
	 * @return true:需要Finish / false:不需要Finish
	 */
	bool StartSessionByCustomIdentify(
			ILiveChatClient* livechat,
			IRequest* request,
			const string& customIdentify
			);

	/**
	 * 会话交互(Session), LiveChat返回会话
	 * @param livechat			LiveChat client
	 * @param customIdentify	自定义 identify 唯一标识
	 * @return 请求实例
	 */
	IRequest* FinishSessionByCustomIdentify(
			ILiveChatClient* livechat,
			const string& customIdentify
			);

	/**
	 * 外部服务(LiveChat), 关闭会话
	 * @param livechat		LiveChat client
	 */
	bool CloseSessionByLiveChat(ILiveChatClient* livechat);

private:
	/**
	 * 会话交互(Session), 客户端发起会话
	 * @param livechat		LiveChat client
	 * @param request		请求实例
	 * @param identify		identify 唯一标识
	 * @return true:需要Finish / false:不需要Finish
	 */
	bool StartSession(
			ILiveChatClient* livechat,
			IRequest* request,
			const string& identify
			);

	/**
	 * 会话交互(Session), LiveChat返回会话
	 * @param livechat		LiveChat client
	 * @param seq			identify 唯一标识
	 * @return 请求实例
	 */
	IRequest* FinishSession(
			ILiveChatClient* livechat,
			const string& identify
			);

	SessionManagerListener* mpSessionManagerListener;

//	/**
//	 * 内部服务(HTTP), 对应会话
//	 */
//	Client2SessionMap mClient2SessionMap;

	/**
	 * 外部服务(LiveChat), 对应会话
	 */
	LiveChat2SessionMap mLiveChat2SessionMap;

	/**
	 * 任务请求超时(秒)
	 */
	unsigned int miTimeout;
};

#endif /* SESSIONMANAGER_H_ */
