/*
 * CamShareExecutor.h
 *
 *  Created on: 2017年1月16日
 *      Author: max
 */

#ifndef EXECUTOR_CAMSHAREEXECUTOR_H_
#define EXECUTOR_CAMSHAREEXECUTOR_H_

#include <server/AsyncIOServer.h>

#include <common/ConfFile.hpp>

#include <parser/HttpParser.h>
#include <respond/IRespond.h>

#define VERSION_STRING "1.0.0"

class CamShareExecutor :
		public AsyncIOServerCallback,
		public HttpParserCallback {
public:
	CamShareExecutor();
	virtual ~CamShareExecutor();

	bool Start(const string& config);
	bool Start();
	void Stop();
	bool IsRunning();

public:
	// AsyncIOServerCallback
	bool OnAccept(Client *client);
	void OnDisconnect(Client* client);

	// HttpParserCallback
	void OnHttpParserHeader(HttpParser* parser);
	void OnHttpParserError(HttpParser* parser);

private:
	/**
	 * 加载配置
	 */
	bool LoadConfig();
	/**
	 * 重新读取日志等级
	 */
	bool ReloadLogConfig();

	/**
	 * 内部服务(HTTP), 发送请求响应
	 * @param client		客户端
	 * @param respond		响应实例
	 * @return true:发送成功/false:发送失败
	 */
	bool SendRespond2Client(
			HttpParser* parser,
			IRespond* respond
			);

	/**
	 * 异步TCP服务
	 */
	AsyncIOServer mAsyncIOServer;

	KMutex mServerMutex;
	bool mRunning;

	/**
	 * 配置文件锁
	 */
	KMutex mConfigMutex;
	/**
	 * 配置文件
	 */
	string mConfigFile;

	/***************************** 基本参数 **************************************/
	/**
	 * 监听端口
	 */
	short miPort;

	/**
	 * 最大连接数
	 */
	int miMaxClient;

	/**
	 * 处理线程数目
	 */
	int miMaxHandleThread;

	/**
	 * 请求超时(秒)
	 */
	unsigned int miTimeout;
	/***************************** 基本参数 end **************************************/

	/***************************** 日志参数 **************************************/
	/**
	 * 日志等级
	 */
	int miLogLevel;

	/**
	 * 日志路径
	 */
	string mLogDir;

	/**
	 * 是否debug模式
	 */
	int miDebugMode;
	/***************************** 日志参数 end **************************************/
};

#endif /* EXECUTOR_CAMSHAREEXECUTOR_H_ */
