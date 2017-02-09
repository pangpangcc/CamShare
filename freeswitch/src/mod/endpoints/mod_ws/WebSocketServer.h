/*
 * WebSocketServer.h
 *
 *  Created on: 2016年9月14日
 *      Author: max
 */

#ifndef SRC_MOD_ENDPOINTS_MOD_WS_WEBSOCKETSERVER_H_
#define SRC_MOD_ENDPOINTS_MOD_WS_WEBSOCKETSERVER_H_

#include "AsyncIOServer.h"
#include "WSClientParser.h"

typedef struct WSChannel;

typedef struct WSProfile {
	char name[128];					/* < Profile name */
	char context[1024];		/* < Default dialplan name */
	char dialplan[1024];		/* < Default dialplan context */
	char bind_address[1024];	/* < Bind address */
	int buffer_len;					/* < Receive buffer length the flash clients should use */
	int handle_thread;				/* < Numbers of handle thread */
	int connection_timeout;			/* < Millisecond of connection timeout time without sending data */
} WSProfile;

class WebSocketServer : public AsyncIOServerCallback, WSClientParserCallback {
public:
	WebSocketServer();
	virtual ~WebSocketServer();

	bool Load(switch_memory_pool_t *pool, switch_loadable_module_interface_t* module_interface);
	void Shutdown();
	bool IsRuning();

	/**
	 *	AsyncIOServerCallback Implement
	 */
	void OnAccept(Client *client);
	void OnDisconnect(Client* socket);

	/**
	 *	WSClientParserCallback Implement
	 */
	void OnWSClientParserHandshake(WSClientParser* parser);
	void OnWSClientParserData(WSClientParser* parser, const char* buffer, int len);
	void OnWSClientDisconected(WSClientParser* parser);

	/**
	 * 频道路由和状态接口
	 */
	switch_status_t WSOnInit(switch_core_session_t *session);
	switch_status_t WSOnHangup(switch_core_session_t *session);
	switch_status_t WSOnDestroy(switch_core_session_t *session);

	/**
	 * 音视频读写接口
	 */
	switch_call_cause_t WSOutgoingChannel(
			switch_core_session_t *session,
			switch_event_t *var_event,
			switch_caller_profile_t *outbound_profile,
			switch_core_session_t **newsession,
			switch_memory_pool_t **inpool,
			switch_originate_flag_t flags,
			switch_call_cause_t *cancel_cause
			);
	switch_status_t WSReceiveMessage(switch_core_session_t *session, switch_core_session_message_t *msg);
	switch_status_t WSReadFrame(switch_core_session_t *session, switch_frame_t **frame, switch_io_flag_t flags, int stream_id);
	switch_status_t WSWriteFrame(switch_core_session_t *session, switch_frame_t *frame, switch_io_flag_t flags, int stream_id);
	switch_status_t WSReadVideoFrame(switch_core_session_t *session, switch_frame_t **frame, switch_io_flag_t flags, int stream_id);
	switch_status_t WSWriteVideoFrame(switch_core_session_t *session, switch_frame_t *frame, switch_io_flag_t flags, int stream_id);

private:
	/**
	 * 加载配置
	 */
	bool LoadConfig();
	bool LoadProfile(WSProfile* profile, switch_xml_t* x_profile, switch_bool_t reload = SWITCH_FALSE);

	/**
	 * 请求创建会话
	 */
	bool CreateCall(WSClientParser* parser);

	/**
	 * 挂断会话
	 */
	bool HangupCall(WSClientParser* parser);

	/**
	 * 断开连接
	 */
	bool Disconnect(WSClientParser* parser);

	/**
	 * 是否运行
	 */
	bool mRunning;

	/**
	 * 终端接口
	 */
	switch_endpoint_interface_t* ws_endpoint_interface;
	/**
	 * 会话列表
	 */
	switch_hash_t *mpChannelHash;
	switch_thread_rwlock_t *mpHashrwlock;

	/**
	 * 异步IO服务器
	 */
	AsyncIOServer mAsyncIOServer;

	/**
	 * 配置文件
	 */
	WSProfile mProfile;
};

#endif /* SRC_MOD_ENDPOINTS_MOD_WS_WEBSOCKETSERVER_H_ */
