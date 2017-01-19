/*
 * SessionManager.cpp
 *
 *  Created on: 2016年4月21日
 *      Author: max
 */

#include "SessionManager.h"

#include <common/LogManager.h>

#define SEQ_TYPE		"SEQ"
#define CUSTOM_TYPE		"CUSTOM"

SessionManager::SessionManager() {
	// TODO Auto-generated constructor stub
	miTimeout = 0;
	mpSessionManagerListener = NULL;
}

SessionManager::~SessionManager() {
	// TODO Auto-generated destructor stub
}

void SessionManager::SetSessionManagerListener(SessionManagerListener* listener) {
	mpSessionManagerListener = listener;
}

void SessionManager::SetRequestTimeout(unsigned int timeout) {
	miTimeout = timeout;
}

bool SessionManager::StartSessionBySeq(
		ILiveChatClient* livechat,
		IRequest* request,
		int seq
		) {

	char identify[1024];
	sprintf(identify, "%s_%d", SEQ_TYPE, seq);

	return StartSession(livechat, request, identify);
}

IRequest* SessionManager::FinishSessionBySeq(
		ILiveChatClient* livechat,
		int seq
		) {
	char identify[1024];
	sprintf(identify, "%s_%d", SEQ_TYPE, seq);

	return FinishSession(livechat, identify);
}

bool SessionManager::StartSessionByCustomIdentify(
		ILiveChatClient* livechat,
		IRequest* request,
		const string& customIdentify
		) {
	char identify[1024];
	sprintf(identify, "%s_%s", CUSTOM_TYPE, customIdentify.c_str());

	return StartSession(livechat, request, identify);
}

IRequest* SessionManager::FinishSessionByCustomIdentify(
		ILiveChatClient* livechat,
		const string& customIdentify
		) {
	char identify[1024];
	sprintf(identify, "%s_%s", CUSTOM_TYPE, customIdentify.c_str());

	return FinishSession(livechat, identify);
}

bool SessionManager::StartSession(
		ILiveChatClient* livechat,
		IRequest* request,
		const string& identify
		) {
	bool bFlag = true;

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"SessionManager::StartSession( "
			"[会话交互(Session), 创建客户端到LiveChat会话], "
			"livechat : %p, "
			"request : %p "
			")",
			livechat,
			request
			);

	if( !livechat || !request ) {
		return false;
	}

	bool bRecord = false;
	// 是否需要等待返回的请求
	bool bNeedReturn = request->IsNeedReturn();
	if( bNeedReturn ) {
		mLiveChat2SessionMap.Lock();
		Session* session = NULL;
		LiveChat2SessionMap::iterator itr = mLiveChat2SessionMap.Find(livechat);
		if( itr != mLiveChat2SessionMap.End() ) {
			session = itr->second;
			LogManager::GetLogManager()->Log(
					LOG_MSG,
					"SessionManager::StartSession( "
					"[会话交互(Session), Client -> LiveChat, 继续会话], "
					"livechat : %p, "
					"session : %p, "
					"request : %p, "
					"identify : %s "
					")",
					livechat,
					session,
					request,
					identify.c_str()
					);

		} else {
			session = new Session(livechat);
			session->timeout = 1000 * miTimeout;
			mLiveChat2SessionMap.Insert(livechat, session);

			LogManager::GetLogManager()->Log(
					LOG_MSG,
					"SessionManager::StartSession( "
					"[会话交互(Session), Client -> LiveChat, 开始新会话] "
					"livechat : %p, "
					"session : %p, "
					"request : %p, "
					"identify : %s "
					")",
					livechat,
					session,
					request,
					identify.c_str()
					);
		}

		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"SessionManager::StartSession( "
				"[会话交互(Session), Client -> LiveChat, 插入任务到会话], "
				"livechat : %p, "
				"session : %p, "
				"request : %p, "
				"identify : %s "
				")",
				livechat,
				session,
				request,
				identify.c_str()
				);

		// 增加到等待返回的列表
		if( session->AddRequest(identify, request) ) {
			bRecord = true;

		} else {
			LogManager::GetLogManager()->Log(
					LOG_WARNING,
					"SessionManager::StartSession( "
					"[会话交互(Session), Client -> LiveChat, 插入任务到会话, 任务已经存在, 丢弃], "
					"livechat : %p, "
					"session : %p, "
					"request : %p, "
					"identify : %s "
					")",
					livechat,
					session,
					request,
					identify.c_str()
					);
		}


		mLiveChat2SessionMap.Unlock();
	}

	/**
	 * 开始任务
	 * 不需要等待返回的请求, 或者不是重复的请求
	 */
	if( !bNeedReturn || bRecord ) {
		bFlag = request->StartRequest();
	}

	/**
	 * 清除任务
	 * 启动失败, 并且需要返回, 并且不是重复
	 */
	if( !bFlag ) {
		if( bNeedReturn && bRecord ) {
			// 需要返回, 并且不是重复
			mLiveChat2SessionMap.Lock();
			Session* session = NULL;
			LiveChat2SessionMap::iterator itr = mLiveChat2SessionMap.Find(livechat);
			if( itr != mLiveChat2SessionMap.End() ) {
				session = itr->second;
				session->EraseRequest(identify);
			}
			mLiveChat2SessionMap.Unlock();
		}

		// 释放请求
		delete request;
	}

	return bFlag;
}

IRequest* SessionManager::FinishSession(
		ILiveChatClient* livechat,
		const string& identify
		) {
	IRequest* request = NULL;

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"SessionManager::FinishSession( "
			"[会话交互(Session), LiveChat -> Client, 返回响应到客户端], "
			"livechat : %p, "
			"identify : %s "
			")",
			livechat,
			identify.c_str()
			);

	Session* session = NULL;
	mLiveChat2SessionMap.Lock();
	LiveChat2SessionMap::iterator itr = mLiveChat2SessionMap.Find(livechat);
	if( itr != mLiveChat2SessionMap.End() ) {
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"SessionManager::FinishSession( "
				"[会话交互(Session), LiveChat -> Client, 返回响应到客户端, 找到对应会话], "
				"livechat : %p, "
				"identify : %s "
				")",
				livechat,
				identify.c_str()
				);

		session = itr->second;
		if( session != NULL ) {
			request = session->EraseRequest(identify);
			if( request != NULL ) {
				LogManager::GetLogManager()->Log(
						LOG_MSG,
						"SessionManager::FinishSession( "
						"[会话交互(Session), LiveChat -> Client, 返回响应到客户端, 找到对应请求], "
						"livechat : %p, "
						"identify : %s, "
						"request : %p "
						")",
						livechat,
						identify.c_str(),
						request
						);

				// 标记完成任务, 执行成功
				if( request ) {
					request->FinisRequest(true);
				}

			} else {
				LogManager::GetLogManager()->Log(
						LOG_MSG,
						"SessionManager::FinishSession( "
						"[会话交互(Session), LiveChat -> Client, 返回响应到客户端, 找不到对应请求], "
						"livechat : %p, "
						"identify : %s "
						")",
						livechat,
						identify.c_str()
						);
			}

			// 不关闭会话, 等待livechat断开连接才关闭
		}

	}
	mLiveChat2SessionMap.Unlock();

	return request;
}

bool SessionManager::CloseSessionByLiveChat(ILiveChatClient* livechat) {
	bool bFlag = true;

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"SessionManager::CloseSessionByLiveChat( "
			"[外部服务(LiveChat), 关闭所有会话], "
			"livechat : %p "
			")",
			livechat
			);

	// 断开该站所有客户端连接
	mLiveChat2SessionMap.Lock();
	for(LiveChat2SessionMap::iterator itr = mLiveChat2SessionMap.Begin(); itr != mLiveChat2SessionMap.End();) {
		Session* session = itr->second;
		if( session != NULL ) {
			if( session->livechat == livechat ) {
				mLiveChat2SessionMap.Erase(itr++);
				delete session;

			} else {
				itr++;
			}
		}

	}
	mLiveChat2SessionMap.Unlock();

	return bFlag;
}
