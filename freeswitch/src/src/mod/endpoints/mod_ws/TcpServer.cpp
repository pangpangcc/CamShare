/*
 * TcpServer.cpp
 *
 *  Created on: 2016年9月12日
 *      Author: Max.Chiu
 */

#include "TcpServer.h"

static void *SWITCH_THREAD_FUNC ws_io_tcp_thread(switch_thread_t *thread, void *obj) {
	TcpServer* server = (TcpServer*)obj;
	server->IOHandleThread();
}

TcpServer::TcpServer() {
	// TODO Auto-generated constructor stub
	mpTcpServerCallback = NULL;

	mRunning = false;
	mpPool = NULL;

	mpSocket = NULL;

	mpIOThread = NULL;
	mpPollset = NULL;
	mpPollfd = NULL;
}

TcpServer::~TcpServer() {
	// TODO Auto-generated destructor stub
}

void TcpServer::SetTcpServerCallback(TcpServerCallback* callback) {
	mpTcpServerCallback = callback;
}

bool TcpServer::Start(switch_memory_pool_t *pool, const char *ip, switch_port_t port) {
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CONSOLE, "TcpServer::Start( Listening on %s:%u ) \n", ip, port);

	bool bFlag = true;
	switch_sockaddr_t *sa;
	switch_threadattr_t *thd_handle_attr = NULL;

	mpPool = pool;
	mRunning = true;

	mpSocket = Socket::Create();
	mpSocket->SetAddress(ip, port);

	if(switch_sockaddr_info_get(&sa, mpSocket->ip, SWITCH_INET, mpSocket->port, 0, mpPool)) {
		bFlag = false;
	}

	if(switch_socket_create(&mpSocket->socket, switch_sockaddr_get_family(sa), SOCK_STREAM, SWITCH_PROTO_TCP, mpPool)) {
		bFlag = false;
	}

	if(switch_socket_opt_set(mpSocket->socket, SWITCH_SO_REUSEADDR, 1)) {
		bFlag = false;
	}

	if(switch_socket_opt_set(mpSocket->socket, SWITCH_SO_TCP_NODELAY, 1)) {
		bFlag = false;
	}

	if(switch_socket_bind(mpSocket->socket, sa)) {
		bFlag = false;
	}

	if(switch_socket_listen(mpSocket->socket, 10)) {
		bFlag = false;
	}

	if(switch_socket_opt_set(mpSocket->socket, SWITCH_SO_NONBLOCK, TRUE)) {
		bFlag = false;
	}

	if(switch_pollset_create(&mpPollset, 10000 /* max poll fds */, pool, 0) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "TcpServer::Start( switch_pollset_create failed ) \n");
		bFlag = false;
	}

	if(switch_socket_create_pollfd(&mpPollfd, mpSocket->socket, SWITCH_POLLIN | SWITCH_POLLERR, mpSocket, pool) != SWITCH_STATUS_SUCCESS ) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "TcpServer::Start( switch_socket_create_pollfd failed ) \n");
		bFlag = false;
	}

	if(switch_pollset_add(mpPollset, mpPollfd) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "TcpServer::Start( switch_pollset_add failed ) \n");
		bFlag = false;
	}

	if( bFlag ) {
		switch_threadattr_create(&thd_handle_attr, mpPool);
		switch_threadattr_detach_set(thd_handle_attr, 1);
		switch_threadattr_stacksize_set(thd_handle_attr, SWITCH_THREAD_STACKSIZE);
		switch_threadattr_priority_set(thd_handle_attr, SWITCH_PRI_IMPORTANT);
		switch_thread_create(&mpIOThread, thd_handle_attr, ws_io_tcp_thread, this, mpPool);
	}

	if( bFlag ) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "TcpServer::Start( Listening on %s:%u, success ) \n", mpSocket->ip, mpSocket->port);
	} else {
		Stop();
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "TcpServer::Start( Listening on %s:%u, fail ) \n", mpSocket->ip, mpSocket->port);
	}

	return true;
}

void TcpServer::Stop() {
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "TcpServer::Stop( %s:%u ) \n", mpSocket->ip, mpSocket->port);

	// 关掉监听socket
	if( mpSocket ) {
		Disconnect(mpSocket);
	}

	mRunning = false;

	// 停止IO线程
	if( mpIOThread ) {
		switch_status_t retval;
		switch_thread_join(&retval, mpIOThread);
	}

	// 关闭监听Socket
	if( mpSocket ) {
		Close(mpSocket);
	}

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "TcpServer::Stop( %s:%u, finish ) \n", mpSocket->ip, mpSocket->port);
}

bool TcpServer::IsRuning() {
	return mRunning;
}

switch_status_t TcpServer::Read(Socket* socket, const char *data, switch_size_t* len) {
	return socket->Read(data, len);
}

bool TcpServer::Send(Socket* socket, const char *data, switch_size_t* len) {
	return socket->Send(data, len);
}

void TcpServer::Disconnect(Socket* socket) {
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "TcpServer::Disconnect( socket : %p ) \n", socket);

	// 关掉连接socket读
	socket->Disconnect();
}

void TcpServer::Close(Socket* socket) {
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "TcpServer::Close( socket : %p ) \n", socket);

	// 关掉连接socket
	socket->Close();

	// 释放内存
	Socket::Destroy(socket);
}

void TcpServer::IOHandleThread() {
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "TcpServer::IOHandleThread() \n");

	int32_t numfds = 0;
	int32_t i = 0;
	switch_status_t status;
	int32_t ret = 0;
	const switch_pollfd_t *fds;

	while( mRunning ) {
		numfds = 0;
		status = switch_pollset_poll(mpPollset, 500000, &numfds, &fds);

		if (status != SWITCH_STATUS_SUCCESS && status != SWITCH_STATUS_TIMEOUT) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "TcpServer::IOHandleThread( pollset_poll failed, status : %d ) \n", status);
			continue;
		} else if (status == SWITCH_STATUS_TIMEOUT) {
			// sleep
			switch_cond_next();
		}

		for (i = 0; i < numfds; i++) {
			if (fds[i].client_data == mpSocket) {
				// 监听socket收到事件
				switch_log_printf(
						SWITCH_CHANNEL_LOG,
						SWITCH_LOG_DEBUG,
						"TcpServer::IOHandleThread( "
						"Listener Socket Event : %d "
						") \n",
						fds[i].rtnevents
						);

				// 收到请求连接
				if (fds[i].rtnevents & SWITCH_POLLIN) {
					switch_socket_t *newsocket;
					if (switch_socket_accept(&newsocket, mpSocket->socket, mpPool) != SWITCH_STATUS_SUCCESS) {
						switch_log_printf(
								SWITCH_CHANNEL_LOG,
								SWITCH_LOG_ERROR,
								"TcpServer::IOHandleThread( "
								"Socket Accept Error : %s"
								" ) \n",
								strerror(errno)
								);
					} else {
						Socket* socket = Socket::Create();
						socket->socket = newsocket;

						switch_sockaddr_t *addr = NULL;
						char ipbuf[200];
						switch_socket_addr_get(&addr, SWITCH_TRUE, newsocket);
						if (addr) {
							switch_get_addr(ipbuf, sizeof(ipbuf), addr);
							socket->SetAddress(ipbuf, switch_sockaddr_get_port(addr));
						}

						switch_log_printf(
								SWITCH_CHANNEL_LOG,
								SWITCH_LOG_INFO,
								"TcpServer::IOHandleThread( "
								"Socket Accept : %p, "
								"ip : %s, "
								"port : %d "
								") \n",
								socket,
								socket->ip,
								socket->port
								);

						if( mpTcpServerCallback && mpTcpServerCallback->OnAccept(socket) ) {
							// 创建新连接
							ret = switch_socket_opt_set(newsocket, SWITCH_SO_NONBLOCK, TRUE);
							if (ret != 0) {
								switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "TcpServer::IOHandleThread( Couldn't set socket as non-blocking ) \n");
							}
							ret = switch_socket_opt_set(newsocket, SWITCH_SO_TCP_NODELAY, TRUE);
							if (ret != 0) {
								switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "TcpServer::IOHandleThread( Couldn't disable Nagle. ) \n");
							}
							ret = switch_socket_opt_set(newsocket, SWITCH_SO_KEEPALIVE, TRUE);
							if (ret != 0) {
								switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "TcpServer::IOHandleThread( Couldn't set socket KEEPALIVE, ret:%d. ) \n", ret);
							}
							// 间隔idle秒没有数据包，则发送keepalive包；若对端回复，则等idle秒再发keepalive包
							ret = switch_socket_opt_set(newsocket, SWITCH_SO_TCP_KEEPIDLE, 60);
							if (ret != 0) {
								switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "TcpServer::IOHandleThread( Couldn't set socket KEEPIDLE, ret:%d. ) \n", ret);
							}
							// 若发送keepalive对端没有回复，则间隔intvl秒再发送keepalive包
							ret = switch_socket_opt_set(newsocket, SWITCH_SO_TCP_KEEPINTVL, 20);
							if (ret != 0) {
								switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "TcpServer::IOHandleThread( Couldn't set socket KEEPINTVL, ret:%d. ) \n", ret);
							}
							// 若发送keepalive包，超过keepcnt次没有回复就认为断线
							if (switch_socket_opt_set(newsocket, SWITCH_SO_TCP_KEEPCNT, 3)) {
								switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "TcpServer::IOHandleThread( Couldn't set socket KEEPCNT. )\n");
							}

							// 开始监听接收事件
							socket->CreatePollfd(SWITCH_POLLIN | SWITCH_POLLERR);
							switch_pollset_add(mpPollset, socket->pollfd);

						} else {
							// 拒绝连接
							Disconnect(socket);
							// 关闭连接
							Close(socket);
						}
					}
				} else if (fds[i].rtnevents & (SWITCH_POLLERR|SWITCH_POLLHUP|SWITCH_POLLNVAL)) {
					if (mRunning) {
						/* Don't spam the logs if we are shutting down */
						switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "TcpServer::IOHandleThread( Socket Listen Error [%s] ) \n", strerror(errno));
					} else {
						return;
					}
				}
			} else {
				// 连接socket收到事件
				Socket* socket = (Socket *)fds[i].client_data;

				if (fds[i].rtnevents & (SWITCH_POLLERR|SWITCH_POLLHUP|SWITCH_POLLNVAL)) {
					// 连接读出错断开
					switch_pollset_remove(mpPollset, socket->pollfd);

					if( mpTcpServerCallback ) {
						mpTcpServerCallback->OnDisconnect(socket);
					}

				} else if (fds[i].rtnevents & SWITCH_POLLIN) {
					// 读出数据
					if( mpTcpServerCallback ) {
						mpTcpServerCallback->OnRecvEvent(socket);
					}
				}
			}
		}

	}

	mRunning = false;
	switch_socket_close(mpSocket->socket);

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "TcpServer::IOHandleThread( exit ) \n");

}
