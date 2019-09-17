/*
 * mod_rtmp for FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 * Copyright (C) 2011-2012, Barracuda Networks Inc.
 *
 * Version: MPL 1.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mod_rtmp for FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 *
 * The Initial Developer of the Original Code is Barracuda Networks Inc.
 * Portions created by the Initial Developer are Copyright (C)
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Mathieu Rene <mrene@avgs.ca>
 * William King <william.king@quentustech.com>
 * Seven Du <dujinfang@gmail.com>
 *
 * rtmp_tcp.c -- RTMP TCP I/O module
 *
 */

#include "mod_rtmp.h"
// Add by Max 2017-01-12
#include "rtmp_common.h"

#define RETRY_READ_MAXTIMES		2
// ------------------------

static switch_status_t rtmp_tcp_read(rtmp_session_t *rsession, unsigned char *buf, switch_size_t *len)
{
	//rtmp_io_tcp_t *io = (rtmp_io_tcp_t*)rsession->profile->io;
	rtmp_tcp_io_private_t *io_pvt = rsession->io_private;
	switch_status_t status = SWITCH_STATUS_SUCCESS;
#ifdef RTMP_DEBUG_IO
	switch_size_t olen = *len;
#endif
	switch_assert(*len > 0 && *len < 1024000);

	do {
		status = switch_socket_recv(io_pvt->socket, (char*)buf, len);
	} while(status != SWITCH_STATUS_SUCCESS && SWITCH_STATUS_IS_BREAK(status));

#ifdef RTMP_DEBUG_IO
	{
		int i;

		fprintf(rsession->io_debug_in, "recv %p max=%"SWITCH_SIZE_T_FMT" got=%"SWITCH_SIZE_T_FMT"\n< ", (void*)buf, olen, *len);

		for (i = 0; i < *len; i++) {

			fprintf(rsession->io_debug_in, "%02X ", (uint8_t)buf[i]);

			if (i != 0 && i % 32 == 0) {
				fprintf(rsession->io_debug_in, "\n> ");
			}
		}
		fprintf(rsession->io_debug_in, "\n\n");
		fflush(rsession->io_debug_in);
	}
#endif

	// add by Samson 2016-06-02
	if (SWITCH_STATUS_SUCCESS != status) {
		// add by Samson for test
//		switch_log_printf(SWITCH_CHANNEL_UUID_LOG(rsession->uuid), SWITCH_LOG_NOTICE
//				, "uuid:%s, status:%d, tryread:%d\n"
//				, rsession->uuid, status, rsession->tryread_times);
		if ((status | SWITCH_STATUS_INTR)
			&& rsession->tryread_times < RETRY_READ_MAXTIMES)
		{
			rsession->tryread_times++;
			status = SWITCH_STATUS_INTR;
		}
	}
	else {
		rsession->tryread_times = 0;
	}
	// ------------------------

	// add by Samson for test
//	if (SWITCH_STATUS_SUCCESS != status) {
//		switch_log_printf(SWITCH_CHANNEL_UUID_LOG(rsession->uuid), SWITCH_LOG_NOTICE
//				, "uuid:%s, status:%d, errno:%d, SWITCH_STATUS_BREAK:%d, tryread:%d, rtmp tcp read fail\n"
//				, rsession->uuid, status, switch_errno(), SWITCH_STATUS_BREAK, rsession->tryread_times);
//	}

	return status;
}

static switch_status_t rtmp_tcp_write(rtmp_session_t *rsession, const unsigned char *buf, switch_size_t *len)
{
	//rtmp_io_tcp_t *io = (rtmp_io_tcp_t*)rsession->profile->io;
	rtmp_tcp_io_private_t *io_pvt = rsession->io_private;
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	switch_size_t orig_len = *len;
	switch_size_t remaining = *len;
	int sanity = 0;

#ifdef RTMP_DEBUG_IO
	{
		int i;
		fprintf(rsession->io_debug_out,
			"SEND %"SWITCH_SIZE_T_FMT" bytes\n> ", *len);

		for (i = 0; i < *len; i++) {
			fprintf(rsession->io_debug_out, "%02X ", (uint8_t)buf[i]);

			if (i != 0 && i % 32 == 0) {
				fprintf(rsession->io_debug_out, "\n> ");
			}
		}
		fprintf(rsession->io_debug_out, "\n\n ");

		fflush(rsession->io_debug_out);
	}
#endif

	while (remaining > 0) {
		if (rsession->state >= RS_DESTROY) {
			return SWITCH_STATUS_FALSE;
		}

again:
		status = switch_socket_send_nonblock(io_pvt->socket, (char*)buf, len);

//		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "rtmp_tcp_write(), remaining %d, orig_len %d, status %d\n", (int)remaining, (int)*len, status);
		if ((status == 32 || SWITCH_STATUS_IS_BREAK(status)) && sanity++ < 100) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "rtmp_tcp_write(), sending too fast, retrying %d\n", sanity);
			switch_sleep(100000);
			goto again;
		}

		if (status != SWITCH_STATUS_SUCCESS) {
			if (NULL == rsession->account || NULL == rsession->account->user) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "send error %d\n", status);
			}
			else {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR
						, "[%s] send error %d\n", rsession->account->user, status);
			}
			break;
		}

		if (*len != orig_len) switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "sent %"SWITCH_SIZE_T_FMT" of %"SWITCH_SIZE_T_FMT"\n", *len, orig_len);
		buf += *len;
		remaining -= *len;
		*len = remaining;
	}

	*len = orig_len;

	// add by Samson for test
//	if (SWITCH_STATUS_SUCCESS != status) {
//		char buffer[512] = {0};
//		switch_strerror_r(switch_errno(), buffer, sizeof(buffer));
//		switch_log_printf(SWITCH_CHANNEL_UUID_LOG(rsession->uuid), SWITCH_LOG_DEBUG
//				, "uuid:%s, status:%d, error:%s, errno:%d, rtmp tcp write fail\n"
//				, rsession->uuid, status, buffer, switch_errno());
//	}

	return status;
}

static switch_status_t rtmp_tcp_close(rtmp_session_t *rsession)
{
	rtmp_io_tcp_t *io = (rtmp_io_tcp_t*)rsession->profile->io;
	rtmp_tcp_io_private_t *io_pvt = rsession->io_private;

	// add by Samson for test
//	switch_log_printf(SWITCH_CHANNEL_UUID_LOG(rsession->uuid), SWITCH_LOG_DEBUG
//			, "uuid:%s, rtmp tcp close\n", rsession->uuid);

	if (io_pvt->socket) {
		switch_mutex_lock(io->mutex);
		switch_pollset_remove(io->pollset, io_pvt->pollfd);
		switch_mutex_unlock(io->mutex);

		switch_socket_close(io_pvt->socket);
		io_pvt->socket = NULL;
	}
	return SWITCH_STATUS_SUCCESS;
}

// add by Samson 2016-05-28
static void add_to_timeout_list(rtmp_io_tcp_t* io, rtmp_session_t* rsession)
{
	switch_list_lock(io->timeout_list);
	switch_list_pushback(io->timeout_list, NULL, rsession);
	switch_list_unlock(io->timeout_list);

//	switch_log_printf(SWITCH_CHANNEL_UUID_LOG(rsession->uuid), SWITCH_LOG_DEBUG
//			, "list:%p, rsession:%p\n"
//			, (void*)io->timeout_list, (void*)rsession);
}

static void remove_from_timeout_list(rtmp_io_tcp_t* io, rtmp_session_t* rsession)
{
	switch_list_node_t* node = NULL;
	rtmp_session_t* node_rsession = NULL;

//	switch_log_printf(SWITCH_CHANNEL_UUID_LOG(rsession->uuid), SWITCH_LOG_DEBUG
//			, "list:%p, rsession:%p, check_timeout:%d\n"
//			, (void*)io->timeout_list, (void*)rsession, rsession->check_timeout);
	switch_list_lock(io->timeout_list);
	while (1) {
		switch_list_get_next(io->timeout_list, node, &node);
		if (NULL != node) {
			switch_list_get_node_data(io->timeout_list, node, (void**)&node_rsession);
			if (node_rsession == rsession) {
				switch_list_remove(io->timeout_list, node, NULL);
//					switch_log_printf(SWITCH_CHANNEL_UUID_LOG(rsession->uuid), SWITCH_LOG_DEBUG
//							, "list:%p, rsession:%p, check_timeout:%d, node:%p\n"
//							, (void*)io->timeout_list, (void*)rsession, rsession->check_timeout, (void*)node);
				break;
			}
		}
		else {
			break;
		}
	}
	switch_list_unlock(io->timeout_list);
}

void *SWITCH_THREAD_FUNC check_timeout_thread(switch_thread_t *thread, void *obj)
{
	rtmp_io_tcp_t *io = (rtmp_io_tcp_t*)obj;
	switch_list_node_t* node = NULL;
	rtmp_session_t *rsession = NULL;
	long long nowTime = 0;
	const char* user = NULL;

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "%s: Check Connection Timeout Thread starting\n", io->base.profile->name);

	while (io->base.running) {
		node = NULL;

		switch_list_lock(io->timeout_list);
		nowTime = GetTickCount();

		while (1) {
			switch_list_get_next(io->timeout_list, node, &node);
			if (NULL != node) {
				switch_list_get_node_data(io->timeout_list, node, (void**)&rsession);
				/*
				 * Add by Max 2017-01-12
				 * 1.配置心跳检测时间大于0
				 * 2.RTMP连接心跳时间大于配置时间
				 */
				if( rsession ) {
					switch_mutex_lock(rsession->handle_mutex);
					user = NULL;
					if( rsession->account ) {
						user = rsession->account->user;
					}

//					switch_log_printf(
//							SWITCH_CHANNEL_UUID_LOG(rsession->uuid),
//							SWITCH_LOG_INFO,
//							"check_timeout_thread() check time, "
//							"user : '%s', "
//							"check_timeout : %d, "
//							"active_time : %lld, "
//							"nowTime : %lld \n",
//							user,
//							rsession->check_timeout,
//							rsession->active_time,
//							nowTime
//							);

					if( io->base.profile->active_timeout > 0 && rsession->check_timeout && !rsession->client_from_mediaserver ) {
						if (nowTime >= (rsession->active_time + io->base.profile->active_timeout)) {
							switch_log_printf(
									SWITCH_CHANNEL_UUID_LOG(rsession->uuid),
									SWITCH_LOG_WARNING,
									"check_timeout_thread() shutdown, "
									"user : '%s' ",
									user
									);
							// 断开连接
							rsession->check_timeout = 0;
							rtmp_session_shutdown(&rsession);
						}
					}
					switch_mutex_unlock(rsession->handle_mutex);
				}
			}
			else {
				break;
			}
		}
		switch_list_unlock(io->timeout_list);

		switch_sleep(200000);
	}

	return NULL;
}
// ------------------------

void *SWITCH_THREAD_FUNC rtmp_io_tcp_thread(switch_thread_t *thread, void *obj)
{
	rtmp_io_tcp_t *io = (rtmp_io_tcp_t*)obj;
	io->base.running = 1;

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "%s: I/O Thread starting\n", io->base.profile->name);


	while(io->base.running) {
		const switch_pollfd_t *fds;
		int32_t numfds;
		int32_t i;
		switch_status_t status;

		switch_mutex_lock(io->mutex);
		status = switch_pollset_poll(io->pollset, 500000, &numfds, &fds);
		switch_mutex_unlock(io->mutex);

		if (status != SWITCH_STATUS_SUCCESS && status != SWITCH_STATUS_TIMEOUT) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "pollset_poll failed\n");
			continue;
		} else if (status == SWITCH_STATUS_TIMEOUT) {
			switch_cond_next();
		}

		// modify by Samson 2016-05-28
		for (i = 0; i < numfds; i++) {
			if (fds[i].client_data == io->listen_socket) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Listener socket, %d\n"
						, fds[i].rtnevents);
				if (fds[i].rtnevents & SWITCH_POLLIN) {
					switch_socket_t *newsocket;
					if (switch_socket_accept(&newsocket, io->listen_socket, io->base.pool) != SWITCH_STATUS_SUCCESS) {
						switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Socket Accept Error [%s]\n", strerror(errno));
					} else {
						rtmp_session_t *rsession = NULL;
						int32_t ret = 0;

						switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Setting socket\n");

						if (switch_socket_opt_set(newsocket, SWITCH_SO_NONBLOCK, TRUE)) {
							switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Couldn't set socket as non-blocking\n");
						}

						if (switch_socket_opt_set(newsocket, SWITCH_SO_TCP_NODELAY, 1)) {
							switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Couldn't disable Nagle.\n");
						}

						ret = switch_socket_opt_set(newsocket, SWITCH_SO_KEEPALIVE, 1);
						if (ret != 0) {
							switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Couldn't set socket KEEPALIVE, ret:%d.\n", ret);
						}
//						else {
//							switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "set socket KEEPALIVE success.\n");
//						}

						// 间隔idle秒没有数据包，则发送keepalive包；若对端回复，则等idle秒再发keepalive包
						ret = switch_socket_opt_set(newsocket, SWITCH_SO_TCP_KEEPIDLE, 60);
						if (ret != 0) {
							switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Couldn't set socket KEEPIDLE, ret:%d.\n", ret);
						}
//						else {
//							switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "set socket KEEPIDLE success.\n");
//						}

						// 若发送keepalive对端没有回复，则间隔intvl秒再发送keepalive包
						ret = switch_socket_opt_set(newsocket, SWITCH_SO_TCP_KEEPINTVL, 20);
						if (ret != 0) {
							switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Couldn't set socket KEEPINTVL, ret:%d.\n", ret);
						}
//						else {
//							switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "set socket KEEPINTVL success.\n");
//						}

						// 若发送keepalive包，超过keepcnt次没有回复就认为断线
						if (switch_socket_opt_set(newsocket, SWITCH_SO_TCP_KEEPCNT, 3)) {
							switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Couldn't set socket KEEPCNT.\n");
						}
//						else {
//							switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "set socket KEEPCNT success.\n");
//						}

						if (rtmp_session_request(io->base.profile, &rsession) != SWITCH_STATUS_SUCCESS) {
							switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "RTMP session request failed\n");
							switch_socket_close(newsocket);
						} else {
							rtmp_tcp_io_private_t *pvt = NULL;
							switch_sockaddr_t *addr = NULL;
							char ipbuf[200];

							// Add by Max 4 update timeout
							rsession->active_time = GetTickCount();
							rsession->check_timeout = 1;
							add_to_timeout_list(io, rsession);

							/* Create out private data and attach it to the rtmp session structure */
							pvt = switch_core_alloc(rsession->pool, sizeof(*pvt));
							rsession->io_private = pvt;
							pvt->socket = newsocket;
							switch_socket_create_pollfd(&pvt->pollfd, newsocket, SWITCH_POLLIN | SWITCH_POLLERR, rsession, rsession->pool);
							switch_pollset_add(io->pollset, pvt->pollfd);

							/* Get the remote address/port info */
							switch_socket_addr_get(&addr, SWITCH_TRUE, newsocket);
							if (addr) {
								switch_get_addr(ipbuf, sizeof(ipbuf), addr);
								rsession->remote_address = switch_core_strdup(rsession->pool, ipbuf);
								rsession->remote_port = switch_sockaddr_get_port(addr);
							}
							switch_log_printf(SWITCH_CHANNEL_UUID_LOG(rsession->uuid), SWITCH_LOG_INFO, "uuid:%s, Rtmp connection from %s:%i\n",
									rsession->uuid, rsession->remote_address, rsession->remote_port);
						}
					}
				}
				else if (fds[i].rtnevents & (SWITCH_POLLERR|SWITCH_POLLHUP|SWITCH_POLLNVAL)) {
					if (io->base.running) {
						/* Don't spam the logs if we are shutting down */
						switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Socket Listen Error [%s]\n", strerror(errno));
					} else {
						return NULL;
					}
				}
			} else {
				rtmp_session_t *rsession = (rtmp_session_t*)fds[i].client_data;
				rtmp_tcp_io_private_t *io_pvt = (rtmp_tcp_io_private_t*)rsession->io_private;

				if (fds[i].rtnevents & (SWITCH_POLLERR|SWITCH_POLLHUP|SWITCH_POLLNVAL)) {
					uint32_t handle_count = 0;
					switch_bool_t is_destroy = SWITCH_FALSE;

					switch_mutex_lock(rsession->handle_count_mutex);
					is_destroy = rsession->handle_count <= 0;
					handle_count = rsession->handle_count;
					switch_mutex_unlock(rsession->handle_count_mutex);

					if (is_destroy) {
						if (NULL == rsession->account || NULL == rsession->account->user) {
							switch_log_printf(SWITCH_CHANNEL_UUID_LOG(rsession->uuid), SWITCH_LOG_DEBUG
									, "Closing socket, uuid:%s, handle_count:%d\n"
									, rsession->uuid, handle_count);
						}
						else {
							switch_log_printf(SWITCH_CHANNEL_UUID_LOG(rsession->uuid), SWITCH_LOG_DEBUG
									, "Closing socket, uuid:%s, user:%s, handle_count:%d\n"
									, rsession->uuid, rsession->account->user, handle_count);
						}

						remove_from_timeout_list(io, rsession);

//						switch_log_printf(SWITCH_CHANNEL_UUID_LOG(rsession->uuid), SWITCH_LOG_DEBUG
//								, "uuid:%s, remove from timeout list\n", rsession->uuid);

						switch_mutex_lock(io->mutex);
						switch_pollset_remove(io->pollset, io_pvt->pollfd);
						switch_mutex_unlock(io->mutex);

//						switch_log_printf(SWITCH_CHANNEL_UUID_LOG(rsession->uuid), SWITCH_LOG_DEBUG
//								, "uuid:%s, remove\n", rsession->uuid);

						switch_socket_close(io_pvt->socket);
						io_pvt->socket = NULL;

//						switch_log_printf(SWITCH_CHANNEL_UUID_LOG(rsession->uuid), SWITCH_LOG_DEBUG
//								, "uuid:%s, close socket\n", rsession->uuid);

						io->base.close(rsession);

//						switch_log_printf(SWITCH_CHANNEL_UUID_LOG(rsession->uuid), SWITCH_LOG_DEBUG
//								, "uuid:%s, close\n", rsession->uuid);

						rtmp_session_destroy(&rsession);
					}
					else {
						// print log
//						if (NULL == rsession->account || NULL == rsession->account->user) {
//							switch_log_printf(SWITCH_CHANNEL_UUID_LOG(rsession->uuid), SWITCH_LOG_DEBUG
//									, "Wait to close socket, uuid:%s, handle_count:%d\n"
//									, rsession->uuid, handle_count);
//						}
//						else {
//							switch_log_printf(SWITCH_CHANNEL_UUID_LOG(rsession->uuid), SWITCH_LOG_DEBUG
//									, "Wait to close socket, uuid:%s, user:%s, handle_count:%d\n"
//									, rsession->uuid, rsession->account->user, handle_count);
//						}
					}
				}
				else if (fds[i].rtnevents & SWITCH_POLLIN) {
//					remove_from_timeout_list(io, rsession);

					switch_mutex_lock(rsession->handle_count_mutex);
					if (rsession->handle_count == 0) {
						rsession->handle_count++;
						switch_queue_push(io->handle_queue, (void*)rsession);
						// print log
//						if (NULL == rsession->account || NULL == rsession->account->user) {
//							switch_log_printf(SWITCH_CHANNEL_UUID_LOG(rsession->uuid), SWITCH_LOG_DEBUG
//									, "Push to handle queue, uuid:%s, handle_count:%d\n"
//									, rsession->uuid, rsession->handle_count);
//						}
//						else {
//							switch_log_printf(SWITCH_CHANNEL_UUID_LOG(rsession->uuid), SWITCH_LOG_DEBUG
//									, "Push to handle queue, uuid:%s, user:%s, handle_count:%d\n"
//									, rsession->uuid, rsession->account->user, rsession->handle_count);
//						}
					}
					switch_mutex_unlock(rsession->handle_count_mutex);
				}
			}
		}
//		for (i = 0; i < numfds; i++) {
//			if (!fds[i].client_data) {
//				switch_socket_t *newsocket;
//				if (switch_socket_accept(&newsocket, io->listen_socket, io->base.pool) != SWITCH_STATUS_SUCCESS) {
//					if (io->base.running) {
//						/* Don't spam the logs if we are shutting down */
//						switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Socket Error [%s]\n", strerror(errno));
//					} else {
//						return NULL;
//					}
//				} else {
//					rtmp_session_t *rsession;
//
//					if (switch_socket_opt_set(newsocket, SWITCH_SO_NONBLOCK, TRUE)) {
//						switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Couldn't set socket as non-blocking\n");
//					}
//
//					if (switch_socket_opt_set(newsocket, SWITCH_SO_TCP_NODELAY, 1)) {
//						switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Couldn't disable Nagle.\n");
//					}
//
//					if (rtmp_session_request(io->base.profile, &rsession) != SWITCH_STATUS_SUCCESS) {
//						switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "RTMP session request failed\n");
//						switch_socket_close(newsocket);
//					} else {
//						switch_sockaddr_t *addr = NULL;
//						char ipbuf[200];
//
//						/* Create out private data and attach it to the rtmp session structure */
//						rtmp_tcp_io_private_t *pvt = switch_core_alloc(rsession->pool, sizeof(*pvt));
//						rsession->io_private = pvt;
//						pvt->socket = newsocket;
//						switch_socket_create_pollfd(&pvt->pollfd, newsocket, SWITCH_POLLIN | SWITCH_POLLERR, rsession, rsession->pool);
//						switch_pollset_add(io->pollset, pvt->pollfd);
//
//						/* Get the remote address/port info */
//						switch_socket_addr_get(&addr, SWITCH_TRUE, newsocket);
//						if (addr) {
//							switch_get_addr(ipbuf, sizeof(ipbuf), addr);
//							rsession->remote_address = switch_core_strdup(rsession->pool, ipbuf);
//							rsession->remote_port = switch_sockaddr_get_port(addr);
//						}
//						switch_log_printf(SWITCH_CHANNEL_UUID_LOG(rsession->uuid), SWITCH_LOG_INFO, "uuid:%s, Rtmp connection from %s:%i\n",
//								rsession->uuid, rsession->remote_address, rsession->remote_port);
//					}
//				}
//			} else {
//				rtmp_session_t *rsession = (rtmp_session_t*)fds[i].client_data;
//				rtmp_tcp_io_private_t *io_pvt = (rtmp_tcp_io_private_t*)rsession->io_private;
//
//				if (rtmp_handle_data(rsession) != SWITCH_STATUS_SUCCESS) {
//					switch_mutex_lock(io->mutex);
//					switch_pollset_remove(io->pollset, io_pvt->pollfd);
//					switch_mutex_unlock(io->mutex);
//
//					switch_socket_close(io_pvt->socket);
//					io_pvt->socket = NULL;
//
//					io->base.close(rsession);
//
//					rtmp_session_destroy(&rsession);
//				}
//			}
//		}
		// ---------------------------
	}

//	// Modify by Max 2017/01/22 for crash when shutdown
//	io->base.running = 0;
	switch_socket_close(io->listen_socket);

	return NULL;
}

void rtmp_handle_proc(rtmp_session_t *rsession)
{
	switch_status_t result;
	switch_mutex_lock(rsession->handle_mutex);
//	switch_log_printf(SWITCH_CHANNEL_UUID_LOG(rsession->uuid), SWITCH_LOG_DEBUG
//			, "uuid:%s, handle_count:%d, handle data\n", rsession->uuid, rsession->handle_count);
	result = rtmp_handle_data(rsession);
	if (result != SWITCH_STATUS_SUCCESS)
	{
		rtmp_session_shutdown(&rsession);

		if (NULL == rsession->account || NULL == rsession->account->user) {
			switch_log_printf(SWITCH_CHANNEL_UUID_LOG(rsession->uuid), SWITCH_LOG_DEBUG
					, "rtmp handle data fail, result:%d\n"
					, result);
		}
		else {
			switch_log_printf(SWITCH_CHANNEL_UUID_LOG(rsession->uuid), SWITCH_LOG_DEBUG
					, "rtmp handle data fail, user:%s, result:%d\n"
					, rsession->account->user, result);
		}
	}
	switch_mutex_unlock(rsession->handle_mutex);
}

// add by Samson 2016-05-19
void *SWITCH_THREAD_FUNC rtmp_handle_thread(switch_thread_t *thread, void *obj)
{
	rtmp_io_tcp_t *io = (rtmp_io_tcp_t*)obj;
	switch_interval_time_t timeout = 200000;
	rtmp_session_t* rsession = NULL;
	void* pop = NULL;

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "%s: handle Thread starting\n", io->base.profile->name);

	while(io->base.running) {
		if (SWITCH_STATUS_SUCCESS == switch_queue_pop_timeout(io->handle_queue, &pop, timeout))
		{
			rsession = (rtmp_session_t*)pop;
//			switch_log_printf(SWITCH_CHANNEL_UUID_LOG(rsession->uuid), SWITCH_LOG_DEBUG, "uuid:%s, Pop queue\n", rsession->uuid);

			rtmp_handle_proc(rsession);

			switch_mutex_lock(rsession->handle_count_mutex);
			rsession->handle_count--;
			// print log
//			if (NULL == rsession->account || NULL == rsession->account->user) {
//				switch_log_printf(SWITCH_CHANNEL_UUID_LOG(rsession->uuid), SWITCH_LOG_DEBUG
//						, "Pop queue and handle finish, uuid:%s, handle_count:%d\n"
//						, rsession->uuid, rsession->handle_count);
//			}
//			else {
//				switch_log_printf(SWITCH_CHANNEL_UUID_LOG(rsession->uuid), SWITCH_LOG_DEBUG
//						, "Pop queue and handle finish, uuid:%s, user:%s, handle_count:%d\n"
//						, rsession->uuid, rsession->account->user, rsession->handle_count);
//			}
			switch_mutex_unlock(rsession->handle_count_mutex);
		}
	}

	return NULL;
}
// -----------------------

switch_status_t rtmp_tcp_init(rtmp_profile_t *profile, const char *bindaddr, rtmp_io_t **new_io, switch_memory_pool_t *pool)
{
	char *szport;
	switch_sockaddr_t *sa;
	switch_threadattr_t *thd_handle_attr = NULL;
	switch_threadattr_t *thd_timeout_attr = NULL;
	rtmp_io_tcp_t *io_tcp;

	io_tcp = (rtmp_io_tcp_t*)switch_core_alloc(pool, sizeof(rtmp_io_tcp_t));
	io_tcp->base.pool = pool;
	io_tcp->ip = switch_core_strdup(pool, bindaddr);

	*new_io = (rtmp_io_t*)io_tcp;
	io_tcp->base.profile = profile;
	io_tcp->base.read = rtmp_tcp_read;
	io_tcp->base.write = rtmp_tcp_write;
	io_tcp->base.close = rtmp_tcp_close;
	io_tcp->base.name = "tcp";
	io_tcp->base.address = switch_core_strdup(pool, io_tcp->ip);

	if ((szport = strchr(io_tcp->ip, ':'))) {
		*szport++ = '\0';
		io_tcp->port = atoi(szport);
	} else {
		io_tcp->port = RTMP_DEFAULT_PORT;
	}

	if (switch_sockaddr_info_get(&sa, io_tcp->ip, SWITCH_INET, io_tcp->port, 0, pool)) {
		goto fail;
	}
	if (switch_socket_create(&io_tcp->listen_socket, switch_sockaddr_get_family(sa), SOCK_STREAM, SWITCH_PROTO_TCP, pool)) {
		goto fail;
	}
	if (switch_socket_opt_set(io_tcp->listen_socket, SWITCH_SO_REUSEADDR, 1)) {
		goto fail;
	}
	if (switch_socket_opt_set(io_tcp->listen_socket, SWITCH_SO_TCP_NODELAY, 1)) {
		goto fail;
	}
	if (switch_socket_bind(io_tcp->listen_socket, sa)) {
		goto fail;
	}
	if (switch_socket_listen(io_tcp->listen_socket, 10)) {
		goto fail;
	}
	if (switch_socket_opt_set(io_tcp->listen_socket, SWITCH_SO_NONBLOCK, TRUE)) {
		goto fail;
	}

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Listening on %s:%u (tcp)\n", io_tcp->ip, io_tcp->port);

	io_tcp->base.running = 1;

	if (switch_pollset_create(&io_tcp->pollset, 1000 /* max poll fds */, pool, 0) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "pollset_create failed\n");
		goto fail;
	}

	// modify by Samson 2016-05-28
//	switch_socket_create_pollfd(&(io_tcp->listen_pollfd), io_tcp->listen_socket, SWITCH_POLLIN | SWITCH_POLLERR, NULL, pool);
	switch_socket_create_pollfd(&(io_tcp->listen_pollfd), io_tcp->listen_socket, SWITCH_POLLIN | SWITCH_POLLERR, io_tcp->listen_socket, pool);
	// ------------------------
	if (switch_pollset_add(io_tcp->pollset, io_tcp->listen_pollfd) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "pollset_add failed\n");
		goto fail;
	}

	switch_mutex_init(&io_tcp->mutex, SWITCH_MUTEX_NESTED, pool);

	// add by Samson 2016-05-19
	// create handle queue
	switch_queue_create(&io_tcp->handle_queue, SWITCH_CORE_QUEUE_LEN, pool);
	// ------------------------

	// add by Samson 2016-05-28
	// create timeout list and free queue
	switch_list_create(&io_tcp->timeout_list, pool);
	// ------------------------

	switch_threadattr_create(&thd_handle_attr, pool);
	switch_threadattr_detach_set(thd_handle_attr, 1);
	switch_threadattr_stacksize_set(thd_handle_attr, SWITCH_THREAD_STACKSIZE);
	switch_threadattr_priority_set(thd_handle_attr, SWITCH_PRI_IMPORTANT);
	switch_thread_create(&io_tcp->thread, thd_handle_attr, rtmp_io_tcp_thread, *new_io, pool);

	// add by Samson 2016-05-18
	// create handle threads
	{
		int i = 0;
		io_tcp->handle_thread = (switch_thread_t**)switch_core_alloc(pool, profile->handle_thread * sizeof(switch_thread_t*));
		for (i = 0; i < profile->handle_thread; i++)
		{
			switch_thread_create(&io_tcp->handle_thread[i], thd_handle_attr, rtmp_handle_thread, *new_io, pool);
		}
	}

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CONSOLE, "Numbers of handle thread:%d\n", profile->handle_thread);
	// ------------------------

	// add by Samson 2016-05-28
	// create timeout thread
	switch_threadattr_create(&thd_timeout_attr, pool);
	switch_threadattr_detach_set(thd_timeout_attr, 1);
	switch_threadattr_stacksize_set(thd_timeout_attr, SWITCH_THREAD_STACKSIZE);
	switch_threadattr_priority_set(thd_timeout_attr, SWITCH_PRI_LOW);
	switch_thread_create(&(io_tcp->timeout_thread), thd_timeout_attr, check_timeout_thread, *new_io, pool);
	// ------------------------

	return SWITCH_STATUS_SUCCESS;
fail:
	if (io_tcp->listen_socket) {
		switch_socket_close(io_tcp->listen_socket);
	}
	*new_io = NULL;
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Socket error. Couldn't listen on %s:%u\n", io_tcp->ip, io_tcp->port);
	return SWITCH_STATUS_FALSE;
}

// add by Samson 2016-06-01
switch_status_t rtmp_tcp_release(rtmp_profile_t *profile)
{
	rtmp_io_tcp_t *io_tcp = (rtmp_io_tcp_t*)profile->io;
	switch_list_node_t* node = NULL;

	if (NULL != io_tcp) {
		// Modify by Max 2017/01/22
		io_tcp->base.running = 0;

		// wait timeout thread exit
		if (NULL != io_tcp->timeout_thread) {
			switch_status_t st;
			switch_thread_join(&st, io_tcp->timeout_thread);
		}

		// clear timeout list
		switch_list_lock(io_tcp->timeout_list);
		while (1) {
			switch_list_get_next(io_tcp->timeout_list, node, &node);
			if (NULL != node) {
				switch_list_remove(io_tcp->timeout_list, node, NULL);
				node = NULL;
			}
			else {
				break;
			}
		}
		switch_list_unlock(io_tcp->timeout_list);

		// wait handle thread exit
		if (NULL != io_tcp->handle_thread) {
			int i;
			for (i = 0; i < profile->handle_thread; i++) {
				if (NULL != io_tcp->handle_thread[i]) {
					switch_status_t st;
					switch_thread_join(&st, io_tcp->handle_thread[i]);
				}
			}
		}
	}

	return SWITCH_STATUS_SUCCESS;
}
// ------------------------

/* For Emacs:
 * Local Variables:
 * mode:c
 * indent-tabs-mode:t
 * tab-width:4
 * c-basic-offset:4
 * End:
 * For VIM:
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4 noet:
 */
