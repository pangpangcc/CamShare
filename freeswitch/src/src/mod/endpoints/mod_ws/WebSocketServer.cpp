/*
 * WebSocketServer.cpp
 *
 *  Created on: 2016年9月13日
 *      Author: Max.Chiu
 */

#include "WebSocketServer.h"

#define READ_BUFFER_SIZE 1024
#define CLIENT_BUFFER_SIZE 1024 * 1024

typedef struct Client {
	Client() {
		pool = NULL;

		mutex = NULL;
		buffer = NULL;

		socket = NULL;
		disconnected = 0;

		handleCountMutex = NULL;
		handleCount = 0;
	}

	void Create(switch_memory_pool_t* pool) {
		this->pool = pool;

		if( !mutex ) {
			switch_mutex_init(&mutex, SWITCH_MUTEX_NESTED, pool);
		}
		if( !buffer ) {
			switch_buffer_create(pool, &buffer, CLIENT_BUFFER_SIZE);
		}
		if( !handleCountMutex ) {
			switch_mutex_init(&handleCountMutex, SWITCH_MUTEX_NESTED, pool);
		}
		handleCount = 0;
	}

	void Destroy() {
		switch_log_printf(
				SWITCH_CHANNEL_LOG,
				SWITCH_LOG_INFO,
				"WebSocketServer::Destroy( "
				"client : %p, "
				"socket : %p "
				") \n",
				this,
				this->socket
				);
		if( buffer ) {
			switch_buffer_destroy(&buffer);
			buffer = NULL;
		}
		if( mutex ) {
			switch_mutex_destroy(mutex);
		}
		if( handleCountMutex ) {
			switch_mutex_destroy(handleCountMutex);
		}
	}

	bool CheckBufferEnough() {
		bool bFlag = true;
		switch_buffer_lock(buffer);
		bFlag = switch_buffer_inuse(buffer) < CLIENT_BUFFER_SIZE - READ_BUFFER_SIZE;
		switch_buffer_unlock(buffer);
		return bFlag;
	}

	void Read(char *buf, switch_size_t len) {
		switch_buffer_lock(buffer);
		switch_buffer_write(buffer, buf, len);
		switch_buffer_unlock(buffer);
	}

	void Handle() {
		switch_buffer_lock(buffer);
		char temp[1024];
		switch_size_t size = switch_buffer_read(buffer, temp, sizeof(temp));
		if( size > 0 ) {
			switch_log_printf(
					SWITCH_CHANNEL_LOG,
					SWITCH_LOG_INFO,
					"WebSocketServer::Handle( "
					"tid : %lld, "
					"client : %p, "
					"socket : %p, "
					"buffer : %s"
					") \n",
					switch_thread_self(),
					this,
					this->socket,
					temp
					);
		}
		switch_buffer_unlock(buffer);
	}

	/**
	 * 内存池
	 */
	switch_memory_pool_t *pool;

	/**
	 *	接收到的数据
	 */
	switch_mutex_t *mutex;
	switch_buffer_t *buffer;

	/**
	 * socket
	 */
	Socket *socket;

	/**
	 * 是否已经断开连接
	 */
	bool disconnected;

	/**
	 * 处理队列中数量
	 */
	switch_mutex_t *handleCountMutex;
	int handleCount;

} Client;

static void *SWITCH_THREAD_FUNC ws_handle_thread(switch_thread_t *thread, void *obj) {
	WebSocketServer* server = (WebSocketServer*)obj;
	server->HandleThreadHandle();
}

WebSocketServer::WebSocketServer() {
	// TODO Auto-generated constructor stub
	mRunning = false;

	mpPool = NULL;

	mpClientHash = NULL;

	mpHandleQueue = NULL;
	mThreadCount = 4;
	mpHandleThreads = NULL;

	mpTcpServer = new TcpServer();
	mpTcpServer->SetTcpServerCallback(this);
}

WebSocketServer::~WebSocketServer() {
	// TODO Auto-generated destructor stub
	Shutdown();

	if( mpTcpServer ) {
		delete mpTcpServer;
		mpTcpServer = NULL;
	}
}

bool WebSocketServer::Load(switch_memory_pool_t *pool) {
	bool bFlag = false;

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "WebSocketServer::Load() \n");

	// 读取配置
	bFlag = LoadConfig();

	if( bFlag ) {
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
		bFlag = mpTcpServer->Start(pool, "0.0.0.0", 8080);

	}

	if( bFlag ) {
		// 创建在线列表
		switch_core_hash_init(&mpClientHash);
	}

	if( bFlag ) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "WebSocketServer::Load( Success ) \n");
	} else {
		Shutdown();
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "WebSocketServer::Load( Fail ) \n");
	}

	return bFlag;
}

void WebSocketServer::Shutdown() {
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "WebSocketServer::Shutdown() \n");
	mRunning = false;

	// 停止监听socket
	mpTcpServer->Stop();

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

	// 销毁在线列表
	switch_core_hash_destroy(&mpClientHash);
}

bool WebSocketServer::IsRuning() {
	return mRunning;
}

void WebSocketServer::HandleThreadHandle() {
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "WebSocketServer::HandleThreadHandle( start ) \n");

	switch_interval_time_t timeout = 200000;
	void* pop = NULL;

	while( mRunning ) {
		if ( SWITCH_STATUS_SUCCESS == switch_queue_pop_timeout(mpHandleQueue, &pop, timeout) ) {
			Client* client = (Client *)pop;
			// 客户端处理
			client->Handle();

			// 减少处理数
			switch_mutex_lock(client->handleCountMutex);
			client->handleCount--;
			switch_mutex_unlock(client->handleCountMutex);
			// 如果已经断开连接关闭socket
			ClientCloseIfNeed(client);
		}
	}

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "WebSocketServer::HandleThreadHandle( exit ) \n");
}

bool WebSocketServer::OnAceept(Socket* socket) {
	switch_memory_pool_t *pool;
	switch_core_new_memory_pool(&pool);

	Client* client = (Client *)switch_core_alloc(pool, sizeof(Client));
	client->socket = socket;
	socket->data = client;

	client->Create(pool);

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "WebSocketServer::OnAceept( client : %p, socket : %p ) \n", client, socket);

	return true;
}

void WebSocketServer::OnRecvEvent(Socket* socket) {
	Client* client = (Client *)(socket->data);

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "WebSocketServer::OnRecvEvent( client : %p, socket : %p ) \n", client, socket);

	if( client != NULL ) {
		// 尝试读取数据
		char buf[READ_BUFFER_SIZE];
		switch_size_t len = sizeof(buf);
		switch_status_t status = SWITCH_STATUS_SUCCESS;

		if( client->CheckBufferEnough() ) {
			while (true) {
				status = switch_socket_recv(socket->socket, (char *)buf, &len);
				if( status == SWITCH_STATUS_SUCCESS ) {
					// 读取数据成功
					client->Read(buf, len);

					// 放到处理队列
					ClientHandle(client);

				} else if( SWITCH_STATUS_IS_BREAK(status) ) {
					// 没有数据可读超时返回, 不处理
					break;
				} else {
					// 断开
					client->disconnected = true;
					ClientCloseIfNeed(client);
					break;
				}
			}
		}
	}
}

void WebSocketServer::OnDisconnect(Socket* socket) {
	Client* client = (Client *)(socket->data);
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "WebSocketServer::OnDisconnect( client : %p, socket : %p ) \n", client, socket);

	if( client != NULL ) {
		client->disconnected = true;
	}
}

bool WebSocketServer::LoadConfig() {
	bool bFlag = true;

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "WebSocketServer::LoadConfig() \n");

	return bFlag;
}

void WebSocketServer::ClientHandle(Client* client) {
	switch_mutex_lock(client->handleCountMutex);
	client->handleCount++;
	switch_mutex_unlock(client->handleCountMutex);

	// 增加到处理队列
	switch_queue_push(mpHandleQueue, client);
}

void WebSocketServer::ClientCloseIfNeed(Client* client) {
	switch_mutex_lock(client->handleCountMutex);
	if( client->handleCount == 0 && client->disconnected ) {
		mpTcpServer->Close(client->socket);
		// 移除在线列表

		// 销毁客户端
		client->Destroy();

		// 释放内存池
		switch_core_destroy_memory_pool(&client->pool);

		client = NULL;
	}
	switch_mutex_unlock(client->handleCountMutex);
}
