/*
 * WebSocketServer.cpp
 *
 *  Created on: 2016年9月14日
 *      Author: max
 */

#include "WebSocketServer.h"
#include "WSClientParser.h"

#ifndef INT32_MAX
#define INT32_MAX 0x7fffffffL
#endif

/***************************** 频道路由和状态接口 **************************************/
switch_status_t ws_on_init(switch_core_session_t *session);
switch_status_t ws_on_hangup(switch_core_session_t *session);
switch_status_t ws_on_destroy(switch_core_session_t *session);

switch_state_handler_table_t ws_state_handlers = {
	/*.on_init */ ws_on_init,
	/*.on_routing */ NULL,
	/*.on_execute */ NULL,
	/*.on_hangup */ ws_on_hangup,
	/*.on_exchange_media */ NULL,
	/*.on_soft_execute */ NULL,
	/*.on_consume_media */ NULL,
	/*.on_hibernate */ NULL,
	/*.on_reset */ NULL,
	/*.on_park */ NULL,
	/*.on_reporting */ NULL,
	/*.on_destroy */ ws_on_destroy
};
/***************************** 频道路由和状态接口 end **************************************/

/***************************** 音视频读写接口 **************************************/
switch_call_cause_t ws_outgoing_channel(
		switch_core_session_t *session,
		switch_event_t *var_event,
		switch_caller_profile_t *outbound_profile,
		switch_core_session_t **newsession,
		switch_memory_pool_t **inpool,
		switch_originate_flag_t flags,
		switch_call_cause_t *cancel_cause
		);
switch_status_t ws_receive_message(switch_core_session_t *session, switch_core_session_message_t *msg);
switch_status_t ws_read_frame(switch_core_session_t *session, switch_frame_t **frame, switch_io_flag_t flags, int stream_id);
switch_status_t ws_write_frame(switch_core_session_t *session, switch_frame_t *frame, switch_io_flag_t flags, int stream_id);
switch_status_t ws_read_video_frame(switch_core_session_t *session, switch_frame_t **frame, switch_io_flag_t flags, int stream_id);
switch_status_t ws_write_video_frame(switch_core_session_t *session, switch_frame_t *frame, switch_io_flag_t flags, int stream_id);

switch_io_routines_t ws_io_routines = {
	/*.outgoing_channel */ ws_outgoing_channel,
	/*.read_frame */ ws_read_frame,
	/*.write_frame */ ws_write_frame,
	/*.kill_channel */ NULL,
	/*.send_dtmf */ NULL,
	/*.receive_message */ ws_receive_message,
	/*.receive_event */ NULL,
	/*.state_change*/ NULL,
	/*.ws_read_vid_frame */ ws_read_video_frame,
	/*.ws_write_vid_frame */ ws_write_video_frame
};
/***************************** 音视频读写接口 end **************************************/

/***************************** 频道路由和状态接口 **************************************/
switch_status_t ws_on_init(switch_core_session_t *session)
{
	switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "ws_on_init() session : %p \n", (void *)session);
	WebSocketServer* server = (WebSocketServer *)switch_core_session_get_private(session);
	return server->WSOnInit(session);
}

switch_status_t ws_on_hangup(switch_core_session_t *session)
{
	switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "ws_on_hangup() session : %p \n", (void *)session);
	WebSocketServer* server = (WebSocketServer *)switch_core_session_get_private(session);
	return server->WSOnHangup(session);
}

switch_status_t ws_on_destroy(switch_core_session_t *session)
{
	switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "ws_on_destroy() session : %p \n", (void *)session);
	WebSocketServer* server = (WebSocketServer *)switch_core_session_get_private(session);
	return server->WSOnDestroy(session);
}

/***************************** 频道路由和状态接口 end **************************************/

/***************************** 音视频读写接口 **************************************/
switch_call_cause_t ws_outgoing_channel(
		switch_core_session_t *session,
		switch_event_t *var_event,
		switch_caller_profile_t *outbound_profile,
		switch_core_session_t **newsession,
		switch_memory_pool_t **inpool,
		switch_originate_flag_t flags,
		switch_call_cause_t *cancel_cause
		)
{
	switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "ws_outgoing_channel() session : %p \n", (void *)session);
	WebSocketServer* server = (WebSocketServer *)switch_core_session_get_private(session);
	return server->WSOutgoingChannel(session, var_event, outbound_profile, newsession, inpool, flags, cancel_cause);
}

switch_status_t ws_receive_message(switch_core_session_t *session, switch_core_session_message_t *msg)
{
	WebSocketServer* server = (WebSocketServer *)switch_core_session_get_private(session);
	return server->WSReceiveMessage(session, msg);
}

switch_status_t ws_read_frame(switch_core_session_t *session, switch_frame_t **frame, switch_io_flag_t flags, int stream_id) {
	WebSocketServer* server = (WebSocketServer *)switch_core_session_get_private(session);
	return server->WSReadFrame(session, frame, flags, stream_id);
}

switch_status_t ws_write_frame(switch_core_session_t *session, switch_frame_t *frame, switch_io_flag_t flags, int stream_id) {
	WebSocketServer* server = (WebSocketServer *)switch_core_session_get_private(session);
	return server->WSWriteFrame(session, frame, flags, stream_id);
}

switch_status_t ws_read_video_frame(switch_core_session_t *session, switch_frame_t **frame, switch_io_flag_t flags, int stream_id) {
	WebSocketServer* server = (WebSocketServer *)switch_core_session_get_private(session);
	return server->WSReadVideoFrame(session, frame, flags, stream_id);
}

switch_status_t ws_write_video_frame(switch_core_session_t *session, switch_frame_t *frame, switch_io_flag_t flags, int stream_id) {
	WebSocketServer* server = (WebSocketServer *)switch_core_session_get_private(session);
	return server->WSWriteVideoFrame(session, frame, flags, stream_id);
}
/***************************** 音视频读写接口 end **************************************/

WebSocketServer::WebSocketServer() {
	// TODO Auto-generated constructor stub
	ws_endpoint_interface = NULL;
	mpChannelHash = NULL;
	mpHashrwlock = NULL;
	mRunning = false;
	mAsyncIOServer.SetAsyncIOServerCallback(this);
}

WebSocketServer::~WebSocketServer() {
	// TODO Auto-generated destructor stub
}

bool WebSocketServer::Load(switch_memory_pool_t *pool, switch_loadable_module_interface_t* module_interface) {
	bool bFlag = false;

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "WebSocketServer::Load() \n");

	mRunning = true;

	if( module_interface ) {
		// 创建终端接口
		ws_endpoint_interface = (switch_endpoint_interface_t *) switch_loadable_module_create_interface(module_interface, SWITCH_ENDPOINT_INTERFACE);
		ws_endpoint_interface->interface_name = "ws";
		ws_endpoint_interface->io_routines = &ws_io_routines;
		ws_endpoint_interface->state_handler = &ws_state_handlers;
		bFlag = true;
	}

	if( bFlag ) {
		// 注册事件
		if (switch_event_reserve_subclass(WS_EVENT_MAINT) != SWITCH_STATUS_SUCCESS) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "WebSocketServer::Load( Couldn't register subclass %s ) \n", WS_EVENT_MAINT);
			bFlag = false;
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "WebSocketServer::Load( register subclass %s ) \n", WS_EVENT_MAINT);
		}
	}

	if( bFlag ) {
		// 读取配置
		bFlag = LoadConfig();
	}

	if( bFlag ) {
		// 启动服务
		char address[256] = {'\0'};
		strncpy(address, (const char*)mProfile.bind_address, sizeof(address) - 1);

		switch_port_t port = 8080;

		char *szport;
		if ((szport = strchr(address, ':'))) {
			*szport++ = '\0';
			port = atoi(szport);
		}

		bFlag = mAsyncIOServer.Start(pool, mProfile.handle_thread, address, port);
	}

	if( bFlag ) {
		// 创建会话列表
		switch_core_hash_init(&mpChannelHash);
		switch_thread_rwlock_create(&mpHashrwlock, pool);
	}

	if( bFlag ) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "WebSocketServer::Load( success ) \n");
	} else {
		Shutdown();
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "WebSocketServer::Load( fail ) \n");
	}

	return bFlag;
}

void WebSocketServer::Shutdown() {
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "WebSocketServer::Shutdown() \n");

	// 停止监听socket
	mAsyncIOServer.Stop();

	// 销毁会话列表
	if( mpChannelHash ) {
		switch_core_hash_destroy(&mpChannelHash);
	}

	switch_event_free_subclass(WS_EVENT_MAINT);

	mRunning = false;

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "WebSocketServer::Shutdown( finish ) \n");
}

bool WebSocketServer::IsRuning() {
	return mRunning;
}

void WebSocketServer::OnAccept(Client *client) {
	WSClientParser* parser = new WSClientParser();
	parser->SetWSClientParserCallback(this);
	parser->SetClient(client);
	client->parser = parser;
	parser->Connected();

	switch_log_printf(
			SWITCH_CHANNEL_UUID_LOG(parser->GetUUID()),
			SWITCH_LOG_NOTICE,
			"WebSocketServer::OnAccept( "
			"client : %p, "
			"parser : %p, "
			"uuid : %s "
			") \n",
			client,
			parser,
			parser->GetUUID()
			);
}

void WebSocketServer::OnDisconnect(Client* client) {
	WSClientParser* parser = (WSClientParser *)client->parser;

	switch_log_printf(
			SWITCH_CHANNEL_LOG,
			SWITCH_LOG_NOTICE,
			"WebSocketServer::OnDisconnect( "
			"client : %p, "
			"parser : %p "
			") \n",
			client,
			client->parser
			);

	if( parser ) {
		bool bIsAlreadyCall = false;
		WSChannel* wsChannel = NULL;
		// Hang up if make call already
		parser->Lock();
		// Mark client Disconnected
		parser->Disconnected();
		// Mark client Null
		parser->SetClient(NULL);
		// Mark parser hangup call
		bIsAlreadyCall = parser->IsAlreadyCall();
		if( !bIsAlreadyCall ) {
			// 会话不存在
			parser->Unlock();

			switch_log_printf(
					SWITCH_CHANNEL_UUID_LOG(parser->GetUUID()),
					SWITCH_LOG_NOTICE,
					"WebSocketServer::OnDisconnect( "
					"[Real delete parser], "
					"client : %p, "
					"parser : %p, "
					"user : '%s', "
					"domain : '%s', "
					"dest : '%s' "
					") \n",
					client,
					parser,
					parser->GetUser(),
					parser->GetDomain(),
					parser->GetDestNumber()
					);

			// 释放内存
			delete parser;
			client->parser = NULL;

		} else {
			// 存在会话
			parser->Unlock();

			switch_log_printf(
					SWITCH_CHANNEL_UUID_LOG(parser->GetUUID()),
					SWITCH_LOG_NOTICE,
					"WebSocketServer::OnDisconnect( "
					"[Waiting for session destroy], "
					"client : %p, "
					"parser : %p, "
					"wsChannel : %p, "
					"user : '%s', "
					"domain : '%s', "
					"dest : '%s' "
					") \n",
					client,
					parser,
					wsChannel,
					parser->GetUser(),
					parser->GetDomain(),
					parser->GetDestNumber()
					);
//			 临时解决, 延迟挂断, 否则会导致会议室模块死锁
//			switch_yield(switch_time_make(3, 0));

			// 挂断会话
			parser->Lock();
			HangupCall(parser);
			parser->Unlock();
		}

	}
}

void WebSocketServer::OnWSClientParserHandshake(WSClientParser* parser) {
	Client* client = (Client *)parser->GetClient();
	switch_log_printf(
			SWITCH_CHANNEL_UUID_LOG(parser->GetUUID()),
			SWITCH_LOG_NOTICE,
			"WebSocketServer::OnWSClientParserHandshake( "
			"parser : %p, "
			"client : %p, "
			"user : '%s', "
			"domain : '%s', "
			"dest : '%s' "
			") \n",
			parser,
			client,
			parser->GetUser(),
			parser->GetDomain(),
			parser->GetDestNumber()
			);

	// HankShake respond
	char* buffer = NULL;
	int datalen = 0;

	parser->Lock();
	parser->GetHandShakeRespond(&buffer, datalen);
	parser->Unlock();

	if( buffer != NULL && datalen > 0 ) {
		const char* data = (const char*)buffer;
		switch_size_t len = datalen;
		mAsyncIOServer.Send(client, data, &len);
	}

	bool bFlag = false;
	bool bDelete = false;

	parser->Lock();
	bFlag = parser->Login();
	if( bFlag ) {
		bFlag = CreateCall(parser);
	}

	if( !bFlag ) {
		// 断开连接
		bDelete = Disconnect(parser);
	}
	parser->Unlock();

	if( bDelete ) {
		// 释放内存
		delete parser;
	}
}

void WebSocketServer::OnWSClientParserData(WSClientParser* parser, const char* buffer, int len) {
	Client* client = (Client *)parser->GetClient();
//	switch_log_printf(
//			SWITCH_CHANNEL_UUID_LOG(parser->GetUUID()),
//			SWITCH_LOG_INFO,
//			"WebSocketServer::OnWSClientParserData( "
//			"parser : %p, "
//			"len : %d, "
//			"user : '%s', "
//			"domain : '%s', "
//			"dest : '%s' "
//			") \n",
//			parser,
//			len,
//			parser->GetUser(),
//			parser->GetDomain(),
//			parser->GetDestNumber()
//			);
}

void WebSocketServer::OnWSClientDisconected(WSClientParser* parser) {
	Client* client = (Client *)parser->GetClient();
	switch_log_printf(
			SWITCH_CHANNEL_UUID_LOG(parser->GetUUID()),
			SWITCH_LOG_NOTICE,
			"WebSocketServer::OnWSClientDisconected( "
			"parser : %p, "
			"user : '%s', "
			"domain : '%s', "
			"dest : '%s' "
			") \n",
			parser,
			parser->GetUser(),
			parser->GetDomain(),
			parser->GetDestNumber()
			);

	mAsyncIOServer.Disconnect(client);
}

bool WebSocketServer::LoadConfig() {
	bool bFlag = true;

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "WebSocketServer::LoadConfig() \n");

	const char *file = "ws.conf";
	switch_xml_t cfg, xml, x_profiles, x_profile;

	if (!(xml = switch_xml_open_cfg(file, &cfg, NULL))) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "WebSocketServer::LoadConfig( Could not open : %s ) \n", file);
		bFlag = false;
	}

	if( bFlag ) {
		if (!(x_profiles = switch_xml_child(cfg, "profiles"))) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "WebSocketServer::LoadConfig( Could not load profiles ) \n");
			bFlag = false;
		}
	}

	if( bFlag ) {
		bFlag = false;
		for (x_profile = switch_xml_child(x_profiles, "profile"); x_profile; x_profile = x_profile->next) {
			const char *name = switch_xml_attr_soft(x_profile, "name");
			memset(&mProfile, 0, sizeof(mProfile));
			strncpy(mProfile.name, name, sizeof(mProfile.name) - 1);
			if( LoadProfile(&mProfile, &x_profile) ) {
				bFlag = true;
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "WebSocketServer::LoadConfig( load profile : %s ) \n", name);
				break;
			} else {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "WebSocketServer::LoadConfig( Could not load profile : %s ) \n", name);
			}
		}
	}

	if( bFlag ) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "WebSocketServer::LoadConfig( success ) \n");
	} else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "WebSocketServer::LoadConfig( fail ) \n");
	}

	if (xml) {
		switch_xml_free(xml);
	}

	return bFlag;
}

bool WebSocketServer::LoadProfile(WSProfile* profile, switch_xml_t* x_profile, switch_bool_t reload) {
	bool bFlag = true;
	switch_xml_t x_settings;
	switch_event_t *event = NULL;

	if (!(x_settings = switch_xml_child(*x_profile, "settings"))) {
		bFlag = false;
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "WebSocketServer::LoadProfile( Could not load settings ) \n");
	}

	if( bFlag ) {
		int count = switch_event_import_xml(switch_xml_child(x_settings, "param"), "name", "value", &event);

		static switch_xml_config_int_options_t opt_bufferlen = {
			SWITCH_TRUE,
			0,
			SWITCH_TRUE,
			INT32_MAX
		};

		const char* context = NULL;
		const char* dialplan = NULL;
		const char* bind_address = NULL;

		switch_xml_config_item_t instructions[] = {
			/* parameter name        type                 reloadable   pointer                         default value     options structure */
			SWITCH_CONFIG_ITEM("context", SWITCH_CONFIG_STRING, CONFIG_RELOADABLE, &context, "public", &switch_config_string_strdup,
							   "", "The dialplan context to use for inbound calls"),
			SWITCH_CONFIG_ITEM("dialplan", SWITCH_CONFIG_STRING, CONFIG_RELOADABLE, &dialplan, "LUA", &switch_config_string_strdup,
							   "", "The dialplan to use for inbound calls"),
			SWITCH_CONFIG_ITEM("bind-address", SWITCH_CONFIG_STRING, CONFIG_REQUIRED, &bind_address, "0.0.0.0:8080", &switch_config_string_strdup,
							   "ip:port", "IP and port to bind"),
			SWITCH_CONFIG_ITEM("buffer-len", SWITCH_CONFIG_INT, CONFIG_REQUIRED, &profile->buffer_len, 500, &opt_bufferlen, "", "Length of the receiving buffer to be used by the ws clients, in miliseconds"),
			SWITCH_CONFIG_ITEM("handle-thread", SWITCH_CONFIG_INT, CONFIG_REQUIRED, &profile->handle_thread, 4, &opt_bufferlen, "", "Numbers of handle thread"),
			SWITCH_CONFIG_ITEM("connection-timeout", SWITCH_CONFIG_INT, CONFIG_RELOADABLE, &profile->connection_timeout, 5000, &opt_bufferlen, "", "Millisecond of connection timeout time without sending data"),
			// ------------------------
			SWITCH_CONFIG_ITEM_END()
		};

		switch_status_t status = switch_xml_config_parse_event(event, count, reload, instructions);
		bFlag = (status == SWITCH_STATUS_SUCCESS);

		if( context ) {
			strncpy(profile->context, context, sizeof(profile->context) - 1);
		}
		if( dialplan ) {
			strncpy(profile->dialplan, dialplan, sizeof(profile->dialplan) - 1);
		}
		if( bind_address ) {
			strncpy(profile->bind_address, bind_address, sizeof(profile->bind_address) - 1);
		}
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "WebSocketServer::LoadProfile( "
				"context : %s, "
				"dialplan : %s, "
				"bind_address : %s, "
				"buffer_len : %d, "
				"handle_thread : %d, "
				"connection_timeout : %d "
				") \n",
				profile->context,
				profile->dialplan,
				profile->bind_address,
				profile->buffer_len,
				profile->handle_thread,
				profile->connection_timeout
				);
	}

	if (event) {
		switch_event_destroy(&event);
	}

	return bFlag;
}

bool WebSocketServer::CreateCall(WSClientParser* parser) {
	bool bFlag = false;

	Client* client = (Client *)parser->GetClient();
	switch_log_printf(
			SWITCH_CHANNEL_UUID_LOG(parser->GetUUID()),
			SWITCH_LOG_NOTICE,
			"WebSocketServer::CreateCall( "
			"parser : %p, "
			"client : %p, "
			"user : '%s', "
			"domain : '%s', "
			"number : '%s' "
			") \n",
			parser,
			client,
			parser->GetUser(),
			parser->GetDomain(),
			parser->GetDestNumber()
			);

	switch_core_session_t *newsession = NULL;
	switch_channel_t* channel = NULL;
	WSChannel* wsChannel = NULL;

	if ( client != NULL ) {
		newsession = switch_core_session_request(ws_endpoint_interface, SWITCH_CALL_DIRECTION_INBOUND, SOF_NONE, NULL);
		if( newsession != NULL ) {
			bFlag = true;
		}
	}

	// 创建会话
	if( bFlag ) {
		switch_log_printf(
				SWITCH_CHANNEL_UUID_LOG(parser->GetUUID()),
				SWITCH_LOG_NOTICE,
				"WebSocketServer::CreateCall( "
				"[New session created], "
				"parser : %p, "
				"session : '%s', "
				"user : '%s', "
				"domain : '%s', "
				"number : '%s' "
				") \n",
				parser,
				switch_core_session_get_uuid(newsession),
				parser->GetUser(),
				parser->GetDomain(),
				parser->GetDestNumber()
				);
		switch_core_session_set_private(newsession, (void *)this);

		// 创建频道
		wsChannel = parser->CreateCall(
				newsession,
				mProfile.name,
				mProfile.context,
				mProfile.dialplan,
				client->socket->ip
				);

		if( wsChannel != NULL ) {
			bFlag = true;

		} else {
			switch_log_printf(
					SWITCH_CHANNEL_UUID_LOG(parser->GetUUID()),
					SWITCH_LOG_ERROR,
					"WebSocketServer::CreateCall( "
					"[Fail, Couldn't create channel], "
					"parser : %p, "
					"user : '%s', "
					"domain : '%s', "
					"number : '%s' "
					") \n",
					parser,
					parser->GetUser(),
					parser->GetDomain(),
					parser->GetDestNumber()
					);

			bFlag = false;
		}
	}

	if( bFlag ) {
		// 插入会话列表
		switch_core_hash_insert_wrlock(mpChannelHash, switch_core_session_get_uuid(newsession), wsChannel, mpHashrwlock);

		// 启动会话线程
		if (switch_core_session_thread_launch(newsession) == SWITCH_STATUS_FALSE) {
			switch_log_printf(
					SWITCH_CHANNEL_UUID_LOG(parser->GetUUID()),
					SWITCH_LOG_ERROR,
					"WebSocketServer::CreateCall( "
					"[Fail, Couldn't spawn thread], "
					"parser : %p, "
					"session : '%s', "
					"user : '%s', "
					"domain : '%s', "
					"number : '%s' "
					")\n",
					parser,
					switch_core_session_get_uuid(newsession),
					parser->GetUser(),
					parser->GetDomain(),
					parser->GetDestNumber()
					);
			bFlag = false;
		}
	}

	// 创建会话失败
	if( !bFlag ) {
		// 停止会话线程
		if( newsession ) {
			if (!switch_core_session_running(newsession) && !switch_core_session_started(newsession)) {
				switch_log_printf(
						SWITCH_CHANNEL_UUID_LOG(parser->GetUUID()),
						SWITCH_LOG_NOTICE,
						"WebSocketServer::CreateCall( "
						"[Session thread not launch], "
						"parser : %p, "
						"session : '%s', "
						"user : '%s', "
						"domain : '%s', "
						"number : '%s' "
						")\n",
						parser,
						switch_core_session_get_uuid(newsession),
						parser->GetUser(),
						parser->GetDomain(),
						parser->GetDestNumber()
						);

				// 会话线程没有启动
				switch_core_session_destroy(&newsession);

				// 从会话列表删除
				switch_core_hash_delete_wrlock(mpChannelHash, switch_core_session_get_uuid(newsession), mpHashrwlock);

			} else {
				// 会话线程已经启动, 挂断会话
				switch_log_printf(
						SWITCH_CHANNEL_UUID_LOG(parser->GetUUID()),
						SWITCH_LOG_NOTICE,
						"WebSocketServer::CreateCall( "
						"[Session thread already launch, hangup], "
						"parser : %p, "
						"session : '%s', "
						"user : '%s', "
						"domain : '%s', "
						"number : '%s' "
						")\n",
						parser,
						switch_core_session_get_uuid(newsession),
						parser->GetUser(),
						parser->GetDomain(),
						parser->GetDestNumber()
						);

				// 挂断会话
				HangupCall(parser);
			}
		}

	} else {
		switch_log_printf(
				SWITCH_CHANNEL_UUID_LOG(parser->GetUUID()),
				SWITCH_LOG_NOTICE,
				"WebSocketServer::CreateCall( "
				"[Success], "
				"parser : %p, "
				"session : '%s', "
				"wsChannel : %p, "
				"user : '%s', "
				"domain : '%s', "
				"number : '%s' "
				") \n",
				parser,
				switch_core_session_get_uuid(newsession),
				wsChannel,
				parser->GetUser(),
				parser->GetDomain(),
				parser->GetDestNumber()
				);
	}

	return bFlag;
}

bool WebSocketServer::HangupCall(WSClientParser* parser) {
	bool bFlag = false;

	if( parser ) {
		// 挂断会话
		bFlag = parser->HangupCall();
		if( bFlag ) {
			switch_log_printf(
					SWITCH_CHANNEL_UUID_LOG(parser->GetUUID()),
					SWITCH_LOG_NOTICE,
					"WebSocketServer::HangupCall( "
					"[Channel hang up], "
					"parser : %p, "
					"user : '%s', "
					"domain : '%s', "
					"dest : '%s' "
					") \n",
					parser,
					parser->GetUser(),
					parser->GetDomain(),
					parser->GetDestNumber()
					);
		} else {
			switch_log_printf(
					SWITCH_CHANNEL_UUID_LOG(parser->GetUUID()),
					SWITCH_LOG_NOTICE,
					"WebSocketServer::HangupCall( "
					"[No channel to hang up], "
					"parser : %p, "
					"user : '%s', "
					"domain : '%s', "
					"dest : '%s' "
					") \n",
					parser,
					parser->GetUser(),
					parser->GetDomain(),
					parser->GetDestNumber()
					);
		}
	}

	return bFlag;
}

bool WebSocketServer::Disconnect(WSClientParser* parser) {
	bool bFlag = false;
	Client* client = NULL;
	if( parser ) {
		client = (Client *)parser->GetClient();
		if( !parser->IsConnected() ) {
			// 连接已经断开
			switch_log_printf(
					SWITCH_CHANNEL_UUID_LOG(parser->GetUUID()),
					SWITCH_LOG_NOTICE,
					"WebSocketServer::Disconnect( "
					"[Real delete parser], "
					"parser : %p, "
					"client : %p, "
					"user : '%s', "
					"domain : '%s', "
					"dest : '%s' "
					") \n",
					parser,
					client,
					parser->GetUser(),
					parser->GetDomain(),
					parser->GetDestNumber()
					);

			bFlag = true;

		} else {
			// 连接还没断开
			switch_log_printf(
					SWITCH_CHANNEL_UUID_LOG(parser->GetUUID()),
					SWITCH_LOG_NOTICE,
					"WebSocketServer::Disconnect( "
					"[Disconnect for hangup], "
					"parser : %p, "
					"client : %p, "
					"user : '%s', "
					"domain : '%s', "
					"dest : '%s' "
					") \n",
					parser,
					client,
					parser->GetUser(),
					parser->GetDomain(),
					parser->GetDestNumber()
					);
			mAsyncIOServer.Disconnect(client);
		}

	} else {
		switch_log_printf(
				SWITCH_CHANNEL_LOG,
				SWITCH_LOG_ERROR,
				"WebSocketServer::Disconnect( "
				"[Disconnect error parser] "
				") \n"
				);
	}

	return bFlag;
}

/***************************** 频道路由和状态接口 **************************************/
switch_status_t WebSocketServer::WSOnInit(switch_core_session_t *session) {
	switch_channel_t *channel = switch_core_session_get_channel(session);
	switch_channel_set_flag(channel, CF_CNG_PLC);
	WSChannel* wsChannel = (WSChannel *)switch_core_hash_find_rdlock(mpChannelHash, switch_core_session_get_uuid(session), mpHashrwlock);

	switch_log_printf(
			SWITCH_CHANNEL_SESSION_LOG(session),
			SWITCH_LOG_INFO,
			"WebSocketServer::WSOnInit( "
			"parser : %p, "
			"channel : %p "
			") \n",
			wsChannel->parser,
			wsChannel
			);

	WSClientParser* parser = NULL;
	if( wsChannel ) {
		parser = (WSClientParser *)wsChannel->parser;
		if( parser ) {
			parser->Lock();
			switch_log_printf(
					SWITCH_CHANNEL_SESSION_LOG(session),
					SWITCH_LOG_NOTICE,
					"WebSocketServer::WSOnInit( "
					"parser : %p, "
					"channel : %p, "
					"user : '%s', "
					"domain : '%s', "
					"dest : '%s' "
					") \n",
					parser,
					wsChannel,
					parser->GetUser(),
					parser->GetDomain(),
					parser->GetDestNumber()
					);
			parser->Unlock();
		}
	}

	return SWITCH_STATUS_SUCCESS;
}

switch_status_t WebSocketServer::WSOnHangup(switch_core_session_t *session) {
	switch_channel_t *channel = switch_core_session_get_channel(session);
	WSChannel* wsChannel = (WSChannel *)switch_core_hash_find_rdlock(mpChannelHash, switch_core_session_get_uuid(session), mpHashrwlock);

	switch_log_printf(
			SWITCH_CHANNEL_SESSION_LOG(session),
			SWITCH_LOG_INFO,
			"WebSocketServer::WSOnHangup( "
			"parser : %p, "
			"channel : %p "
			") \n",
			wsChannel->parser,
			channel
			);

	WSClientParser* parser = NULL;
	if( wsChannel ) {
		parser = (WSClientParser *)wsChannel->parser;
		if( parser ) {
			parser->Lock();
			switch_log_printf(
					SWITCH_CHANNEL_SESSION_LOG(session),
					SWITCH_LOG_NOTICE,
					"WebSocketServer::WSOnHangup( "
					"parser : %p, "
					"channel : %p, "
					"user : '%s', "
					"domain : '%s', "
					"dest : '%s' "
					") \n",
					parser,
					wsChannel,
					parser->GetUser(),
					parser->GetDomain(),
					parser->GetDestNumber()
					);
			parser->Unlock();
		}
	}

	return SWITCH_STATUS_SUCCESS;
}

switch_status_t WebSocketServer::WSOnDestroy(switch_core_session_t *session) {
	WSChannel* wsChannel = (WSChannel *)switch_core_hash_find_rdlock(mpChannelHash, switch_core_session_get_uuid(session), mpHashrwlock);

	switch_log_printf(
			SWITCH_CHANNEL_SESSION_LOG(session),
			SWITCH_LOG_INFO,
			"WebSocketServer::WSOnDestroy( "
			"parser : %p, "
			"channel : %p "
			") \n",
			wsChannel->parser,
			wsChannel
			);

	WSClientParser* parser = NULL;
	if( wsChannel ) {
		parser = (WSClientParser *)wsChannel->parser;
		if( parser ) {
			switch_log_printf(
					SWITCH_CHANNEL_SESSION_LOG(session),
					SWITCH_LOG_NOTICE,
					"WebSocketServer::WSOnDestroy( "
					"parser : %p, "
					"channel : %p, "
					"user : '%s', "
					"domain : '%s', "
					"dest : '%s' "
					") \n",
					parser,
					wsChannel,
					parser->GetUser(),
					parser->GetDomain(),
					parser->GetDestNumber()
					);

			bool bDelete = false;

			parser->Lock();
			// 销毁会话
			parser->DestroyCall();
			// 断开连接
			bDelete = Disconnect(parser);
			parser->Unlock();

			if( bDelete ) {
				// 释放内存
				delete parser;
			}
		}
	}

	// 从会话列表删除
	switch_core_hash_delete_wrlock(mpChannelHash, switch_core_session_get_uuid(session), mpHashrwlock);

	return SWITCH_STATUS_SUCCESS;
}
/***************************** 频道路由和状态接口 end **************************************/

/***************************** 音视频读写接口 **************************************/
switch_call_cause_t WebSocketServer::WSOutgoingChannel(
		switch_core_session_t *session,
		switch_event_t *var_event,
		switch_caller_profile_t *outbound_profile,
		switch_core_session_t **newsession,
		switch_memory_pool_t **inpool,
		switch_originate_flag_t flags,
		switch_call_cause_t *cancel_cause
		) {
	WSChannel* wsChannel = (WSChannel *)switch_core_hash_find_rdlock(mpChannelHash, switch_core_session_get_uuid(session), mpHashrwlock);
//	Client* client = (Client *)wsChannel->parser->GetClient();
//	switch_log_printf(
//			SWITCH_CHANNEL_SESSION_LOG(session),
//			SWITCH_LOG_NOTICE,
//			"WebSocketServer::WSOutgoingChannel( "
//			"parser : %p, "
//			"wsChannel : %p, "
//			"session : %p, "
//			"user : '%s', "
//			"domain : '%s' "
//			") \n",
//			wsChannel->parser,
//			wsChannel,
//			session,
//			wsChannel->parser->GetUser(),
//			wsChannel->parser->GetDomain()
//			);

	switch_channel_t *channel = switch_core_session_get_channel(session);

	return SWITCH_CAUSE_SUCCESS;
}

switch_status_t WebSocketServer::WSReceiveMessage(switch_core_session_t *session, switch_core_session_message_t *msg) {
	WSChannel* wsChannel = (WSChannel *)switch_core_hash_find_rdlock(mpChannelHash, switch_core_session_get_uuid(session), mpHashrwlock);
	switch_channel_t *channel = switch_core_session_get_channel(session);

//	switch_log_printf(
//			SWITCH_CHANNEL_SESSION_LOG(session),
//			SWITCH_LOG_INFO,
//			"WebSocketServer::WSReceiveMessage( "
//			"parser : %p, "
//			"channel : %p "
//			") \n",
//			wsChannel->parser,
//			wsChannel
//			);

	switch (msg->message_id) {
	case SWITCH_MESSAGE_INDICATE_ANSWER:
		switch_channel_mark_answered(channel);
		break;
	case SWITCH_MESSAGE_INDICATE_RINGING:
		switch_channel_mark_ring_ready(channel);
		break;
	case SWITCH_MESSAGE_INDICATE_PROGRESS:
		switch_channel_mark_pre_answered(channel);
		break;
	case SWITCH_MESSAGE_INDICATE_HOLD:
	case SWITCH_MESSAGE_INDICATE_UNHOLD:
		break;
	case SWITCH_MESSAGE_INDICATE_BRIDGE:
		switch_log_printf(
				SWITCH_CHANNEL_SESSION_LOG(session),
				SWITCH_LOG_NOTICE,
				"WebSocketServer::WSReceiveMessage( "
				"[Flushing read buffer], "
				"wsChannel : %p "
				") \n",
				wsChannel
				);

		switch_mutex_lock(wsChannel->video_readbuf_mutex);
		switch_buffer_zero(wsChannel->video_readbuf);
		switch_mutex_unlock(wsChannel->video_readbuf_mutex);
		break;
	case SWITCH_MESSAGE_INDICATE_DISPLAY:
		{
			const char *name = msg->string_array_arg[0], *number = msg->string_array_arg[1];
			char *arg = NULL;
			char *argv[2] = { 0 };
			//int argc;

			if (zstr(name) && !zstr(msg->string_arg)) {
				arg = strdup(msg->string_arg);
				switch_assert(arg);

				switch_separate_string(arg, '|', argv, (sizeof(argv) / sizeof(argv[0])));
				name = argv[0];
				number = argv[1];

			}

			if (!zstr(name)) {
				if (zstr(number)) {
					switch_caller_profile_t *caller_profile = switch_channel_get_caller_profile(channel);
					number = caller_profile->destination_number;
				}
			}

			switch_safe_free(arg);
		}
		break;
	case SWITCH_MESSAGE_INDICATE_DEBUG_MEDIA:
		break;
	default:
		break;
	}

	return SWITCH_STATUS_SUCCESS;
}

switch_status_t WebSocketServer::WSReadFrame(switch_core_session_t *session, switch_frame_t **frame, switch_io_flag_t flags, int stream_id) {
	switch_channel_t *channel = switch_core_session_get_channel(session);
	WSChannel* wsChannel = (WSChannel *)switch_core_hash_find_rdlock(mpChannelHash, switch_core_session_get_uuid(session), mpHashrwlock);

	Client* client = NULL;
	WSClientParser* parser = NULL;
	if( wsChannel ) {
		parser = (WSClientParser *)wsChannel->parser;
		if( parser ) {
		}

		wsChannel->read_frame.codec = &wsChannel->read_codec;

		wsChannel->read_frame.datalen = 2;
		switch_byte_t* data = (switch_byte_t *)wsChannel->read_frame.data;
		data[0] = 65;
		data[1] = 0;

		*frame = &wsChannel->read_frame;
	}

	switch_yield(20000);

	return SWITCH_STATUS_SUCCESS;
}

switch_status_t WebSocketServer::WSWriteFrame(switch_core_session_t *session, switch_frame_t *frame, switch_io_flag_t flags, int stream_id) {
	WSChannel* wsChannel = (WSChannel *)switch_core_hash_find_rdlock(mpChannelHash, switch_core_session_get_uuid(session), mpHashrwlock);

	Client* client = NULL;
	WSClientParser* parser = NULL;
	if( wsChannel ) {
		parser = (WSClientParser *)wsChannel->parser;
		if( parser ) {
		}
	}

	switch_channel_t *channel = switch_core_session_get_channel(session);
	return SWITCH_STATUS_SUCCESS;
}

switch_status_t WebSocketServer::WSReadVideoFrame(switch_core_session_t *session, switch_frame_t **frame, switch_io_flag_t flags, int stream_id) {
	switch_channel_t *channel = switch_core_session_get_channel(session);
	WSChannel* wsChannel = (WSChannel *)switch_core_hash_find_rdlock(mpChannelHash, switch_core_session_get_uuid(session), mpHashrwlock);

	Client* client = NULL;
	WSClientParser* parser = NULL;
	if( wsChannel ) {
		parser = (WSClientParser *)wsChannel->parser;
		if( parser ) {
		}

		wsChannel->video_read_frame.datalen = 0;
		wsChannel->video_read_frame.flags = SFF_CNG;
		wsChannel->video_read_frame.codec = &wsChannel->video_read_codec;

		*frame = &wsChannel->video_read_frame;
	}

	switch_yield(20000);

	return SWITCH_STATUS_SUCCESS;
}

switch_status_t WebSocketServer::WSWriteVideoFrame(switch_core_session_t *session, switch_frame_t *frame, switch_io_flag_t flags, int stream_id) {
	switch_channel_t *channel = switch_core_session_get_channel(session);
	WSChannel* wsChannel = (WSChannel *)switch_core_hash_find_rdlock(mpChannelHash, switch_core_session_get_uuid(session), mpHashrwlock);

	Client* client = NULL;
	WSClientParser* parser = NULL;
	if( wsChannel ) {
		parser = (WSClientParser *)wsChannel->parser;
		if( parser ) {
			parser->Lock();
			client = (Client *)parser->GetClient();

			switch_byte_t *payload = (switch_byte_t *)frame->data;
			int nalType = payload[0] & 0x1f;

			const char* data = NULL;
			switch_size_t len = 0;
			switch_size_t dataLen = 0;

			// Get H264 frame
			if( parser->GetFrame(frame, &data, &dataLen) ) {
				// Get frame success
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG,
						"mod_ws: WebSocketServer::WSWriteVideoFrame(), "
						"timestamp: %d, dataLen: %d \n",
						frame->timestamp, dataLen);

				// Get WebSocket packet
				char header[WS_HEADER_MAX_LEN] = {0};
				WSPacket* packet = (WSPacket *)header;
				parser->GetPacket(packet, dataLen);

				// Send WebSocket header
				len = packet->GetHeaderLength();
				if( client ) {
					if( mAsyncIOServer.Send(client, (const char *)packet, &len) ) {
						// Send playload
						mAsyncIOServer.Send(client, data, &dataLen);
					}
				}

				// Release video buffer
				parser->DestroyVideoBuffer();
			}
			parser->Unlock();
		}
	}

	return SWITCH_STATUS_SUCCESS;
}
/***************************** 音视频读写接口 end **************************************/
