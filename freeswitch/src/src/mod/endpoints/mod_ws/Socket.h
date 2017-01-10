/*
 * Socket.h
 *
 *  Created on: 2016年9月23日
 *      Author: max
 */

#ifndef SRC_MOD_ENDPOINTS_MOD_WS_SOCKET_H_
#define SRC_MOD_ENDPOINTS_MOD_WS_SOCKET_H_

typedef struct Socket {
	static Socket* Create() {
		switch_memory_pool_t *pool;
		switch_core_new_memory_pool(&pool);

		Socket* socket = (Socket *)switch_core_alloc(pool, sizeof(Socket));
		socket->Create(pool);

		return socket;
	}

	static void Destroy(Socket* socket) {
		if( socket ) {
			socket->Destroy();
			switch_core_destroy_memory_pool(&socket->pool);
		}
	}

	Socket() {
		pool = NULL;
		socket = NULL;
		pollfd = NULL;
		ip = NULL;
		port = 0;
		data = NULL;
	};

	void Create(switch_memory_pool_t* pool) {
		socket = NULL;
		pollfd = NULL;
		ip = NULL;
		port = 0;
		data = NULL;

		this->pool = pool;
	}

	void Destroy() {

	}

	void SetAddress(const char* ip, switch_port_t port) {
		this->ip = switch_core_strdup(pool, ip);
		this->port = port;
	}

	void CreatePollfd(int16_t flags) {
		switch_socket_create_pollfd(&this->pollfd, this->socket, flags, this, this->pool);
	}

	switch_status_t Read(const char *data, switch_size_t* len) {
		return switch_socket_recv(socket, (char *)data, len);
	}

	bool Send(const char *data, switch_size_t* len) {
		switch_status_t status = switch_socket_send_nonblock(socket, (const char *)data, len);
		return status == SWITCH_STATUS_SUCCESS;
	}

	void Disconnect() {
		// 关掉连接socket读
		switch_socket_shutdown(socket, SWITCH_SHUTDOWN_READWRITE);
	}

	void Close() {
		switch_socket_close(socket);
	}

	switch_memory_pool_t *pool;

	switch_socket_t *socket;
	switch_pollfd_t *pollfd;

	const char *ip;
	switch_port_t port;

	/**
	 * 自定义数据
	 */
	void* data;

} Socket;

#endif /* SRC_MOD_ENDPOINTS_MOD_WS_SOCKET_H_ */
