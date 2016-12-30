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

#define WS_MOD_VERSION "1.0.1"

/**
 * switch_mod模块接口
 */
SWITCH_MODULE_LOAD_FUNCTION(mod_ws_load);
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_ws_shutdown);
SWITCH_MODULE_RUNTIME_FUNCTION(mod_ws_runtime);
SWITCH_MODULE_DEFINITION(mod_ws, mod_ws_load, mod_ws_shutdown, mod_ws_runtime);

/***************************** API接口 **************************************/
#define WS_FUNCTION_SYNTAX "version\n"
#define WS_FUNCTION_VERSION "version"
SWITCH_STANDARD_API(ws_function)
{
	int argc;
	char *argv[10];
	char *dup = NULL;

	if (zstr(cmd)) {
		goto usage;
	}

	dup = strdup(cmd);
	argc = switch_split(dup, ' ', argv);

	if (argc < 1 || zstr(argv[0])) {
		goto usage;
	}

	// 返回版本号
	if (!strcmp(argv[0], WS_FUNCTION_VERSION)) {
		stream->write_function(stream, "%s\n", WS_MOD_VERSION);
	}

	goto done;

usage:
	stream->write_function(stream, "-ERR Usage: "WS_FUNCTION_SYNTAX"\n");

done:
	switch_safe_free(dup);
	return SWITCH_STATUS_SUCCESS;
}

/***************************** API接口 end **************************************/

/***************************** 模块接口 **************************************/
/*
 * 加载WebSocket模块
 * Macro expands to: switch_status_t mod_ws_load()
 */
SWITCH_MODULE_LOAD_FUNCTION(mod_ws_load)
{
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CONSOLE, "mod_ws_load( version : [%s] ) \n", WS_MOD_VERSION);

	switch_api_interface_t *api_interface;

	// 创建模块
	*module_interface = switch_loadable_module_create_module_interface(pool, modname);

	// 加载WebSocketServer
	server.Load(pool, *module_interface);

	// 创建命令
	SWITCH_ADD_API(api_interface, "ws", "ws management", ws_function, WS_FUNCTION_SYNTAX);

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CONSOLE, "mod_ws_load() finish \n");

	return SWITCH_STATUS_SUCCESS;
}

/**
 * 卸载WebSocket模块
 * Macro expands to: switch_status_t mod_ws_shutdown()
 */
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_ws_shutdown)
{
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CONSOLE, "mod_ws_shutdown() \n");
	// 卸载WebSocketServer
	server.Shutdown();
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CONSOLE, "mod_ws_shutdown() finish \n");

	return SWITCH_STATUS_SUCCESS;
}

SWITCH_MODULE_RUNTIME_FUNCTION(mod_ws_runtime)
{
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CONSOLE, "mod_ws_runtime() \n");

	// 等待WebSocketServer停止
	while( server.IsRuning() ) {
		switch_yield(500 * 1000);
	}

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CONSOLE, "mod_ws_runtime() finish \n");

	return SWITCH_STATUS_TERM;
}
/***************************** 模块接口 end **************************************/
