/*
 * mod_w.cpp
 *
 *  Created on: 2016-09-12
 *      Author: Max.chiu
 * Description: Websocket的freeswitch自定义模块
 */

#include <switch.h>
#include "WebSocketServer.h"

/**
 * 声明全局变量
 */
static WebSocketServer server;

/**
 * switch_mod模块接口
 */
SWITCH_MODULE_LOAD_FUNCTION(mod_ws_load);
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_ws_shutdown);
SWITCH_MODULE_RUNTIME_FUNCTION(mod_ws_runtime);
SWITCH_MODULE_DEFINITION(mod_ws, mod_ws_load, mod_ws_shutdown, mod_ws_runtime);

/**
 * 终端接口
 */
switch_endpoint_interface_t *ws_endpoint_interface = NULL;

/**
 * 频道路由和状态接口
 */
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

/**
 * 音视频读写接口
 */
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
switch_status_t ws_write_video_frame(switch_core_session_t *session, switch_frame_t *frame, switch_io_flag_t flags, int stream_id);

switch_io_routines_t ws_io_routines = {
	/*.outgoing_channel */ ws_outgoing_channel,
	/*.read_frame */ NULL,
	/*.write_frame */ NULL,
	/*.kill_channel */ NULL,
	/*.send_dtmf */ NULL,
	/*.receive_message */ ws_receive_message,
	/*.receive_event */ NULL,
	/*.state_change*/ NULL,
	/*.ws_read_vid_frame */ NULL,
	/*.ws_write_vid_frame */ ws_write_video_frame
};

/***************************** 模块接口 **************************************/
/*
 * 加载WebSocket模块
 * Macro expands to: switch_status_t mod_ws_load()
 */
SWITCH_MODULE_LOAD_FUNCTION(mod_ws_load)
{
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "mod_ws_load() \n");

	// 创建模块
	*module_interface = switch_loadable_module_create_module_interface(pool, modname);

	// 创建模块接口
	ws_endpoint_interface = (switch_endpoint_interface_t *) switch_loadable_module_create_interface(*module_interface, SWITCH_ENDPOINT_INTERFACE);
	ws_endpoint_interface->interface_name = "ws";
	ws_endpoint_interface->io_routines = &ws_io_routines;
	ws_endpoint_interface->state_handler = &ws_state_handlers;

	// 加载WebSocketServer
	server.Load(pool);

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "mod_ws_load() finish \n");

	return SWITCH_STATUS_SUCCESS;
}

/**
 * 卸载WebSocket模块
 * Macro expands to: switch_status_t mod_ws_shutdown()
 */
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_ws_shutdown)
{
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "mod_ws_shutdown() \n");
	// 卸载WebSocketServer
	server.Shutdown();
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "mod_ws_shutdown() finish \n");

	return SWITCH_STATUS_SUCCESS;
}

SWITCH_MODULE_RUNTIME_FUNCTION(mod_ws_runtime)
{
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "mod_ws_runtime() \n");

	// 等待WebSocketServer停止
	while( server.IsRuning() ) {
		switch_yield(500 * 1000);
	}

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "mod_ws_runtime() finish \n");

	return SWITCH_STATUS_TERM;
}
/***************************** 模块接口 end **************************************/

/***************************** 频道路由和状态接口 **************************************/
switch_status_t ws_on_init(switch_core_session_t *session)
{
	switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_INFO, "ws_on_init() session : %p \n", (void *)session);
	return SWITCH_STATUS_SUCCESS;
}

switch_status_t ws_on_hangup(switch_core_session_t *session)
{
	switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_INFO, "ws_on_hangup() session : %p \n", (void *)session);
	return SWITCH_STATUS_SUCCESS;
}

switch_status_t ws_on_destroy(switch_core_session_t *session)
{
	switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_INFO, "ws_on_destroy() session : %p \n", (void *)session);
	return SWITCH_STATUS_SUCCESS;
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
	switch_call_cause_t cause = SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER;
//	char *destination = NULL;

	if (zstr(outbound_profile->destination_number)) {
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR, "ws_outgoing_channel() session : %p, No destination \n", (void *)session);
		goto fail;
	}

//	destination = strdup(outbound_profile->destination_number);

	cause = SWITCH_CAUSE_SUCCESS;

fail:
	return cause;
}

switch_status_t ws_receive_message(switch_core_session_t *session, switch_core_session_message_t *msg)
{
	switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR, "ws_receive_message() session : %p \n", (void *)session);
	return SWITCH_STATUS_SUCCESS;
}

switch_status_t ws_write_video_frame(switch_core_session_t *session, switch_frame_t *frame, switch_io_flag_t flags, int stream_id)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR, "ws_write_video_frame() session : %p \n", (void *)session);
	return status;
}
/***************************** 音视频读写接口 end **************************************/
