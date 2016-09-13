/*
 * WebSocketServer.h
 *
 *  Created on: 2016年9月13日
 *      Author: Max.Chiu
 */

#ifndef SRC_MOD_ENDPOINTS_MOD_WS_WEBSOCKETSERVER_H_
#define SRC_MOD_ENDPOINTS_MOD_WS_WEBSOCKETSERVER_H_

#include "TcpServer.h"

struct Client;
class WebSocketServer : public TcpServerCallback {
public:
	WebSocketServer();
	virtual ~WebSocketServer();

	bool Load(switch_memory_pool_t *pool);
	void Shutdown();
	bool IsRuning();

	bool OnAceept(Socket* socket);
	void OnRecvEvent(Socket* socket);
	void OnDisconnect(Socket* socket);

	void HandleThreadHandle();

private:
	bool LoadConfig();

	void ClientHandle(Client* client);
	void ClientCloseIfNeed(Client* client);

	bool mRunning;

	switch_memory_pool_t *mpPool;


	TcpServer* mpTcpServer;

	switch_hash_t *mpClientHash;

	/**
	 * 处理队列和线程
	 */
	switch_queue_t* mpHandleQueue;
	int mThreadCount;
	switch_thread_t** mpHandleThreads;
};

#endif /* SRC_MOD_ENDPOINTS_MOD_WS_WEBSOCKETSERVER_H_ */
