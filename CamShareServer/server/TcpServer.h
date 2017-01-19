/*
 * TcpServer.h
 *
 *  Created on: 2016年9月12日
 *      Author: Max.Chiu
 */

#ifndef TCPSERVER_H_
#define TCPSERVER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/syscall.h>

#include <netinet/in.h>
#include <netinet/tcp.h>

#include <common/KThread.h>
#include <common/KSafeList.h>
#include <common/LogManager.h>

#include "Socket.h"

class TcpServerCallback {
public:
	virtual ~TcpServerCallback(){};
	virtual bool OnAccept(Socket* socket) = 0;
	virtual void OnRecvEvent(Socket* socket) = 0;
	virtual void OnDisconnect(Socket* socket) = 0;
};

typedef KSafeList<ev_io *> WatcherList;
class IORunnable;

class TcpServer {
public:
	TcpServer();
	virtual ~TcpServer();

	void SetTcpServerCallback(TcpServerCallback* callback);

	bool Start(int port = 9201, int maxConnection = 1000, const char *ip = "0.0.0.0");
	void Stop();
	bool IsRunning();

	SocketStatus Read(Socket* socket, const char *data, int &len);
	bool Send(Socket* socket, const char *data, int &len);
	void Disconnect(Socket* socket);
	void Close(Socket* socket);

	void IOHandleThread();
	void IOHandleAccept(ev_io *w, int revents);
	void IOHandleRecv(ev_io *w, int revents);
	void IOHandleOnDisconnect(Socket* socket);

private:
	void StopEvIO(ev_io *w);

	TcpServerCallback* mpTcpServerCallback;

	KMutex mServerMutex;
	bool mRunning;

	Socket mSocket;
	int miMaxConnection;

	IORunnable* mpIORunnable;
	KThread mIOThread;

	ev_io mAcceptWatcher;
	struct ev_loop *mLoop;
	KMutex mWatcherMutex;
	WatcherList mWatcherList;
};

#endif /* TCPSERVER_H_ */
