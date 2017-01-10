/*
 * Client.h
 *
 *  Created on: 2016年9月23日
 *      Author: max
 */

#ifndef SRC_MOD_ENDPOINTS_MOD_WS_CLIENT_H_
#define SRC_MOD_ENDPOINTS_MOD_WS_CLIENT_H_

#include <switch.h>

#include "Socket.h"

/**
 * 每次读取数据Buffer Size
 */
#define READ_BUFFER_SIZE 1024
/**
 * 总读包缓存Buffer Size
 */
#define CLIENT_BUFFER_SIZE 1 * 1024 * 1024

typedef struct Client {
	static Client* Create(Socket *socket) {
		switch_memory_pool_t *pool;
		switch_core_new_memory_pool(&pool);

		Client* client = (Client *)switch_core_alloc(pool, sizeof(Client));
		client->Create(pool, socket);

		return client;
	}

	static void Destroy(Client* client) {
		if( client ) {
			client->Destroy();
			switch_core_destroy_memory_pool(&client->pool);
		}
	}

	Client() {
		pool = NULL;

		buffer = NULL;
		socket = NULL;

		recvHandleCount = 0;
		disconnected = false;
		clientMutex = NULL;

		parser = NULL;

		data = NULL;
	}

	void Create(switch_memory_pool_t* pool, Socket *socket) {
		recvHandleCount = 0;
		disconnected = false;
		parser = NULL;
		data = NULL;

		this->pool = pool;
		this->socket = socket;

		if( !buffer ) {
			switch_buffer_create(pool, &buffer, CLIENT_BUFFER_SIZE);
//			switch_buffer_bzero(buffer);
		}

		if( !clientMutex ) {
			switch_mutex_init(&clientMutex, SWITCH_MUTEX_NESTED, pool);
		}

		switch_log_printf(
				SWITCH_CHANNEL_LOG,
				SWITCH_LOG_DEBUG,
				"Client::Create( "
				"client : %p, "
				"socket : %p "
				") \n",
				this,
				this->socket
				);
	}

	void Destroy() {
		switch_log_printf(
				SWITCH_CHANNEL_LOG,
				SWITCH_LOG_DEBUG,
				"Client::Destroy( "
				"client : %p, "
				"socket : %p "
				") \n",
				this,
				this->socket
				);
		this->socket->data = NULL;

		if( buffer ) {
			switch_buffer_destroy(&buffer);
			buffer = NULL;
		}
		if( clientMutex ) {
			switch_mutex_destroy(clientMutex);
			clientMutex = NULL;
		}
	}

	bool CheckBufferEnough() {
		bool bFlag = true;
		bFlag = switch_buffer_freespace(buffer) > READ_BUFFER_SIZE;
		return bFlag;
	}

	bool Write(char *buf, switch_size_t len) {
		switch_size_t size = 0;
		size = switch_buffer_write(buffer, buf, len);
		return size > 0;
	}

	void Parse() {
		void *data = NULL;
		switch_size_t size = 0;
		switch_buffer_peek_zerocopy(buffer, (const void **)&data);
		size = switch_buffer_inuse(buffer);

		if( size > 0 && data ) {
//			switch_log_printf(
//					SWITCH_CHANNEL_UUID_LOG(uuid),
//					SWITCH_LOG_DEBUG,
//					"Client::Parse( "
//					"client : %p, "
//					"socket : %p, "
//					"size : %d "
//					") \n",
//					this,
//					this->socket,
//					size
//					);

			if( parser ) {
				int parseLen = parser->ParseData((char *)data, size);
				if( parseLen > 0 ) {
//					switch_log_printf(
//							SWITCH_CHANNEL_UUID_LOG(uuid),
//							SWITCH_LOG_DEBUG,
//							"Client::Parse( "
//							"client : %p, "
//							"socket : %p, "
//							"parseLen : %d "
//							") \n",
//							this,
//							this->socket,
//							parseLen
//							);

					// 去除已经解析的Buffer
//					size = switch_buffer_recorvery(buffer, parseLen);
					size = switch_buffer_toss(buffer, parseLen);
				}
			}
		}
	}

	/**
	 * 内存池
	 */
	switch_memory_pool_t *pool;

	/**
	 *	接收到的数据
	 */
	switch_buffer_t *buffer;

	/**
	 * socket
	 */
	Socket *socket;

	/**
	 * 处理队列中数量
	 */
	int recvHandleCount;

	/**
	 * 是否已经断开连接
	 */
	bool disconnected;

	/**
	 * 同步锁
	 */
	switch_mutex_t *clientMutex;

	/**
	 * 解析器
	 */
	IDataParser* parser;

	/**
	 * 自定义数据
	 */
	void* data;

} Client;

#endif /* SRC_MOD_ENDPOINTS_MOD_WS_CLIENT_H_ */
