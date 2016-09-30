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

/***************************** 模块接口 **************************************/
/*
 * 加载WebSocket模块
 * Macro expands to: switch_status_t mod_ws_load()
 */
SWITCH_MODULE_LOAD_FUNCTION(mod_ws_load)
{
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CONSOLE, "mod_ws_load() \n");

	// 创建模块
	*module_interface = switch_loadable_module_create_module_interface(pool, modname);

	// 加载WebSocketServer
	server.Load(pool, *module_interface);

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
