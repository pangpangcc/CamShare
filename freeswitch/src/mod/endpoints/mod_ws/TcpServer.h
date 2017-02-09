/*
 * TcpServer.h
 *
 *  Created on: 2016年9月12日
 *      Author: Max.Chiu
 */

#ifndef SRC_MOD_ENDPOINTS_MOD_WS_TCPSERVER_H_
#define SRC_MOD_ENDPOINTS_MOD_WS_TCPSERVER_H_

#include <switch.h>
#include <switch_apr.h>

#include "Socket.h"

class TcpServerCallback {
public:
	virtual ~TcpServerCallback(){};
	virtual bool OnAccept(Socket* socket) = 0;
	virtual void OnRecvEvent(Socket* socket) = 0;
	virtual void OnDisconnect(Socket* socket) = 0;
};

class TcpServer {
public:
	TcpServer();
	virtual ~TcpServer();

	void SetTcpServerCallback(TcpServerCallback* callback);

	bool Start(switch_memory_pool_t *pool, const char *ip, switch_port_t port);
	void Stop();
	bool IsRuning();
	switch_status_t Read(Socket* socket, const char *data, switch_size_t* len);
	bool Send(Socket* socket, const char *data, switch_size_t* len);
	void Disconnect(Socket* socket);
	void Close(Socket* socket);

	void IOHandleThread();

private:
	TcpServerCallback* mpTcpServerCallback;

	bool mRunning;
	switch_memory_pool_t *mpPool;

	Socket* mpSocket;

	/**
	 * 同步锁
	 */
	switch_mutex_t *mpSocketMutex;

	switch_thread_t *mpIOThread;
	switch_pollset_t *mpPollset;
	switch_pollfd_t *mpPollfd;

};

#endif /* SRC_MOD_ENDPOINTS_MOD_WS_TCPSERVER_H_ */
