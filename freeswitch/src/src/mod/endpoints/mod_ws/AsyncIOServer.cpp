/*
 * AsyncIOServer.cpp
 *
 *  Created on: 2016年9月13日
 *      Author: Max.Chiu
 */

#include "AsyncIOServer.h"

static void *SWITCH_THREAD_FUNC ws_handle_thread(switch_thread_t *thread, void *obj) {
	AsyncIOServer* server = (AsyncIOServer*)obj;
	server->RecvHandleThread();
}

AsyncIOServer::AsyncIOServer() {
	// TODO Auto-generated constructor stub
	mRunning = false;
	mpAsyncIOServerCallback = NULL;
	mpPool = NULL;

	mpHandleQueue = NULL;
	mThreadCount = 4;
	mpHandleThreads = NULL;

	mpSendQueue = NULL;
	mpSendThread = NULL;

	mTcpServer.SetTcpServerCallback(this);
}

AsyncIOServer::~AsyncIOServer() {
	// TODO Auto-generated destructor stub
	Stop();
}

void AsyncIOServer::SetAsyncIOServerCallback(AsyncIOServerCallback* callback) {
	mpAsyncIOServerCallback = callback;
}

bool AsyncIOServer::Start(
		switch_memory_pool_t *pool,
		int iThreadCount,
		const char *ip,
		switch_port_t port
		) {
	bool bFlag = false;

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "AsyncIOServer::Start( "
			"iThreadCount : %d, "
			"ip : %s, "
			"port : %d "
			") \n",
			iThreadCount,
			ip,
			port
			);

	mRunning = true;

	// 创建处理队列
	switch_queue_create(&mpHandleQueue, SWITCH_CORE_QUEUE_LEN, pool);

	// 创建处理线程
	switch_threadattr_t *thd_handle_attr = NULL;
	switch_threadattr_create(&thd_handle_attr, pool);
	switch_threadattr_detach_set(thd_handle_attr, 1);
	switch_threadattr_stacksize_set(thd_handle_attr, SWITCH_THREAD_STACKSIZE);
	switch_threadattr_priority_set(thd_handle_attr, SWITCH_PRI_IMPORTANT);

	mpHandleThreads = (switch_thread_t**)switch_core_alloc(pool, mThreadCount * sizeof(switch_thread_t*));
	for(int i = 0; i < mThreadCount; i++) {
		switch_thread_create(&mpHandleThreads[i], thd_handle_attr, ws_handle_thread, this, pool);
	}

	// 开始监听socket
	bFlag = mTcpServer.Start(pool, ip, port);

	if( bFlag ) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "AsyncIOServer::Start( success ) \n");
	} else {
		Stop();
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "AsyncIOServer::Start( fail ) \n");
	}

	return bFlag;
}

void AsyncIOServer::Stop() {
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "AsyncIOServer::Stop() \n");
	mRunning = false;

	// 停止监听socket
	mTcpServer.Stop();

	// 停止处理线程
	if( mpHandleThreads != NULL ) {
		for(int i = 0; i < mThreadCount; i++) {
			switch_status_t retval;
			switch_thread_join(&retval, mpHandleThreads[i]);
		}
	}

	// 销毁处理队列
	unsigned int size;
	void* pop = NULL;
	while(true) {
		size = switch_queue_size(mpHandleQueue);
		if( size == 0 ) {
			break;
		} else {
			switch_queue_pop(mpHandleQueue, &pop);
		}
	}

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "AsyncIOServer::Stop( finish ) \n");
}

bool AsyncIOServer::IsRuning() {
	return mRunning;
}

bool AsyncIOServer::Send(Client* client, const char* buffer, switch_size_t* len) {
	switch_log_printf(
			SWITCH_CHANNEL_UUID_LOG(client->uuid),
			SWITCH_LOG_DEBUG,
			"AsyncIOServer::Send( "
			"client : %p, "
			"len : %d "
			") \n",
			client,
			*len
			);
	bool bFlag = false;

	if( client ) {
		switch_mutex_lock(client->clientMutex);
		if( !client->disconnected ) {
			bFlag = mTcpServer.Send(client->socket, buffer, len);
			if( !bFlag ) {
				Disconnect(client);
			}
		}
		switch_mutex_unlock(client->clientMutex);
	}

	return bFlag;
}

void AsyncIOServer::Disconnect(Client* client) {
	switch_log_printf(
			SWITCH_CHANNEL_UUID_LOG(client->uuid),
			SWITCH_LOG_DEBUG,
			"AsyncIOServer::Disconnect( "
			"client : %p "
			") \n",
			client
			);
	if( client ) {
		mTcpServer.Disconnect(client->socket);
	}
}

bool AsyncIOServer::OnAccept(Socket* socket) {
	// 创建新连接
	Client* client = Client::Create(socket);//(Client *)switch_core_alloc(pool, sizeof(Client));
	socket->data = client;

	switch_log_printf(
			SWITCH_CHANNEL_UUID_LOG(client->uuid),
			SWITCH_LOG_DEBUG,
			"AsyncIOServer::OnAccept( "
			"client : %p, "
			"socket : %p, "
			"ip : %s, "
			"port : %d "
			") \n",
			client,
			socket,
			socket->ip,
			socket->port
			);

	if( mpAsyncIOServerCallback ) {
		mpAsyncIOServerCallback->OnAccept(client);
	}

	return true;
}

void AsyncIOServer::OnRecvEvent(Socket* socket) {
	Client* client = (Client *)(socket->data);
	if( client != NULL ) {
		switch_log_printf(
				SWITCH_CHANNEL_UUID_LOG(client->uuid),
				SWITCH_LOG_DEBUG,
				"AsyncIOServer::OnRecvEvent( client : %p, socket : %p ) \n",
				client,
				socket
				);

		// 尝试读取数据
		char buf[READ_BUFFER_SIZE];
		switch_size_t len = sizeof(buf);
		switch_status_t status = SWITCH_STATUS_SUCCESS;
		bool disconnect = false;

		// 有足够的缓存空间
		if( client->CheckBufferEnough() ) {
			while (true) {
				status = mTcpServer.Read(socket, (char *)buf, &len);
//				status = switch_socket_recv(socket->socket, (char *)buf, &len);
				if( status == SWITCH_STATUS_SUCCESS ) {
					// 读取数据成功, 缓存到客户端
					switch_log_printf(
							SWITCH_CHANNEL_UUID_LOG(client->uuid),
							SWITCH_LOG_DEBUG,
							"AsyncIOServer::OnRecvEvent( client : %p, socket : %p, read ok ) \n",
							client,
							socket
							);

					if( client->Write(buf, len) ) {
						// 放到处理队列
						switch_log_printf(
								SWITCH_CHANNEL_UUID_LOG(client->uuid),
								SWITCH_LOG_DEBUG,
								"AsyncIOServer::OnRecvEvent( client : %p, socket : %p, client write buffer ok ) \n",
								client,
								socket
								);

						RecvHandle(client);

					} else {
						// 没有足够的缓存空间
						switch_log_printf(
								SWITCH_CHANNEL_UUID_LOG(client->uuid),
								SWITCH_LOG_INFO,
								"AsyncIOServer::OnRecvEvent( client : %p, socket : %p, write error ) \n",
								client,
								socket
								);
						disconnect = true;
						break;
					}
				} else if( SWITCH_STATUS_IS_BREAK(status) /*|| (status == SWITCH_STATUS_INTR)*/ ) {
					// 没有数据可读超时返回, 不处理
					switch_log_printf(
							SWITCH_CHANNEL_UUID_LOG(client->uuid),
							SWITCH_LOG_DEBUG,
							"AsyncIOServer::OnRecvEvent( client : %p, socket : %p, nothing to read ) \n",
							client,
							socket
							);
					break;
				} else {
					// 读取数据出错, 断开
					switch_log_printf(
							SWITCH_CHANNEL_UUID_LOG(client->uuid),
							SWITCH_LOG_INFO,
							"AsyncIOServer::OnRecvEvent( client : %p, socket : %p, read error : %d ) \n",
							client,
							socket,
							status
							);
					disconnect = true;
					break;
				}
			}
		} else {
			// 缓存数据过大, 断开
			switch_log_printf(
					SWITCH_CHANNEL_UUID_LOG(client->uuid),
					SWITCH_LOG_INFO,
					"AsyncIOServer::OnRecvEvent( client : %p, socket : %p, buffer not enough error ) \n",
					client,
					socket
					);
			disconnect = true;
		}

		if( disconnect ) {
			Disconnect(client);
		}

		bool bFlag = false;
		switch_mutex_lock(client->clientMutex);
		bFlag = ClientCloseIfNeed(client);
		switch_mutex_unlock(client->clientMutex);

		if( bFlag ) {
			// 销毁客户端
			Client::Destroy(client);
		}
	}
}

void AsyncIOServer::OnDisconnect(Socket* socket) {
	Client* client = (Client *)(socket->data);
	if( client != NULL ) {
		switch_mutex_lock(client->clientMutex);
		client->disconnected = true;

		switch_log_printf(
				SWITCH_CHANNEL_UUID_LOG(client->uuid),
				SWITCH_LOG_DEBUG,
				"AsyncIOServer::OnDisconnect( "
				"client : %p, "
				"socket : %p, "
				"recvHandleCount : %d "
				") \n",
				client,
				socket,
				client->recvHandleCount
				);

		bool bFlag = ClientCloseIfNeed(client);

		switch_mutex_unlock(client->clientMutex);

		if( bFlag ) {
			// 销毁客户端
			Client::Destroy(client);
		}
	}
}

void AsyncIOServer::RecvHandleThread() {
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "AsyncIOServer::RecvHandleThread() \n");

	switch_interval_time_t timeout = 200000;
	void* pop = NULL;
	bool bFlag = false;

	while( mRunning ) {
		if ( SWITCH_STATUS_SUCCESS == switch_queue_pop_timeout(mpHandleQueue, &pop, timeout) ) {
			Client* client = (Client *)pop;

			switch_log_printf(
					SWITCH_CHANNEL_UUID_LOG(client->uuid),
					SWITCH_LOG_DEBUG,
					"AsyncIOServer::RecvHandleThread( "
					"client : %p "
					") \n",
					client
					);

			switch_mutex_lock(client->clientMutex);
			// 开始处理
			client->Parse();

			// 减少处理数
			client->recvHandleCount--;

			// 如果已经断开连接关闭socket
			bFlag = ClientCloseIfNeed(client);
			switch_mutex_unlock(client->clientMutex);

			if( bFlag ) {
				// 销毁客户端
				Client::Destroy(client);
			}
		}
	}

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "AsyncIOServer::RecvHandleThread( exit ) \n");
}

void AsyncIOServer::RecvHandle(Client* client) {
	switch_mutex_lock(client->clientMutex);
//	if( client->recvHandleCount == 0 ) {

	// 增加计数器
	client->recvHandleCount++;

	// 增加到处理队列
	switch_queue_push(mpHandleQueue, client);
//
//	} else {
//		// 已经有包在处理则返回
//	}
	switch_mutex_unlock(client->clientMutex);

//	switch_log_printf(SWITCH_CHANNEL_UUID_LOG(client->uuid), SWITCH_LOG_DEBUG, "AsyncIOServer::RecvHandle( "
//			"client : %p, "
//			"recvHandleCount : %d "
//			") \n",
//			client,
//			client->recvHandleCount
//			);
}

bool AsyncIOServer::ClientCloseIfNeed(Client* client) {
	bool bFlag = false;
	if( client->recvHandleCount == 0 && client->disconnected ) {
		switch_log_printf(
				SWITCH_CHANNEL_UUID_LOG(client->uuid),
				SWITCH_LOG_DEBUG,
				"AsyncIOServer::ClientCloseIfNeed( "
				"client : %p "
				") \n",
				client
				);

		if( mpAsyncIOServerCallback ) {
			mpAsyncIOServerCallback->OnDisconnect(client);
		}

		// 关闭Socket
		mTcpServer.Close(client->socket);

		bFlag = true;
	}

	return bFlag;
}
