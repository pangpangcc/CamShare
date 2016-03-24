/*
 * Session.h
 *
 *  Created on: 2015年11月15日
 *      Author: kingsley
 */

#ifndef SESSION_H_
#define SESSION_H_

#include "Client.h"

#include <livechat/ILiveChatClient.h>
#include <request/IRequest.h>

typedef KSafeMap<int, IRequest*> RequestkMap;
class Session {
public:
	Session(Client* client, ILiveChatClient* livechat);
	virtual ~Session();

	/**
	 * 记录已经发送的请求
	 * @param seq livechat client 请求序列号
	 * @param request 请求实例
	 */
	void AddRequest(int seq, IRequest* request);

	/**
	 * 获得并删除已经返回的请求
	 * @param seq livechat client 请求序列号
	 * @return 请求实例
	 */
	IRequest* EraseRequest(int seq);

	/**
	 * 客户端句柄
	 */
	Client* client;

	ILiveChatClient* livechat;

private:

	/**
	 * 请求列表
	 */
	RequestkMap mRequestMap;
};

#endif /* SESSION_H_ */
