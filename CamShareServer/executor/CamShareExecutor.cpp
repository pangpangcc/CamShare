/*
 * CamShareExecutor.cpp
 *
 *  Created on: 2017年1月16日
 *      Author: max
 */

#include "CamShareExecutor.h"

#include "respond/ExecCmdRespond.h"

CamShareExecutor::CamShareExecutor() :
mServerMutex(KMutex::MutexType_Recursive)
{
	// TODO Auto-generated constructor stub
	// 基本参数
	miPort = 0;
	miMaxClient = 0;
	miMaxHandleThread = 0;
	miTimeout = 0;

	// 日志参数
	miDebugMode = 0;
	miLogLevel = 0;

	// 其他
	mRunning = false;

	mAsyncIOServer.SetAsyncIOServerCallback(this);
}

CamShareExecutor::~CamShareExecutor() {
	// TODO Auto-generated destructor stub
}

bool CamShareExecutor::Start(const string& config) {
	if( config.length() > 0 ) {
		mConfigFile = config;

		// LoadConfig config
		if( LoadConfig() ) {
			return Start();

		} else {
			printf("# CamShare Executor can not load config file exit. \n");
		}

	} else {
		printf("# CamShare Executor has no config file can be use exit. \n");
	}

	return false;
}

bool CamShareExecutor::Start() {
	bool bFlag = false;

	LogManager::GetLogManager()->Start(miLogLevel, mLogDir);
	LogManager::GetLogManager()->SetDebugMode(miDebugMode);
	LogManager::GetLogManager()->LogSetFlushBuffer(1 * BUFFER_SIZE_1K * BUFFER_SIZE_1K);

	LogManager::GetLogManager()->Log(
			LOG_ERR_SYS,
			"CamShareExecutor::Start( "
			"############## CamShare Executor ############## "
			")"
			);

	LogManager::GetLogManager()->Log(
			LOG_ERR_SYS,
			"CamShareExecutor::Start( "
			"# Version : %s, "
			"Build date : %s %s "
			")",
			VERSION_STRING,
			__DATE__,
			__TIME__
			);

	// 基本参数
	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"CamShareExecutor::Start( "
			"[Start], "
			"miPort : %d, "
			"miMaxClient : %d "
			")",
			miPort,
			miMaxClient
			);

	// 日志参数
	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"CamShareExecutor::Start( "
			"miDebugMode : %d, "
			"miLogLevel : %d, "
			"mlogDir : %s "
			")",
			miDebugMode,
			miLogLevel,
			mLogDir.c_str()
			);

	mServerMutex.lock();
	if( mRunning ) {
		Stop();
	}
	mRunning = true;

	bFlag = mAsyncIOServer.Start(miPort, miMaxClient, miMaxHandleThread);

	if( bFlag ) {
		LogManager::GetLogManager()->Log(
				LOG_WARNING,
				"CamShareExecutor::Start( "
				"[OK] "
				")"
				);
	} else {
		LogManager::GetLogManager()->Log(
				LOG_ERR_SYS,
				"CamShareExecutor::Start( "
				"[Fail] "
				")"
				);
		Stop();
	}
	mServerMutex.unlock();

	return bFlag;
}

void CamShareExecutor::Stop() {
	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"CamShareExecutor::Stop( "
			")"
			);

	mServerMutex.lock();

	if( mRunning ) {
		mRunning = false;

		// 停止监听socket
		mAsyncIOServer.Stop();

	}

	mServerMutex.unlock();

	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"CamShareExecutor::Stop( "
			"[OK] "
			")"
			);
}

bool CamShareExecutor::IsRunning() {
	return mRunning;
}

bool CamShareExecutor::LoadConfig() {
	bool bFlag = false;
	mConfigMutex.lock();
	if( mConfigFile.length() > 0 ) {
		ConfFile conf;
		conf.InitConfFile(mConfigFile.c_str(), "");
		if ( conf.LoadConfFile() ) {
			// 基本参数
			miPort = atoi(conf.GetPrivate("BASE", "PORT", "9201").c_str());
			miMaxClient = atoi(conf.GetPrivate("BASE", "MAXCLIENT", "100000").c_str());
			miMaxHandleThread = atoi(conf.GetPrivate("BASE", "MAXHANDLETHREAD", "4").c_str());
			miTimeout = atoi(conf.GetPrivate("BASE", "TIMEOUT", "10").c_str());

			// 日志参数
			miLogLevel = atoi(conf.GetPrivate("LOG", "LOGLEVEL", "5").c_str());
			mLogDir = conf.GetPrivate("LOG", "LOGDIR", "log");
			miDebugMode = atoi(conf.GetPrivate("LOG", "DEBUGMODE", "0").c_str());

			bFlag = true;
		}
	}
	mConfigMutex.unlock();
	return bFlag;
}

bool CamShareExecutor::ReloadLogConfig() {
	bool bFlag = false;
	mConfigMutex.lock();
	if( mConfigFile.length() > 0 ) {
		ConfFile conf;
		conf.InitConfFile(mConfigFile.c_str(), "");
		if ( conf.LoadConfFile() ) {
			// 日志参数
			miLogLevel = atoi(conf.GetPrivate("LOG", "LOGLEVEL", "5").c_str());
			miDebugMode = atoi(conf.GetPrivate("LOG", "DEBUGMODE", "0").c_str());

			LogManager::GetLogManager()->SetLogLevel(miLogLevel);
			LogManager::GetLogManager()->SetDebugMode(miDebugMode);
			LogManager::GetLogManager()->LogSetFlushBuffer(1 * BUFFER_SIZE_1K * BUFFER_SIZE_1K);

			bFlag = true;
		}
	}
	mConfigMutex.unlock();
	return bFlag;
}

bool CamShareExecutor::SendRespond2Client(
		HttpParser* parser,
		IRespond* respond
		) {
	bool bFlag = false;
	Client* client = (Client *)parser->custom;

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"CamShareExecutor::SendRespond2Client( "
			"[内部服务(HTTP), 返回请求到客户端], "
			"parser : %p, "
			"client : %p, "
			"respond : %p "
			")",
			parser,
			client,
			respond
			);

	// 发送头部
	char buffer[MAX_BUFFER_LEN];
	snprintf(
			buffer,
			MAX_BUFFER_LEN - 1,
			"HTTP/1.1 200 OK\r\n"
			"Connection:Keep-Alive\r\n"
			"Content-Type:text/html; charset=utf-8\r\n"
			"\r\n"
			);
	int len = strlen(buffer);

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"CamShareExecutor::SendRespond2Client( "
			"[内部服务(HTTP), 返回请求头部到客户端], "
			"parser : %p, "
			"client : %p, "
			"buffer :\n%s\n"
			")",
			parser,
			client,
			buffer
			);

	mAsyncIOServer.Send(client, buffer, len);

	if( respond != NULL ) {
		// 发送内容
		bool more = false;
		while( true ) {
			len = respond->GetData(buffer, MAX_BUFFER_LEN, more);
			LogManager::GetLogManager()->Log(
					LOG_WARNING,
					"CamShareExecutor::SendRespond2Client( "
					"[内部服务(HTTP), 返回请求内容到客户端], "
					"parser : %p, "
					"client : %p, "
					"buffer :\n%s\n"
					")",
					parser,
					client,
					buffer
					);

			mAsyncIOServer.Send(client, buffer, len);

			if( !more ) {
				// 全部发送完成
				bFlag = true;
				break;
			}
		}
//		delete respond;
	}

	if( !bFlag ) {
		LogManager::GetLogManager()->Log(
				LOG_WARNING,
				"CamShareExecutor::SendRespond2Client( "
				"[内部服务(HTTP), 返回请求到客户端, 失败], "
				"parser : %p, "
				"client : %p "
				")",
				parser,
				client
				);
	}

	return bFlag;
}

bool CamShareExecutor::OnAccept(Client *client) {
	HttpParser* parser = new HttpParser();
	parser->SetCallback(this);
	parser->custom = client;
	client->parser = parser;

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"CamShareExecutor::OnAccept( "
			"parser : %p, "
			"client : %p "
			")",
			parser,
			client
			);

	return true;
}

void CamShareExecutor::OnDisconnect(Client* client) {
	HttpParser* parser = (HttpParser *)client->parser;

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"CamShareExecutor::OnDisconnect( "
			"parser : %p, "
			"client : %p "
			")",
			parser,
			client
			);

	if( parser ) {
		delete parser;
		client->parser = NULL;
	}
}

void CamShareExecutor::OnHttpParserHeader(HttpParser* parser) {
	Client* client = (Client *)parser->custom;

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"CamShareExecutor::OnHttpParserHeader( "
			"parser : %p, "
			"client : %p, "
			"path : %s "
			")",
			parser,
			client,
			parser->GetPath().c_str()
			);

	bool bFlag = false;
	if( parser->GetPath() == "/EXEC" ) {
		int ret = -1;
		ExecCmdRespond rsp;
		const string cmd = parser->GetParam("CMD");
		if( cmd.length() > 0 ) {
			ret = system(cmd.c_str());
			rsp.SetParam(ret);
		}

		if( ret != -1 ) {
			LogManager::GetLogManager()->Log(
					LOG_WARNING,
					"CamShareExecutor::OnHttpParserHeader( "
					"[内部服务(HTTP), 收到请求:执行命令], "
					"parser : %p, "
					"client : %p, "
					"cmd : %s "
					")",
					parser,
					client,
					cmd.c_str()
					);
		} else {
			LogManager::GetLogManager()->Log(
					LOG_ERR_USER,
					"CamShareExecutor::OnHttpParserHeader( "
					"[内部服务(HTTP), 收到请求:执行命令, 失败], "
					"parser : %p, "
					"client : %p, "
					"cmd : %s, "
					"ret : %d "
					")",
					parser,
					client,
					cmd.c_str(),
					ret
					);
		}

		SendRespond2Client(parser, &rsp);
	} else {
		LogManager::GetLogManager()->Log(
				LOG_WARNING,
				"CamShareExecutor::OnHttpParserHeader( "
				"[内部服务(HTTP), 收到请求:未知请求], "
				"parser : %p, "
				"client : %p "
				")",
				parser,
				client
				);
	}

	mAsyncIOServer.Disconnect(client);
}

void CamShareExecutor::OnHttpParserError(HttpParser* parser) {
	Client* client = (Client *)parser->custom;

	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"CamShareExecutor::OnHttpParserError( "
			"parser : %p, "
			"client : %p "
			")",
			parser,
			client
			);

	mAsyncIOServer.Disconnect(client);
}
