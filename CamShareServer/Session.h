/*
 * Session.h
 *
 *  Created on: 2015年11月15日
 *      Author: kingsley
 */

#ifndef SESSION_H_
#define SESSION_H_

#include <common/TimeProc.hpp>
#include <common/KThread.h>
#include <common/KSafeList.h>
#include <common/KSafeMap.h>

#include <livechat/ILiveChatClient.h>
#include <request/IRequest.h>

class CheckTimeoutRunnable;
typedef struct RequestItem {
	RequestItem(
			string identify,
			IRequest* request
			) {
		this->identify = identify;
		this->request = request;
		timestamp = GetTickCount();
	}
	string identify;
	IRequest* request;
	unsigned int timestamp;
} RequestItem;

typedef KSafeList<RequestItem*> RequestItemList;
typedef KSafeMap<string, IRequest*> RequestkMap;
class Session {
public:
	Session(ILiveChatClient* livechat);
	virtual ~Session();

	/**
	 * 记录已经发送的请求
	 * @param identify 唯一标识
	 * @param request 请求实例
	 */
	bool AddRequest(const string& identify, IRequest* request);

	/**
	 * 获得并删除已经返回的请求
	 * @param identify 唯一标识
	 * @return 请求实例
	 */
	IRequest* EraseRequest(const string& identify);

	/**
	 * livechat句柄
	 */
	ILiveChatClient* livechat;

	void CheckTimeoutRunnableHandle();

	/**
	 * 任务请求超时(毫秒)
	 */
	unsigned int timeout;

private:
	/**
	 * 任务列表
	 */
	RequestkMap mRequestMap;

	/**
	 * 任务检测列表
	 */
	RequestItemList mRequestItemList;

	/**
	 * 检测超时线程
	 */
	CheckTimeoutRunnable* mpCheckTimeoutRunnable;
	KThread* mpCheckTimeoutThread;

	bool mIsRunning;
};

#endif /* SESSION_H_ */
