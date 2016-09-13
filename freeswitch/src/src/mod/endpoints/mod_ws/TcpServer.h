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

typedef struct Socket {
	Socket() {
		socket = NULL;
		pollfd = NULL;
		ip = NULL;
		port = 0;
		data = NULL;
	};
	switch_socket_t *socket;
	switch_pollfd_t *pollfd;

	const char *ip;
	switch_port_t port;

	void* data;
} Socket;

class TcpServerCallback {
public:
	virtual ~TcpServerCallback(){};
	virtual bool OnAceept(Socket* socket) = 0;
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
	void Disconnect(const Socket* socket);
	void Close(const Socket* socket);

	void IOThreadHandle();

private:
	TcpServerCallback* mpTcpServerCallback;

	bool mRunning;
	switch_memory_pool_t *mpPool;

	Socket* mpSocket;

	switch_thread_t *mpIOThread;
	switch_pollset_t *mpPollset;
	switch_pollfd_t *mpPollfd;

};

#endif /* SRC_MOD_ENDPOINTS_MOD_WS_TCPSERVER_H_ */
