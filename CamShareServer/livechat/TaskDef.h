/*
 * author: Samson.Fan
 *   date: 2015-03-25
 *   file: TaskDef.h
 *   desc: 记录Task的定义
 */

#pragma once

// 使用协议定义
typedef enum {
	NOHEAD_PROTOCOL = -1,   // 只有长度没有包头
	JSON_PROTOCOL = 0,      // json协议
	AMF_PROTOCOL = 1        // amf协议
} TASK_PROTOCOL_TYPE;

// 命令定义
typedef enum {
	TCMD_UNKNOW			    		= 0,	// 未知命令
	// 客户端主动请求命令
	TCMD_CHECKVER		    		= -1,	// 版本检测
	TCMD_SENDENTERCONFERENCE		= 244,	// 进入会议室认证
	TCMD_SENDMSG					= 246,	// 发送消息到客户端
	// 服务器主动请求命令
	TCMD_RECVENTERCONFERENCE		= 243,	// 进入会议室认证结果
	TCMD_RECVKICKUSERFROMCONFERENCE	= 254,	// 从会议室踢出用户
} TASK_CMD_TYPE;

// 判断是否客户端主动请求的命令
inline bool IsRequestCmd(int cmd)
{
	bool result = false;
	switch (cmd) {
	case TCMD_CHECKVER:				// 版本检测
	case TCMD_SENDENTERCONFERENCE:	// 进入会议室认证
		result = true;				// 主动请求的命令
		break;
	case TCMD_SENDMSG:				// 发送消息到客户端
		result = true;				// 主动请求的命令
		break;
	default:
		break;
	}
	return result;
}
