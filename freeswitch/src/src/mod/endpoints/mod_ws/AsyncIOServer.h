/*
 * AsyncIOServer.h
 *
 *  Created on: 2016年9月13日
 *      Author: Max.Chiu
 */

#ifndef SRC_MOD_ENDPOINTS_MOD_WS_ASYNCIOSERVER_H_
#define SRC_MOD_ENDPOINTS_MOD_WS_ASYNCIOSERVER_H_

#include "TcpServer.h"
#include "IDataParser.h"
#include "Client.h"

class AsyncIOServerCallback {
public:
	virtual ~AsyncIOServerCallback(){};
	virtual void OnAccept(Client *client) = 0;
	virtual void OnDisconnect(Client* client) = 0;
};

class AsyncIOServer : public TcpServerCallback {
public:
	AsyncIOServer();
	virtual ~AsyncIOServer();

	void SetAsyncIOServerCallback(AsyncIOServerCallback* callback);
	bool Start(
			switch_memory_pool_t *pool,
			int iThreadCount = 4,
			const char *ip = "0.0.0.0",
			switch_port_t port = 8080
			);
	void Stop();
	bool IsRuning();

	bool Send(Client* client, const char* buffer, switch_size_t* len);
	void Disconnect(Client* client);

	/**
	 * TcpServerCallback Implement
	 */
	bool OnAccept(Socket* socket);
	void OnRecvEvent(Socket* socket);
	void OnDisconnect(Socket* socket);

	/**
	 * Receive Data Thread Handler
	 */
	void RecvHandleThread();

private:
	void RecvHandle(Client* client);
	bool ClientCloseIfNeed(Client* client);

	bool mRunning;

	AsyncIOServerCallback* mpAsyncIOServerCallback;

	switch_memory_pool_t *mpPool;

	TcpServer mTcpServer;

	/**
	 * 接收处理队列和线程
	 */
	switch_queue_t* mpHandleQueue;
	int mThreadCount;
	switch_thread_t** mpHandleThreads;

	/**
	 * 发送处理队列和线程
	 */
	switch_queue_t* mpSendQueue;
	switch_thread_t* mpSendThread;
};

#endif /* SRC_MOD_ENDPOINTS_MOD_WS_ASYNCIOSERVER_H_ */
