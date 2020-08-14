/*
 * CamShareMiddleware.cpp
 *
 *  Created on: 2015-1-13
 *      Author: Max.Chiu
 *      Email: Kingsleyyau@gmail.com
 */

#include "CamShareMiddleware.h"

#include <sys/syscall.h>
#include <algorithm>

#include <simulatorchecker/SimulatorProtocolTool.h>

#include <httpclient/HttpClient.h>

#include <request/SendMsgEnterConferenceRequest.h>
#include <request/SendMsgExitConferenceRequest.h>
#include <request/OnlineStatusRequest.h>
#include <request/OnlineListRequest.h>

#include <respond/BaseRespond.h>
#include <respond/BaseResultRespond.h>
#include <respond/ReSendErrorRecordRespond.h>

#include <json/json.h>

/***************************** 线程处理 **************************************/
/**
 * 状态监视线程
 */
class StateRunnable : public KRunnable {
public:
	StateRunnable(CamShareMiddleware *container) {
		mContainer = container;
	}
	virtual ~StateRunnable() {
		mContainer = NULL;
	}
protected:
	void onRun() {
		mContainer->StateHandle();
	}
private:
	CamShareMiddleware *mContainer;
};

/**
 * 连接Livechat线程
 */
class ConnectLiveChatRunnable : public KRunnable {
public:
	ConnectLiveChatRunnable(CamShareMiddleware *container) {
		mContainer = container;
	}
	virtual ~ConnectLiveChatRunnable() {
		mContainer = NULL;
	}
protected:
	void onRun() {
		mContainer->ConnectLiveChatHandle();
	}
private:
	CamShareMiddleware *mContainer;
};

/**
 * 连接Freeswitch线程
 */
class ConnectFreeswitchRunnable : public KRunnable {
public:
	ConnectFreeswitchRunnable(CamShareMiddleware *container) {
		mContainer = container;
	}
	virtual ~ConnectFreeswitchRunnable() {
		mContainer = NULL;
	}
protected:
	void onRun() {
		mContainer->ConnectFreeswitchHandle();
	}
private:
	CamShareMiddleware *mContainer;
};

/**
 * 定时验证会议室用户权限线程
 */
class CheckConferenceRunnable : public KRunnable {
public:
	CheckConferenceRunnable(CamShareMiddleware *container) {
		mContainer = container;
	}
	virtual ~CheckConferenceRunnable() {
		mContainer = NULL;
	}
protected:
	void onRun() {
		mContainer->CheckConferenceHandle();
	}
private:
	CamShareMiddleware *mContainer;
};

/**
 * 同步本地录制完成记录线程
 */
class UploadRecordsRunnable : public KRunnable {
public:
	UploadRecordsRunnable(CamShareMiddleware *container) {
		mContainer = container;
	}
	virtual ~UploadRecordsRunnable() {
		mContainer = NULL;
	}
protected:
	void onRun() {
		mContainer->UploadRecordsHandle();
	}
private:
	CamShareMiddleware *mContainer;
};
/***************************** 线程处理 end **************************************/

CamShareMiddleware::CamShareMiddleware() {
	// TODO Auto-generated constructor stub
	mAsyncIOServer.SetAsyncIOServerCallback(this);

	// 处理线程
	mpStateRunnable = new StateRunnable(this);
	mpConnectLiveChatRunnable = new ConnectLiveChatRunnable(this);
	mpConnectFreeswitchRunnable = new ConnectFreeswitchRunnable(this);
	mpCheckConferenceRunnable = new CheckConferenceRunnable(this);
	mpUploadRecordsRunnable = new UploadRecordsRunnable(this);

	// 基本参数
	miPort = 0;
	miMaxClient = 0;
	miMaxHandleThread = 0;
	miMaxQueryPerThread = 0;
	miTimeout = 0;
	miFlashTimeout = 0;

	// 日志参数
	miStateTime = 0;
	miDebugMode = 0;
	miLogLevel = 0;

	// Livechat服务器参数
	miLivechatPort = 0;
	mLivechatIp = "";
	mLivechatName = "";

	// Freeswitch服务器参数
	miFreeswitchPort = 0;
	mFreeswitchIp = "";
	mFreeswitchUser = "";
	mFreeswitchPassword = "";
	mFreeswitchRecordingPath = "";
	mbFreeswitchIsRecording = false;
	mAuthorizationTime = 0;

	// 站点参数
	miSiteCount = 0;
	miUploadTime = 0;
	mpSiteConfig = NULL;

	// 统计参数
	mResponed = 0;
	mTotal = 0;

	mMakeCallTotal = 0;

	// 其他
	mRunning = false;

	mFreeswitch.SetFreeswitchClientListener(this);
}

CamShareMiddleware::~CamShareMiddleware() {
	// TODO Auto-generated destructor stub
	Stop();

	if( mpConnectLiveChatRunnable ) {
		delete mpConnectLiveChatRunnable;
		mpConnectLiveChatRunnable = NULL;
	}

	if( mpConnectFreeswitchRunnable ) {
		delete mpConnectFreeswitchRunnable;
		mpConnectFreeswitchRunnable = NULL;
	}

	if( mpCheckConferenceRunnable ) {
		delete mpCheckConferenceRunnable;
		mpCheckConferenceRunnable = NULL;
	}

	if( mpUploadRecordsRunnable ) {
		delete mpUploadRecordsRunnable;
		mpUploadRecordsRunnable = NULL;
	}

	if( mpStateRunnable ) {
		delete mpStateRunnable;
		mpStateRunnable = NULL;
	}
}

bool CamShareMiddleware::Start(const string& config) {
	if( config.length() > 0 ) {
		mConfigFile = config;

		// LoadConfig config
		if( LoadConfig() ) {
			return Start();

		} else {
			printf("# CamShareMiddleware can not load config file exit. \n");
		}

	} else {
		printf("# No config file can be use exit. \n");
	}

	return false;
}

bool CamShareMiddleware::Start() {
	bool bFlag = false;

	LogManager::GetLogManager()->Start(miLogLevel, mLogDir);
	LogManager::GetLogManager()->SetDebugMode(miDebugMode);
	LogManager::GetLogManager()->LogSetFlushBuffer(1 * BUFFER_SIZE_1K * BUFFER_SIZE_1K);

	LogManager::GetLogManager()->Log(
			LOG_ERR_SYS,
			"CamShareMiddleware::Start( "
			"############## CamShare Middleware ############## "
			")"
			);

	LogManager::GetLogManager()->Log(
			LOG_ERR_SYS,
			"CamShareMiddleware::Start( "
			"Version : %s, "
			"Build date : %s %s "
			")",
			VERSION_STRING,
			__DATE__,
			__TIME__
			);

	// 基本参数
	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"CamShareMiddleware::Start( "
			"miPort : %d, "
			"miMaxClient : %d, "
			"miMaxHandleThread : %d, "
			"miMaxQueryPerThread : %d, "
			"miTimeout : %d, "
			"miFlashTimeout : %d, "
			"miStateTime, %d "
			")",
			miPort,
			miMaxClient,
			miMaxHandleThread,
			miMaxQueryPerThread,
			miTimeout,
			miFlashTimeout,
			miStateTime
			);

	// 日志参数
	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"CamShareMiddleware::Start( "
			"miDebugMode : %d, "
			"miLogLevel : %d, "
			"mlogDir : %s "
			")",
			miDebugMode,
			miLogLevel,
			mLogDir.c_str()
			);

	// Livechat参数
	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"CamShareMiddleware::Start( "
			"miLivechatPort : %d, "
			"mLivechatIp : %s, "
			"mLivechatName : %s "
			")",
			miLivechatPort,
			mLivechatIp.c_str(),
			mLivechatName.c_str()
			);

	// Freeswitch参数
	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"CamShareMiddleware::Start( "
			"miFreeswitchPort : %d, "
			"mFreeswitchIp : %s, "
			"mFreeswitchUser : %s, "
			"mFreeswitchPassword : %s, "
			"mbFreeswitchIsRecording : %d, "
			"mFreeswitchRecordingPath : %s, "
			"mAuthorizationTime : %d "
			")",
			miFreeswitchPort,
			mFreeswitchIp.c_str(),
			mFreeswitchUser.c_str(),
			mFreeswitchPassword.c_str(),
			mbFreeswitchIsRecording,
			mFreeswitchRecordingPath.c_str(),
			mAuthorizationTime
			);

	// 站点参数
	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"CamShareMiddleware::Start( "
			"miSiteCount : %d, "
			"miUploadTime : %d "
			")",
			miSiteCount,
			miUploadTime
			);

	mServerMutex.lock();
	if( mRunning ) {
		Stop();
	}
	mRunning = true;

	// 创建HTTP client
	HttpClient::Init();

	// 创建请求缓存数据持久化实例
	bFlag = mDBHandler.Init();
	if( bFlag ) {
		LogManager::GetLogManager()->Log(LOG_WARNING, "CamShareMiddleware::Start( [创建sqlite数据库-成功] )");
	} else {
		LogManager::GetLogManager()->Log(LOG_ERR_SYS, "CamShareMiddleware::Start( [创建sqlite数据库-失败] )");
	}

	if( bFlag ) {
		/**
		 * 创建会话管理器
		 * 设置超时
		 */
		mSessionManager.SetRequestTimeout(miFlashTimeout);
	}

	// 创建HTTP server
	if( bFlag ) {
		bFlag = mAsyncIOServer.Start(miPort, miMaxClient, miMaxHandleThread);
		if( bFlag ) {
			LogManager::GetLogManager()->Log(LOG_WARNING, "CamShareMiddleware::Start( [创建内部服务(HTTP)-成功] )");

		} else {
			LogManager::GetLogManager()->Log(LOG_ERR_SYS, "CamShareMiddleware::Start( [创建内部服务(HTTP)-失败] )");
		}
	}

	// 初始化LiveChat server
	if( bFlag ) {
		for(int i = 0; i < miSiteCount; i++) {
			SiteConfig* pConfig = &(mpSiteConfig[i]);

			// 插入LiveChat Client
			mLiveChatClientMap.Lock();
			list<string> ips;
			ips.push_back(mLivechatIp);
			ILiveChatClient* livechat = ILiveChatClient::CreateClient();
			livechat->Init(ips, miLivechatPort, this);
			mLiveChatClientMap.Insert(pConfig->siteId, livechat);
			mLiveChatClientMap.Unlock();

			LogManager::GetLogManager()->Log(
					LOG_WARNING,
					"CamShareMiddleware::Start( "
					"event : [插入外部服务(LiveChat)], "
					"livechat : %p, "
					"siteId : %s "
					")",
					livechat,
					pConfig->siteId.c_str()
					);

			// 插入配置
			mSiteConfigMap.Lock();
			mSiteConfigMap.Insert(pConfig->siteId, pConfig);
			mSiteConfigMap.Unlock();

			// 插入在线列表
			UserMap userMap;
			mSiteUserMap.Lock();
			mSiteUserMap.Insert(pConfig->siteId, userMap);
			mSiteUserMap.Unlock();

			LogManager::GetLogManager()->Log(
					LOG_WARNING,
					"CamShareMiddleware::Start( "
					"event : [插入外部服务(PHP)配置], "
					"siteId : %s, "
					"recordFinishUrl : %s "
					")",
					pConfig->siteId.c_str(),
					pConfig->recordFinishUrl.c_str()
					);
		}
	}

	// 初始化Freeswitch
	if( bFlag ) {
		mFreeswitch.SetRecording(mbFreeswitchIsRecording, mFreeswitchRecordingPath);
	}

	// 开始状态监视线程
	if( bFlag ) {
		if( mStateThread.start(mpStateRunnable) != 0 ) {
			LogManager::GetLogManager()->Log(
					LOG_WARNING,
					"CamShareMiddleware::Start( "
					"event : [开始状态监视线程] "
					")");
		} else {
			bFlag = false;
			LogManager::GetLogManager()->Log(
					LOG_ERR_SYS,
					"CamShareMiddleware::Start( "
					"event : [开始状态监视线程-失败] "
					")"
					);
		}
	}

	// 开始LiveChat连接线程
	if( bFlag ) {
		if( mLiveChatConnectThread.start(mpConnectLiveChatRunnable) != 0 ) {
			LogManager::GetLogManager()->Log(
					LOG_WARNING,
					"CamShareMiddleware::Start( "
					"event : [开始LiveChat连接线程] "
					")"
					);
		} else {
			bFlag = false;
			LogManager::GetLogManager()->Log(
					LOG_ERR_SYS,
					"CamShareMiddleware::Start( "
					"event : [开始LiveChat连接线程-失败] "
					")"
					);
		}
	}

	// 开始Freeswitch连接线程
	if( bFlag ) {
		if( mConnectFreeswitchThread.start(mpConnectFreeswitchRunnable) != 0 ) {
			LogManager::GetLogManager()->Log(
					LOG_WARNING,
					"CamShareMiddleware::Start( "
					"event : [开始Freeswitch连接线程] "
					")"
					);
		} else {
			bFlag = false;
			LogManager::GetLogManager()->Log(
					LOG_ERR_SYS,
					"CamShareMiddleware::Start( "
					"event : [开始Freeswitch连接线程-失败] "
					")");
		}
	}

	// 开始定时验证会议室用户权限线程
	if( bFlag ) {
		if( mCheckConferenceThread.start(mpCheckConferenceRunnable) != 0 ) {
			LogManager::GetLogManager()->Log(
					LOG_WARNING,
					"CamShareMiddleware::Start( "
					"event : [开始定时验证会议室用户权限线程] "
					")"
					);
		} else {
			bFlag = false;
			LogManager::GetLogManager()->Log(
					LOG_ERR_SYS,
					"CamShareMiddleware::Start( "
					"event : [开始定时验证会议室用户权限线程-失败] "
					")"
					);
		}
	}

	// 开始同步本地录制完成记录线程
	if( bFlag ) {
		if( mUploadRecordsThread.start(mpUploadRecordsRunnable) != 0 ) {
			LogManager::GetLogManager()->Log(LOG_WARNING, "CamShareMiddleware::Start( event : [开始同步本地录制完成记录线程] )");
		} else {
			bFlag = false;
			LogManager::GetLogManager()->Log(LOG_ERR_SYS, "CamShareMiddleware::Start( event : [开始同步本地录制完成记录线程-失败] )");
		}
	}

	if( bFlag ) {
		// 服务启动成功
		LogManager::GetLogManager()->Log(
				LOG_WARNING,
				"CamShareMiddleware::Start( "
				"event : [OK] "
				")"
				);
		printf("# CamShareMiddleware start OK. \n");

	} else {
		// 服务启动失败
		LogManager::GetLogManager()->Log(
				LOG_ERR_SYS,
				"CamShareMiddleware::Start( "
				"event : [Fail] "
				")"
				);
		Stop();
	}
	mServerMutex.unlock();

	return bFlag;
}

bool CamShareMiddleware::LoadConfig() {
	bool bFlag = false;
	mConfigMutex.lock();
	if( mConfigFile.length() > 0 ) {
		ConfFile conf;
		conf.InitConfFile(mConfigFile.c_str(), "");
		if ( conf.LoadConfFile() ) {
			// 基本参数
			miPort = atoi(conf.GetPrivate("BASE", "PORT", "9876").c_str());
			miMaxClient = atoi(conf.GetPrivate("BASE", "MAXCLIENT", "100000").c_str());
			miMaxHandleThread = atoi(conf.GetPrivate("BASE", "MAXHANDLETHREAD", "2").c_str());
			miMaxQueryPerThread = atoi(conf.GetPrivate("BASE", "MAXQUERYPERCOPY", "10").c_str());
			miTimeout = atoi(conf.GetPrivate("BASE", "TIMEOUT", "10").c_str());
			miFlashTimeout = atoi(conf.GetPrivate("BASE", "FLASH_TIMEOUT", "5").c_str());
			miStateTime = atoi(conf.GetPrivate("BASE", "STATETIME", "30").c_str());

//			mClientTcpServer.SetHandleSize(miTimeout * miMaxQueryPerThread);

			// 日志参数
			miLogLevel = atoi(conf.GetPrivate("LOG", "LOGLEVEL", "5").c_str());
			mLogDir = conf.GetPrivate("LOG", "LOGDIR", "log");
			miDebugMode = atoi(conf.GetPrivate("LOG", "DEBUGMODE", "0").c_str());

			// Livechat参数
			miLivechatPort = atoi(conf.GetPrivate("LIVECHAT", "PORT", "9877").c_str());
			mLivechatIp = conf.GetPrivate("LIVECHAT", "IP", "127.0.0.1");
			mLivechatName = conf.GetPrivate("LIVECHAT", "NAME", "CAM");

			// Freeswitch参数
			miFreeswitchPort = atoi(conf.GetPrivate("FREESWITCH", "PORT", "8021").c_str());
			mFreeswitchIp = conf.GetPrivate("FREESWITCH", "IP", "127.0.0.1");
			mFreeswitchUser = conf.GetPrivate("FREESWITCH", "USER", "");
			mFreeswitchPassword = conf.GetPrivate("FREESWITCH", "PASSWORD", "ClueCon");
			mbFreeswitchIsRecording = atoi(conf.GetPrivate("FREESWITCH", "ISRECORDING", "0").c_str());
			mFreeswitchRecordingPath = conf.GetPrivate("FREESWITCH", "RECORDINGPATH", "/usr/local/freeswitch/recordings");
			mAuthorizationTime = atoi(conf.GetPrivate("FREESWITCH", "AUTHORIZATION_TIME", "60").c_str());

			// 站点参数
			miSiteCount = atoi(conf.GetPrivate("SITE", "SITE_COUNT", "0").c_str());
			miUploadTime = atoi(conf.GetPrivate("SITE", "UPLOADTIME", "30").c_str());

			if( miSiteCount > 0 ) {
				// 新建数组
				if( mpSiteConfig ) {
					delete[] mpSiteConfig;
				}
				mpSiteConfig = new SiteConfig[miSiteCount];

				char domain[16] = {'\0'};
				for(int i = 0; i < miSiteCount; i++) {
					SiteConfig* pConfig = &(mpSiteConfig[i]);
					sprintf(domain, "SITE_%d", i);
					pConfig->siteId = conf.GetPrivate(domain, "SITE_ID", "").c_str();
					pConfig->recordFinishUrl = conf.GetPrivate(domain, "SITE_HTTP_RECORD_URL", "").c_str();
					pConfig->loginCheckUrl = conf.GetPrivate(domain, "SITE_HTTP_LOGIN_URL", "").c_str();
					pConfig->loginCheck = atoi(conf.GetPrivate(domain, "SITE_EXTERNL_LOGIN", "0").c_str());
				}
			}

			bFlag = true;
		}
	}
	mConfigMutex.unlock();
	return bFlag;
}

bool CamShareMiddleware::ReloadLogConfig() {
	bool bFlag = false;
	mConfigMutex.lock();
	if( mConfigFile.length() > 0 ) {
		ConfFile conf;
		conf.InitConfFile(mConfigFile.c_str(), "");
		if ( conf.LoadConfFile() ) {
			// 基本参数
			miStateTime = atoi(conf.GetPrivate("BASE", "STATETIME", "30").c_str());

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

bool CamShareMiddleware::IsRunning() {
	return mRunning;
}

bool CamShareMiddleware::Stop() {
	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"CamShareMiddleware::Stop( "
			")"
			);

	mServerMutex.lock();

	if( mRunning ) {
		mRunning = false;

		// 停止监听socket
		mAsyncIOServer.Stop();

		mLiveChatConnectThread.stop();
		mConnectFreeswitchThread.stop();
		mCheckConferenceThread.stop();
		mUploadRecordsThread.stop();
		mStateThread.stop();

		if( mpSiteConfig ) {
			delete[] mpSiteConfig;
			mpSiteConfig = NULL;
		}
	}

	mServerMutex.unlock();

	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"CamShareExecutor::Stop( "
			"event : [OK] "
			")"
			);

	LogManager::GetLogManager()->Stop();

	return true;
}

/***************************** 线程处理函数 **************************************/
void CamShareMiddleware::StateHandle() {
	unsigned int iCount = 1;
	unsigned int iStateTime = miStateTime;

	unsigned int iTotal = 0;
	double iSecondTotal = 0;
//	double iResponed = 0;

	unsigned int iMakeCallTotal = 0;

	while( IsRunning() ) {
		if ( iCount < iStateTime ) {
			iCount++;

		} else {
			iCount = 1;
			iSecondTotal = 0;
//			iResponed = 0;

			mCountMutex.lock();
			iTotal = mTotal;

			if( iStateTime != 0 ) {
				iSecondTotal = 1.0 * iTotal / iStateTime;
			}
//			if( iTotal != 0 ) {
//				iResponed = 1.0 * mResponed / iTotal;
//			}

			mTotal = 0;
			mResponed = 0;
			mCountMutex.unlock();

			mMakeCallCountMutex.lock();
			iMakeCallTotal = mMakeCallTotal;
			mMakeCallTotal = 0;
			mMakeCallCountMutex.unlock();

			unsigned int recordCount = 0;
			mDBHandler.GetRecordsCount(recordCount);
			unsigned int recordErrorCount = 0;
			mDBHandler.GetRecordsErrorCount(recordErrorCount);

			LogManager::GetLogManager()->Log(
					LOG_ERR_USER,
					"CamShareMiddleware::StateHandle( "
					"event : [内部服务(HTTP)], "
					"过去%u秒共收到请求 : %u, "
					"平均收到请求 : %.1lf/秒 "
//					"平均响应时间%.1lf毫秒/个,"
					")",
					iStateTime,
					iTotal,
					iSecondTotal
//					iResponed,
					);

			LogManager::GetLogManager()->Log(
					LOG_ERR_USER,
					"CamShareMiddleware::StateHandle( "
					"event : [内部服务(Freeswitch)], "
					"当前通话 : %u, "
					"在线用户 : %u, "
					"在线RTMP : %u, "
					"在线Websocket : %u, "
					"过去%u秒共收到MakeCall请求 : %u "
					")",
					mFreeswitch.GetChannelCount(),
					mFreeswitch.GetOnlineUserCount(),
					mFreeswitch.GetRTMPSessionCount(),
					mFreeswitch.GetWebSocketUserCount(),
					iStateTime,
					iMakeCallTotal
					);

			LogManager::GetLogManager()->Log(
					LOG_ERR_USER,
					"CamShareMiddleware::StateHandle( "
					"event : [外部服务(PHP)], "
					"未发送录制完成记录 : %u, "
					"未处理录制完成错误记录 : %u "
					")",
					recordCount,
					recordErrorCount
					);

			iStateTime = miStateTime;

		}
		sleep(1);
	}
}

void CamShareMiddleware::ConnectLiveChatHandle() {
	bool bFlag = false;
	while( IsRunning() ) {
		mLiveChatClientMap.Lock();
		for(LiveChatClientMap::iterator itr = mLiveChatClientMap.Begin(); itr != mLiveChatClientMap.End(); itr++) {
			ILiveChatClient* livechat = itr->second;
			bFlag = livechat->ConnectServer(itr->first, mLivechatName);
			if( bFlag ) {
				LogManager::GetLogManager()->Log(
						LOG_ERR_USER,
						"CamShareMiddleware::ConnectLiveChatHandle( "
						"event : [外部服务(LiveChat)-开始连接], "
						"siteId : %s, "
						"name : %s, "
						"livechat : %p "
						")",
						itr->first.c_str(),
						mLivechatName.c_str(),
						livechat
						);
			}
		}
		mLiveChatClientMap.Unlock();

		sleep(30);
	}
}

void CamShareMiddleware::ConnectFreeswitchHandle() {
//	bool bFlag = false;
	unsigned int i = 30;
	while( IsRunning() ) {
//		// 先让LiveChat连接服务器
//		mLiveChatClientMap.Lock();
//		for(LiveChatClientMap::iterator itr = mLiveChatClientMap.Begin(); itr != mLiveChatClientMap.End(); itr++) {
//			ILiveChatClient* client = itr->second;
//			bFlag = client->IsConnected();
//
//			if( !bFlag ) {
//				// 其中一个站点未连上
//				break;
//			}
//		}
//		mLiveChatClientMap.Unlock();

		if( i >= 30 ) {
			// 开始连接Freeswitch
			if( !mFreeswitch.Proceed(
					mFreeswitchIp,
					miFreeswitchPort,
					mFreeswitchUser,
					mFreeswitchPassword
					) ) {
				LogManager::GetLogManager()->Log(
						LOG_ERR_USER,
						"CamShareMiddleware::ConnectFreeswitchHandle( "
						"event : [内部服务(Freeswitch), 连接Freeswitch失败] "
						")"
						);
			} else {
				LogManager::GetLogManager()->Log(
						LOG_ERR_USER,
						"CamShareMiddleware::ConnectFreeswitchHandle( "
						"event : [内部服务(Freeswitch), 连接断开] "
						")"
						);
			}

			// Freeswitch连接断开
			i = 1;
			sleep(10);

		} else {
			// 继续等待
			i++;
			sleep(1);
		}

	}
}

void CamShareMiddleware::CheckConferenceHandle() {
	int count = 0;
	while( IsRunning() ) {
		if( mAuthorizationTime > 0 ) {
			if( count >= mAuthorizationTime ) {
				LogManager::GetLogManager()->Log(
						LOG_WARNING,
						"CamShareMiddleware::CheckConferenceHandle( "
						"event : [内部服务(Freeswitch), 重新验证所有会议室用户] "
						")"
						);

				// 内部服务(Freeswitch), 重新验证所有会议室用户
				mFreeswitch.AuthorizationAllConference();

				// 复位
				count = 1;
			}

			count++;
		}
		sleep(1);
	}
}

void CamShareMiddleware::UploadRecordsHandle() {
	static int selectCount = 60;
	Record records[selectCount];
	int getSize = 0;
	HttpClient client;
	bool success = false;
	bool errorRecord = false;
	string errorCode = "";
	bool bFlag = false;

	while( IsRunning() ) {
		// 从本地数据库获取记录
		getSize = 0;
		mDBHandler.GetRecords(records, selectCount, getSize);
		if( getSize > 0 ) {
			for(int i = 0; i < getSize; i++) {
				// 发送记录到服务器
				bFlag = SendRecordFinish(&client, records[i], success, errorRecord, errorCode);

				// 删除本地记录
				mDBHandler.RemoveRecord(records[i]);

				if( bFlag ) {
					// 发送成功, 并解析返回成功
					if( !success ) {
						// 服务器返回, 上传失败错误, 插入到错误库
						mDBHandler.InsertErrorRecord(records[i], errorCode);
					}

				} else {
					// 发送失败, 或者解析失败
					if( !errorRecord ) {
						// 不是本地参数错误, 有效记录, 插回本地库
						mDBHandler.InsertRecord(records[i]);

						sleep(miUploadTime);
					} else {
						// 本地错误, 插入到错误库
						mDBHandler.InsertErrorRecord(records[i], errorCode);
					}
				}
			}
		} else {
			sleep(miUploadTime);
		}

	}
}
/***************************** 线程处理函数 end **************************************/

/***************************** 内部服务(HTTP)回调 **************************************/
bool CamShareMiddleware::OnAccept(Client *client) {
	HttpParser* parser = new HttpParser();
	parser->SetCallback(this);
	parser->custom = client;
	client->parser = parser;

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"CamShareMiddleware::OnAccept( "
			"parser : %p, "
			"client : %p "
			")",
			parser,
			client
			);

	return true;
}

void CamShareMiddleware::OnDisconnect(Client* client) {
	HttpParser* parser = (HttpParser *)client->parser;

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"CamShareMiddleware::OnDisconnect( "
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

void CamShareMiddleware::OnHttpParserHeader(HttpParser* parser) {
	Client* client = (Client *)parser->custom;

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"CamShareMiddleware::OnHttpParserHeader( "
			"parser : %p, "
			"client : %p, "
			"path : %s "
			")",
			parser,
			client,
			parser->GetPath().c_str()
			);

	bool bFlag = false;

	bFlag = HttpParseRequestHeader(parser);
	// 已经处理则可以断开连接
	if( bFlag ) {
		mAsyncIOServer.Disconnect(client);
	}
}

void CamShareMiddleware::OnHttpParserBody(HttpParser* parser) {
	Client* client = (Client *)parser->custom;

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"CamShareMiddleware::OnHttpParserBody( "
			"parser : %p, "
			"client : %p "
			")",
			parser,
			client
			);

	bool bFlag = HttpParseRequestBody(parser);
	if ( bFlag ) {
		mAsyncIOServer.Disconnect(client);
	}
}

void CamShareMiddleware::OnHttpParserError(HttpParser* parser) {
	Client* client = (Client *)parser->custom;

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"CamShareMiddleware::OnHttpParserError( "
			"parser : %p, "
			"client : %p "
			")",
			parser,
			client
			);

	mAsyncIOServer.Disconnect(client);
}
/***************************** 内部服务(HTTP)回调 End **************************************/

/***************************** 内部服务(HTTP)接口 **************************************/
bool CamShareMiddleware::HttpParseRequestHeader(HttpParser* parser) {
	bool bFlag = true;

	if( parser->GetPath() == "/GETDIALPLAN" ) {
		// 进入会议室
		OnRequestGetDialplan(parser);

	} else if( parser->GetPath() == "/RECORDFINISH" ) {
		// 录制文件成功
		OnRequestRecordFinish(parser);

	} else if( parser->GetPath() == "/RELOADLOGCONFIG" ) {
		// 重新加载日志配置
		OnRequestReloadLogConfig(parser);

	} else if( parser->GetPath() == "/RESENDERRORRECORD" ) {
		// 重新发送错误记录
		OnRequestReSendErrorRecord(parser);

	} else if( parser->GetPath() == "/REMOVEERRORRECORD" ) {
		// 删除错误记录
		OnRequestRemoveErrorRecord(parser);

	} else {
		// 未知命令
//		OnRequestUndefinedCommand(parser);
		bFlag = false;
	}

	return bFlag;
}

bool CamShareMiddleware::HttpParseRequestBody(HttpParser* parser) {
	bool bFlag = true;

	if( parser->GetPath() == "/set_status" ) {
		// 接收上下线通知
		OnRequestSetStatus(parser);
	} else if( parser->GetPath() == "/sync_status" ) {
		// 同步在线状态
		OnRequestSyncStatus(parser);
	} else {
		// 未知命令
		OnRequestUndefinedCommand(parser);
	}

	return bFlag;
}

bool CamShareMiddleware::HttpSendRespond(
		HttpParser* parser,
		IRespond* respond
		) {
	bool bFlag = false;
	Client* client = (Client *)parser->custom;

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"CamShareMiddleware::HttpSendRespond( "
			"event : [内部服务(HTTP)-返回请求到客户端], "
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

	mAsyncIOServer.Send(client, buffer, len);

	if( respond != NULL ) {
		// 发送内容
		bool more = false;
		while( true ) {
			len = respond->GetData(buffer, MAX_BUFFER_LEN, more);
			LogManager::GetLogManager()->Log(
					LOG_WARNING,
					"CamShareMiddleware::HttpSendRespond( "
					"event : [内部服务(HTTP)-返回请求内容到客户端], "
					"parser : %p, "
					"client : %p, "
					"respond : %p, "
					"buffer : %s "
					")",
					parser,
					client,
					respond,
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
				"CamShareMiddleware::HttpSendRespond( "
				"event : [内部服务(HTTP)-返回请求到客户端-失败], "
				"parser : %p, "
				"client : %p "
				")",
				parser,
				client
				);
	}

	return bFlag;
}
/***************************** 内部服务(HTTP)接口 end **************************************/

/***************************** 内部服务(HTTP) 回调处理 **************************************/
void CamShareMiddleware::OnRequestGetDialplan(
		HttpParser* parser
		) {
	const string caller = parser->GetParam("caller");
	const string channelId = parser->GetParam("channelId");
	const string conference = parser->GetParam("conference");
	const string serverId = parser->GetParam("serverId");
	const string siteId = parser->GetParam("siteId");
	const string source = parser->GetParam("source");
	const string chat_type_string = parser->GetParam("chat_type_string");

	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"CamShareMiddleware::OnRequestGetDialplan( "
			"event : [内部服务(HTTP)-收到命令:获取拨号计划], "
			"caller : %s, "
			"conference : %s, "
			"serverId : %s, "
			"siteId : %s, "
			"source : %s, "
			"chat_type_string : %s, "
			"channelId : %s, "
			"parser : %p "
			")",
			caller.c_str(),
			conference.c_str(),
			serverId.c_str(),
			siteId.c_str(),
			source.c_str(),
			chat_type_string.c_str(),
			channelId.c_str(),
			parser
			);

	// 统计MakeCall请求
	mMakeCallCountMutex.lock();
	mMakeCallTotal++;
	mMakeCallCountMutex.unlock();

	// 插入channel
    bool bFlag = mFreeswitch.UpdateChannelWithDialplan(channelId, caller, conference, serverId, siteId, chat_type_string);
    if( !bFlag ) {
    	LogManager::GetLogManager()->Log(
    			LOG_ERR_USER,
    			"CamShareMiddleware::OnClientGetDialplan( "
    			"event : [内部服务(HTTP)-收到命令:获取拨号计划-失败], "
    			"caller : %s, "
    			"channelId : %s, "
    			"conference : %s, "
    			"serverId : %s, "
    			"siteId : %s, "
    			"source : %s "
    			")",
    			caller.c_str(),
    			channelId.c_str(),
    			conference.c_str(),
    			serverId.c_str(),
    			siteId.c_str(),
    			source.c_str()
    			);
    }

	// 马上返回数据
    BaseResultRespond respond;
	respond.SetParam(bFlag);
	HttpSendRespond(parser, &respond);
}

void CamShareMiddleware::OnRequestRecordFinish(
		HttpParser* parser
		) {
	const string conference = parser->GetParam("userId");
	const string siteId = parser->GetParam("siteId");
	const string filePath = parser->GetParam("fileName");
	const string startTime = parser->GetParam("startTime");
	const string endTime = parser->GetParam("endTime");

	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"CamShareMiddleware::OnRequestRecordFinish( "
			"event : [内部服务(HTTP)-收到命令:录制文件成功], "
			"parser : %p, "
			"conference : %s, "
			"siteId : %s, "
			"filePath : %s, "
			"startTime : %s, "
			"endTime : %s "
			")",
			parser,
			conference.c_str(),
			siteId.c_str(),
			filePath.c_str(),
			startTime.c_str(),
			endTime.c_str()
			);
	// 插入数据库记录
	Record record(conference, siteId, filePath, startTime, endTime);
	mDBHandler.InsertRecord(record);

	// 马上返回数据
	BaseResultRespond respond;
	respond.SetParam(true);
	HttpSendRespond(parser, &respond);
}

void CamShareMiddleware::OnRequestReloadLogConfig(HttpParser* parser) {
	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"CamShareMiddleware::OnRequestReloadLogConfig( "
			"event : [内部服务(HTTP)-收到命令:重新加载日志配置], "
			"parser : %p "
			")",
			parser
			);
	// 重新加载日志配置
	ReloadLogConfig();

	// 马上返回数据
	BaseResultRespond respond;
	respond.SetParam(true);
	HttpSendRespond(parser, &respond);
}

void CamShareMiddleware::OnRequestReSendErrorRecord(HttpParser* parser) {
	const string recordId = parser->GetParam("recordId");

	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"CamShareMiddleware::OnRequestReSendErrorRecord( "
			"event : [内部服务(HTTP)-收到命令:重新发送错误记录], "
			"parser : %p, "
			"recordId : %s "
			")",
			parser,
			recordId.c_str()
			);

	HttpClient client;
	bool success = false;
	bool errorRecord = false;
	string errorCode = "";

	const char* respondData = NULL;
	int respondSize = 0;

	bool bUploadSuccess = false;
	if( recordId.length() > 0 ) {
		Record record;
		if( mDBHandler.GetErrorRecord(&record, recordId) ) {
			// 发送记录到服务器
			bool bFlag = SendRecordFinish(&client, record, success, errorRecord, errorCode);
			if( bFlag && success ) {
				// 服务器返回
				if( success ) {
					// 上传成功
					bUploadSuccess = true;

					// 删除错误记录
					mDBHandler.RemoveErrorRecord(recordId);
				}
			}
		} else {
			LogManager::GetLogManager()->Log(
					LOG_ERR_USER,
					"CamShareMiddleware::OnRequestReSendErrorRecord( "
					"event : [内部服务(HTTP)-收到命令:重新发送错误记录, 找不到对应记录], "
					"parser : %p, "
					"recordId : %s "
					")",
					parser,
					recordId.c_str()
					);
		}
	}

	// 马上返回数据
	ReSendErrorRecordRespond respond;

	// 成功失败
	respond.SetParam(bUploadSuccess);

	// 接口返回内容
	client.GetBody(&respondData, respondSize);
	respond.SetDataZeroMemory(respondData);

	HttpSendRespond(parser, &respond);
}

void CamShareMiddleware::OnRequestRemoveErrorRecord(HttpParser* parser) {
	const string recordId = parser->GetParam("recordId");

	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"CamShareMiddleware::OnRequestRemoveErrorRecord( "
			"event : [内部服务(HTTP)-收到命令:删除错误记录], "
			"parser : %p, "
			"recordId : %s "
			")",
			parser,
			recordId.c_str()
			);

	bool bFlag = false;
	if( recordId.length() > 0 ) {
		bFlag = mDBHandler.RemoveErrorRecord(recordId);
	}

	if( !bFlag ) {
		LogManager::GetLogManager()->Log(
				LOG_ERR_USER,
				"CamShareMiddleware::OnRequestRemoveErrorRecord( "
				"event : [内部服务(HTTP)-收到命令:删除错误记录-失败], "
				"parser : %p, "
				"recordId : %s "
				")",
				parser,
				recordId.c_str()
				);
	}

	// 马上返回数据
	BaseResultRespond respond;
	// 成功失败
	respond.SetParam(bFlag);
	HttpSendRespond(parser, &respond);
}

void CamShareMiddleware::OnRequestSetStatus(HttpParser* parser) {
	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"CamShareMiddleware::OnRequestSetStatus( "
			"event : [内部服务(HTTP)-收到命令:设置在线状态], "
			"parser : %p, "
			"body : %s "
			")",
			parser,
			parser->GetBody()
			);

	bool bFlag = false;
	bool bChange = false;
	Json::Value reqRoot;
	Json::Reader reader;
	bool bParse = reader.parse(parser->GetBody(), reqRoot, false);
	if ( bParse ) {
		if( reqRoot.isObject() ) {
			if ( reqRoot["param"].isString() && reqRoot["status"].isInt() ) {
				string param = reqRoot["param"].asString();
				bool status = reqRoot["status"].asInt();
				string userId = "";
				string siteId = "";
				GetExtParameters(param, userId, siteId);

				if ( userId.length() > 0 && siteId.length() > 0 ) {
					bool bResult = true;
					if( status ) {
						// 外部登录校验
						bResult = SendExternalLoginCheck(siteId, param);
					}

					if ( bResult ) {
						ILiveChatClient* livechat = NULL;
						mLiveChatClientMap.Lock();
						LiveChatClientMap::iterator lcItr = mLiveChatClientMap.Find(siteId);
						if( lcItr != mLiveChatClientMap.End() ) {
							livechat = lcItr->second;
						}
						mLiveChatClientMap.Unlock();

						// 更新在线状态
						mSiteUserMap.Lock();
						SiteUserMap::iterator itr = mSiteUserMap.Find(siteId);
						if( itr != mSiteUserMap.End() ) {
							UserMap *pUserMap = &itr->second;
							pUserMap->Lock();
							UserMap::iterator userItr = pUserMap->Find(userId);
							if( status ) {
								if( userItr == pUserMap->End() ) {
									// 用户上线
									SiteConnection *con = new SiteConnection();
									con->wsCount = 1;
									pUserMap->Insert(userId, con);
									bFlag = true;
									bChange = true;

									LogManager::GetLogManager()->Log(
											LOG_WARNING,
											"CamShareMiddleware::OnRequestSetStatus( "
											"event : [内部服务(HTTP)-收到命令:设置在线状态, 上线], "
											"parser : %p, "
											"body : %s, "
											"count : %d "
											")",
											parser,
											parser->GetBody(),
											con->wsCount
											);
								} else {
									SiteConnection *con = userItr->second;
									con->wsCount++;
									bFlag = true;

									LogManager::GetLogManager()->Log(
											LOG_WARNING,
											"CamShareMiddleware::OnRequestSetStatus( "
											"event : [内部服务(HTTP)-收到命令:设置在线状态, 增加数量], "
											"parser : %p, "
											"body : %s, "
											"count : %d "
											")",
											parser,
											parser->GetBody(),
											con->wsCount
											);
								}
							} else if( userItr != pUserMap->End() ) {
								// 用户下线
								SiteConnection *con = userItr->second;
								con->wsCount--;
								bFlag = true;
								if ( con->IsOffline() ) {
									delete con;
									pUserMap->Erase(userItr);
									bChange = true;

									LogManager::GetLogManager()->Log(
											LOG_WARNING,
											"CamShareMiddleware::OnRequestSetStatus( "
											"event : [内部服务(HTTP)-收到命令:设置在线状态, 下线], "
											"parser : %p, "
											"body : %s "
											")",
											parser,
											parser->GetBody()
											);
								}
							}
							pUserMap->Unlock();
						}

						if ( bChange ) {
							// 发送用户在线状态改变命令
							SendOnlineStatus2LiveChat(livechat, userId, status);
						}

						mSiteUserMap.Unlock();
					}
				}
			}
		}
	}

	// 马上返回数据
	BaseResultRespond respond;
	// 成功失败
	respond.SetParam(bFlag);
	HttpSendRespond(parser, &respond);
}

void CamShareMiddleware::OnRequestSyncStatus(HttpParser* parser) {
	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"CamShareMiddleware::OnRequestSyncStatus( "
			"event : [内部服务(HTTP)-收到命令:同步在线列表], "
			"parser : %p, "
			"body : %s "
			")",
			parser,
			parser->GetBody()
			);

	bool bFlag = false;
	Json::Value reqRoot;
	Json::Reader reader;
	bool bParse = reader.parse(parser->GetBody(), reqRoot, false);
	if ( bParse ) {
		if( reqRoot.isObject() ) {
			if ( reqRoot["params"].isArray() ) {
				bFlag = true;

				// 清空分站在线用户列表
				mSiteUserMap.Lock();
				for(SiteUserMap::iterator siteItr = mSiteUserMap.Begin(); siteItr != mSiteUserMap.End(); siteItr++) {
					UserMap *pUserMap = &siteItr->second;
					pUserMap->Lock();
					for(UserMap::iterator userItr = pUserMap->Begin(); userItr != pUserMap->End();) {
						SiteConnection *con = userItr->second;
						con->wsCount = 0;

						if ( con->IsOffline() ) {
							delete con;
							pUserMap->Erase(userItr++);
						} else {
							userItr++;
						}
					}
					pUserMap->Unlock();
				}
				mSiteUserMap.Unlock();

				// 插入新的在线用户
				for (unsigned int i = 0; i < reqRoot["params"].size(); i++) {
					if ( reqRoot["params"][i].isString() ) {
						string param = reqRoot["params"][i].asString();

						bool status = true;
						string userId = "";
						string siteId = "";
						GetExtParameters(param, userId, siteId);

						if ( userId.length() > 0 && siteId.length() > 0 ) {
							mSiteUserMap.Lock();
							SiteUserMap::iterator siteItr = mSiteUserMap.Find(siteId);
							if( siteItr != mSiteUserMap.End() ) {
								UserMap *pUserMap = &siteItr->second;
								pUserMap->Lock();
								UserMap::iterator userItr = pUserMap->Find(userId);
								if( userItr == pUserMap->End() ) {
									// 用户上线
									SiteConnection *con = new SiteConnection();
									con->wsCount = 1;
									pUserMap->Insert(userId, con);
								} else {
									// 增加连接数
									SiteConnection *con = userItr->second;
									con->wsCount++;
								}
								pUserMap->Unlock();
							}
							mSiteUserMap.Unlock();
						}
					}
				}

				// 通知Livechat重置在线列表
				SendAllOnlineList();
			}
		}
	}

	// 马上返回数据
	BaseResultRespond respond;
	// 成功失败
	respond.SetParam(bFlag);
	HttpSendRespond(parser, &respond);
}

void CamShareMiddleware::OnRequestUndefinedCommand(HttpParser* parser) {
	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"CamShareMiddleware::OnRequestUndefinedCommand( "
			"event : [内部服务(HTTP)-收到命令:未知命令], "
			"parser : %p "
			")",
			parser
			);
	Client* client = (Client *)parser->custom;

	// 马上返回数据
	BaseResultRespond respond;
	respond.SetParam(false);
	HttpSendRespond(parser, &respond);
}
/***************************** 内部服务(HTTP) 回调处理 end **************************************/

/***************************** 内部服务(Freeswitch) 回调处理 **************************************/
void CamShareMiddleware::OnFreeswitchConnect(FreeswitchClient* freeswitch) {
}

void CamShareMiddleware::OnFreeswitchEventConferenceAuthorizationMember(
		FreeswitchClient* freeswitch,
		const Channel* channel
		) {
	// 踢出相同账户已经进入的其他连接
	mFreeswitch.KickUserFromConference(channel->user, channel->conference, channel->memberId);
	if( channel->user != channel->conference && !CheckTestAccount(channel->user) ) {
		// 进入别人的会议室
		string serverId = channel->serverId;
		string siteId = channel->siteId;

		// 找出需要发送的LiveChat Client
		ILiveChatClient* livechat = NULL;
		mLiveChatClientMap.Lock();
		LiveChatClientMap::iterator itr = mLiveChatClientMap.Find(siteId);
		if( itr != mLiveChatClientMap.End() ) {
			livechat = itr->second;
		}
		mLiveChatClientMap.Unlock();

		if( !CheckTestAccount(channel->user) ) {
			// 非测试账号
			// 发送进入会议室认证命令
			if( !SendEnterConference2LiveChat(livechat, channel->user, channel->conference, channel->type, channel->serverId, Timer, channel->chatType) ) {
				// 断开指定用户视频
				freeswitch->KickUserFromConference(channel->user, channel->conference, "");
			}
		} else {
			// 测试账号
			// 开放成员视频
			mFreeswitch.StartUserRecvVideo(channel->user, channel->conference, channel->type);
		}

	} else {
		// 进入自己会议室, 直接通过
		// 开放成员视频
		mFreeswitch.StartUserRecvVideo(channel->user, channel->conference, channel->type);
	}
}

void CamShareMiddleware::OnFreeswitchEventConferenceAddMember(
		FreeswitchClient* freeswitch,
		const Channel* channel
		) {
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"CamShareMiddleware::OnFreeswitchEventConferenceAddMember( "
			"event : [内部服务(Freeswitch)-收到命令:增加会议成员], "
			"user : %s, "
			"conference : %s, "
			"type : %s, "
			"memberId : %s, "
			"serverId : %s, "
			"siteId : %s, "
            "chatType : %d "
			")",
			channel->user.c_str(),
			channel->conference.c_str(),
			(channel->type == Moderator)?"Moderator":"Member",
			channel->memberId.c_str(),
			channel->serverId.c_str(),
			channel->siteId.c_str(),
            channel->chatType
			);

	// 踢出相同账户已经进入的其他连接
	mFreeswitch.KickUserFromConference(channel->user, channel->conference, channel->memberId);

	if( channel->user != channel->conference ) {
		// 进入别人会议室

		// 找出需要发送的LiveChat Client
		string siteId = channel->siteId;
		ILiveChatClient* livechat = NULL;
		mLiveChatClientMap.Lock();
		LiveChatClientMap::iterator itr = mLiveChatClientMap.Find(siteId);
		if( itr != mLiveChatClientMap.End() ) {
			livechat = itr->second;
		}
		mLiveChatClientMap.Unlock();

		if( !CheckTestAccount(channel->user) ) {
			// 非测试账号
			string serverId = channel->serverId;

			// 发送进入会议室认证命令
			if( !SendEnterConference2LiveChat(livechat, channel->user, channel->conference, channel->type, channel->serverId, Active, channel->chatType) ) {
				// 断开指定用户视频
				freeswitch->KickUserFromConference(channel->user, channel->conference, "");
			}

			// 获取用户列表
			list<string> userList;
			mFreeswitch.GetConferenceUserList(channel->conference, userList);

			// 发送通知客户端进入会议室命令
			SendMsgEnterConference2LiveChat(livechat, channel->user, channel->conference, userList);

		} else {
			// 测试账号
			// 开放成员视频
			mFreeswitch.StartUserRecvVideo(channel->user, channel->conference, channel->type);
		}

	} else {
		// 进入自己会议室, 直接通过
		// 开放成员视频
		mFreeswitch. StartUserRecvVideo(channel->user, channel->conference, channel->type);
	}

}

void CamShareMiddleware::OnFreeswitchEventConferenceDelMember(
		FreeswitchClient* freeswitch,
		const Channel* channel
		) {
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"CamShareMiddleware::OnFreeswitchEventConferenceDelMember( "
			"event : [内部服务(Freeswitch)-收到命令:退出会议成员], "
			"user : %s, "
			"conference : %s, "
			"type : %s, "
			"memberId : %s, "
			"serverId : %s, "
			"siteId : %s "
			")",
			channel->user.c_str(),
			channel->conference.c_str(),
			(channel->type == Moderator)?"Moderator":"Member",
			channel->memberId.c_str(),
			channel->serverId.c_str(),
			channel->siteId.c_str()
			);

	if( channel->user != channel->conference ) {
		// 退出别人会议室
		if( channel->user.length() > 0
				&& channel->conference.length() > 0
				&& channel->siteId.length() > 0
				) {
			// 通知客户端
			ILiveChatClient* livechat = NULL;
			mLiveChatClientMap.Lock();
			LiveChatClientMap::iterator itr = mLiveChatClientMap.Find(channel->siteId);
			if( itr != mLiveChatClientMap.End() ) {
				livechat = itr->second;
			}
			mLiveChatClientMap.Unlock();

			if( !CheckTestAccount(channel->user) ) {
				// 非测试账号

				// 获取用户列表
				list<string> userList;
				mFreeswitch.GetConferenceUserList(channel->conference, userList);

				// 发送通知客户端退出会议室命令
				SendMsgExitConference2LiveChat(livechat, channel->user, channel->conference, userList);
			}
		}

	}
}
void CamShareMiddleware::OnFreeswitchEventOnlineList(
		FreeswitchClient* freeswitch,
		RtmpObjectList& rtmpObjectList
		) {

	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"CamShareMiddleware::OnFreeswitchEventOnlineList( "
			"event : [内部服务(Freeswitch)-收到命令:同步用户状态列表], "
			"count : %d "
			")",
			rtmpObjectList.Size()
			);

	// 清空分站在线用户列表
	mSiteUserMap.Lock();
	for(SiteUserMap::iterator siteItr = mSiteUserMap.Begin(); siteItr != mSiteUserMap.End(); siteItr++) {
		UserMap *pUserMap = &siteItr->second;
		pUserMap->Lock();
		for(UserMap::iterator userItr = pUserMap->Begin(); userItr != pUserMap->End();) {
			SiteConnection *con = userItr->second;
			con->rtmpCount = 0;

			if ( con->IsOffline() ) {
				delete con;
				pUserMap->Erase(userItr++);
			} else {
				userItr++;
			}
		}
		pUserMap->Unlock();
	}
	mSiteUserMap.Unlock();

	// 分发用户到分站列表
	for(RtmpObjectList::iterator itr = rtmpObjectList.Begin(); itr != rtmpObjectList.End(); itr++) {
		mSiteUserMap.Lock();
		SiteUserMap::iterator siteItr = mSiteUserMap.Find(itr->siteId);
		if( siteItr != mSiteUserMap.End() ) {
			UserMap *pUserMap = &siteItr->second;
			pUserMap->Lock();
			UserMap::iterator userItr = pUserMap->Find(itr->user);
			if( userItr == pUserMap->End() ) {
				// 用户上线
				SiteConnection *con = new SiteConnection();
				con->rtmpCount = 1;
				pUserMap->Insert(itr->user, con);
			} else {
				// 增加连接数
				SiteConnection *con = userItr->second;
				con->rtmpCount++;
			}
			pUserMap->Unlock();
		}
		mSiteUserMap.Unlock();
	}

	// 通知Livechat重置在线列表
	SendAllOnlineList();
}

void CamShareMiddleware::OnFreeswitchEventOnlineStatus(
		FreeswitchClient* freeswitch,
		const RtmpObject& rtmpObject,
		bool online
		) {

	bool bFlag = false;
	int onlineCount = 0;

	if( rtmpObject.siteId.length() > 0 && rtmpObject.user.length() > 0 ) {
		ILiveChatClient* livechat = NULL;
		mLiveChatClientMap.Lock();
		LiveChatClientMap::iterator lcItr = mLiveChatClientMap.Find(rtmpObject.siteId);
		if( lcItr != mLiveChatClientMap.End() ) {
			livechat = lcItr->second;
		}
		mLiveChatClientMap.Unlock();

		// 更新在线状态
		mSiteUserMap.Lock();
		SiteUserMap::iterator itr = mSiteUserMap.Find(rtmpObject.siteId);
		if( itr != mSiteUserMap.End() ) {
			UserMap *pUserMap = &itr->second;
			pUserMap->Lock();
			UserMap::iterator userItr = pUserMap->Find(rtmpObject.user);
			if( online ) {
				if( userItr == pUserMap->End() ) {
					// 用户上线
					SiteConnection *con = new SiteConnection();
					con->rtmpCount = 1;

					pUserMap->Insert(rtmpObject.user, con);
					onlineCount = 1;
					bFlag = true;
				} else {
					// 增加连接数
					SiteConnection *con = userItr->second;
					onlineCount = ++con->rtmpCount;
				}

			} else if( userItr != pUserMap->End() ) {
				// 用户下线
				SiteConnection *con = userItr->second;
				onlineCount = --con->rtmpCount;
				if ( con->IsOffline() ) {
//				if( onlineCount == 0 ) {
					delete con;
					pUserMap->Erase(userItr);
					bFlag = true;
				}
			}
			pUserMap->Unlock();
		}

		if( bFlag ) {
			LogManager::GetLogManager()->Log(
					LOG_WARNING,
					"CamShareMiddleware::OnFreeswitchEventRtmpOnlineStatus( "
					"event : [内部服务(Freeswitch)-收到命令:用户改变在线状态], "
					"user : %s, "
					"online : %s, "
					"onlineCount : %d, "
					"siteId : %s "
					")",
					rtmpObject.user.c_str(),
					online?"true":"false",
					onlineCount,
					rtmpObject.siteId.c_str()
					);
			// 发送用户在线状态改变命令
			SendOnlineStatus2LiveChat(livechat, rtmpObject.user, online);
		}
		mSiteUserMap.Unlock();
	}
}
/***************************** 内部服务(Freeswitch) 回调处理 end **************************************/

/***************************** 外部服务(LiveChat) 回调处理 **************************************/
void CamShareMiddleware::OnConnect(
		ILiveChatClient* livechat,
		LCC_ERR_TYPE err,
		const string& errmsg
		) {
	if( err == LCC_ERR_SUCCESS ) {
		LogManager::GetLogManager()->Log(
				LOG_ERR_USER,
				"CamShareMiddleware::OnConnect( "
				"event : [外部服务(LiveChat)-连接服务器成功], "
				"siteId : %s, "
				"livechat : %p "
				")",
				livechat->GetSiteId().c_str(),
				livechat
				);

		// 创建用户Id列表
		list<string> userIdList;

		mSiteUserMap.Lock();
		SiteUserMap::iterator siteItr = mSiteUserMap.Find(livechat->GetSiteId());
		if( siteItr != mSiteUserMap.End() ) {
			UserMap userMap = siteItr->second;
			for(UserMap::iterator userItr = userMap.Begin(); userItr != userMap.End(); userItr++) {
				userIdList.push_back(userItr->first);
			}
			userMap.Unlock();
		}
		mSiteUserMap.Unlock();

		// 发送用户在线列表命令
		SendOnlineList2LiveChat(livechat, userIdList);

	} else {
		LogManager::GetLogManager()->Log(
				LOG_ERR_USER,
				"CamShareMiddleware::OnConnect( "
				"event : [外部服务(LiveChat)-连接服务器失败], "
				"err : %d, "
				"errmsg : %s, "
				"siteId : %s, "
				"livechat : %p "
				")",
				err,
				errmsg.c_str(),
				livechat->GetSiteId().c_str(),
				livechat
				);
	}

}

void CamShareMiddleware::OnDisconnect(
		ILiveChatClient* livechat,
		LCC_ERR_TYPE err,
		const string& errmsg
		) {
	LogManager::GetLogManager()->Log(
			LOG_ERR_USER,
			"CamShareMiddleware::OnDisconnect( "
			"event : [外部服务(LiveChat)-断开服务器], "
			"err : %d, "
			"errmsg : %s, "
			"siteId : %s, "
			"livechat : %p "
			")",
			err,
			errmsg.c_str(),
			livechat->GetSiteId().c_str(),
			livechat
			);

	mSessionManager.CloseSessionByLiveChat(livechat);

}

void CamShareMiddleware::OnSendEnterConference(
		ILiveChatClient* livechat,
		int seq,
		const string& fromId,
		const string& toId,
		const string& key,
		LCC_ERR_TYPE err,
		const string& errmsg
		) {
	if( err == LCC_ERR_SUCCESS ) {
		LogManager::GetLogManager()->Log(
				LOG_WARNING,
				"CamShareMiddleware::OnSendEnterConference( "
				"event : [外部服务(LiveChat)-发送命令:进入会议室认证-成功], "
				"fromId : %s, "
				"toId : %s, "
				"key : %s, "
				"siteId : %s, "
				"seq : %d, "
				"livechat : %p "
				")",
				fromId.c_str(),
				toId.c_str(),
				key.c_str(),
				livechat->GetSiteId().c_str(),
				seq,
				livechat
				);

	} else {
		LogManager::GetLogManager()->Log(
				LOG_ERR_USER,
				"CamShareMiddleware::OnSendEnterConference( "
				"event : [外部服务(LiveChat)-发送命令:进入会议室认证-失败], "
				"fromId : %s, "
				"toId : %s "
				"key : %s, "
				"err : %d, "
				"errmsg : %s, "
				"siteId : %s, "
				"seq : %d, "
				"livechat : %p "
				")",
				fromId.c_str(),
				toId.c_str(),
				key.c_str(),
				err,
				errmsg.c_str(),
				livechat->GetSiteId().c_str(),
				seq,
				livechat
				);
		// 断开用户
		mFreeswitch.KickUserFromConference(fromId, toId, "");
	}

}

void CamShareMiddleware::OnRecvEnterConference(
		ILiveChatClient* livechat,
		int seq,
		const string& fromId,
		const string& toId,
		const string& key,
		bool bAuth,
		LCC_ERR_TYPE err,
		const string& errmsg
		) {

	// 结束会话任务
	string identify = EnterConferenceRequest::GetIdentify(fromId, toId, key);
	IRequest* request = mSessionManager.FinishSessionByCustomIdentify(livechat, identify);

	if( request ) {
		if( err == LCC_ERR_SUCCESS && bAuth ) {
			LogManager::GetLogManager()->Log(
					LOG_WARNING,
					"CamShareMiddleware::OnRecvEnterConference( "
					"event : [外部服务(LiveChat)-收到命令:进入会议室认证结果-成功], "
					"fromId : %s, "
					"toId : %s, "
					"key : %s, "
					"bAuth : %s, "
					"siteId : %s, "
					"seq : %d, "
					"livechat : %p, "
					"request : %p "
					")",
					fromId.c_str(),
					toId.c_str(),
					key.c_str(),
					bAuth?"true":"false",
					livechat->GetSiteId().c_str(),
					seq,
					livechat,
					request
					);

			string strKey = key;
			transform(strKey.begin(), strKey.end(), strKey.begin(), ::tolower);
			if( strKey == "active" ) {
				// 主动请求验证
				mFreeswitch.StartUserRecvVideo(fromId, toId, ((EnterConferenceRequest*)request)->GetMemberType());
			}

		} else {
			LogManager::GetLogManager()->Log(
					LOG_ERR_USER,
					"CamShareMiddleware::OnRecvEnterConference( "
					"event : [外部服务(LiveChat)-收到命令:进入会议室认证结果-失败], "
					"fromId : %s, "
					"toId : %s, "
					"key : %s, "
					"bAuth : %s, "
					"err : %d, "
					"errmsg : %s, "
					"siteId : %s, "
					"seq : %d, "
					"livechat : %p, "
					"request : %p "
					")",
					fromId.c_str(),
					toId.c_str(),
					key.c_str(),
					bAuth?"true":"false",
					err,
					errmsg.c_str(),
					livechat->GetSiteId().c_str(),
					seq,
					livechat,
					request
					);

			// 断开用户
			mFreeswitch.KickUserFromConference(fromId, toId, "");
		}
	} else {
		// 找不到任务，已经超时
		LogManager::GetLogManager()->Log(
				LOG_WARNING,
				"CamShareMiddleware::OnRecvEnterConference( "
				"event : [外部服务(LiveChat)-收到命令:进入会议室认证结果-失败-找不到任务-已经超时], "
				"fromId : %s, "
				"toId : %s, "
				"key : %s, "
				"bAuth : %s, "
				"siteId : %s, "
				"seq : %d, "
				"livechat : %p "
				")",
				fromId.c_str(),
				toId.c_str(),
				key.c_str(),
				bAuth?"true":"false",
				livechat->GetSiteId().c_str(),
				seq,
				livechat
				);
	}

	// 释放任务
	if( request != NULL ) {
		delete request;
		request = NULL;
	}
}

void CamShareMiddleware::OnRecvKickUserFromConference(
		ILiveChatClient* livechat,
		int seq,
		const string& fromId,
		const string& toId,
		LCC_ERR_TYPE err,
		const string& errmsg) {
	if( err == LCC_ERR_SUCCESS ) {
		LogManager::GetLogManager()->Log(
				LOG_WARNING,
				"CamShareMiddleware::OnRecvKickUserFromConference( "
				"event : [外部服务(LiveChat)-收到命令:从会议室踢出用户], "
				"fromId : %s, "
				"toId : %s, "
				"siteId : %s, "
				"seq : %d, "
				"livechat : %p "
				")",
				fromId.c_str(),
				toId.c_str(),
				livechat->GetSiteId().c_str(),
				seq,
				livechat
				);

		if( fromId != toId ) {
			// 互踢
			// 从会议室(toId)踢出用户(fromId)
			mFreeswitch.KickUserFromConference(fromId, toId, "");
			// 从会议室(manId)踢出用户(womanId)
			mFreeswitch.KickUserFromConference(toId, fromId, "");
		} else {
			// 随便踢一次
			mFreeswitch.KickUserFromConference(fromId, toId, "");
		}
	}
}

void CamShareMiddleware::OnRecvGetOnlineList(ILiveChatClient* livechat, int seq, LCC_ERR_TYPE err, const string& errmsg) {
	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"CamShareMiddleware::OnRecvGetOnlineList( "
			"event : [外部服务(LiveChat)-收到命令:获取用户在线列表], "
			"siteId : %s, "
			"seq : %d, "
			"livechat : %p "
			")",
			livechat->GetSiteId().c_str(),
			seq,
			livechat
			);

	if( err == LCC_ERR_SUCCESS ) {
		// 创建用户Id列表
		list<string> userIdList;

		mSiteUserMap.Lock();
		SiteUserMap::iterator siteItr = mSiteUserMap.Find(livechat->GetSiteId());
		if( siteItr != mSiteUserMap.End() ) {
			UserMap userMap = siteItr->second;
			for(UserMap::iterator userItr = userMap.Begin(); userItr != userMap.End(); userItr++) {
				userIdList.push_back(userItr->first);
			}
			userMap.Unlock();
		}
		mSiteUserMap.Unlock();

		// 发送用户在线列表命令
		SendOnlineList2LiveChat(livechat, userIdList);
	}
}

/***************************** 外部服务(LiveChat) 回调处理 end **************************************/

/***************************** 外部服务接口 **************************************/
bool CamShareMiddleware::SendEnterConference2LiveChat(
		ILiveChatClient* livechat,
		const string& fromId,
		const string& toId,
		MemberType type,
		const string& serverId,
		EnterConferenceRequestCheckType checkType,
        ENTERCONFERENCETYPE chatType
		) {
	bool bFlag = true;

	string siteId = "";
	// LiveChat client生成请求序列号
	int seq = -1;
	if( livechat != NULL ) {
		seq = livechat->GetSeq();
		siteId = livechat->GetSiteId();

	} else {
		bFlag = false;
	}

	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"CamShareMiddleware::SendEnterConference2LiveChat( "
			"event : [外部服务(LiveChat)-发送命令:进入会议室认证], "
			"fromId : %s, "
			"toId : %s, "
			"type : %u, "
			"serverId : %s, "
			"siteId : %s, "
            "chatType : %d "
			")",
			fromId.c_str(),
			toId.c_str(),
			type,
			serverId.c_str(),
			siteId.c_str(),
            chatType
			);

	if( bFlag ) {
		bFlag = false;

		// 处理是否需要记录请求
		EnterConferenceRequest* request = new EnterConferenceRequest();
		request->SetParam(&mFreeswitch, livechat, seq, serverId, fromId, toId, type, checkType, chatType);

		// 生成会话
		string key = request->GetKey();
		string identify = EnterConferenceRequest::GetIdentify(fromId, toId, key);
		if( mSessionManager.StartSessionByCustomIdentify(livechat, request, identify) ) {
			// 发送请求成功
			LogManager::GetLogManager()->Log(
					LOG_WARNING,
					"CamShareMiddleware::SendEnterConference2LiveChat( "
					"event : [外部服务(LiveChat)-发送命令:进入会议室认证-生成会话成功], "
					"fromId : %s, "
					"toId : %s, "
					"type : %u, "
					"serverId : %s, "
					"key : %s, "
					"siteId : %s, "
					"livechat : %p, "
					"request : %p "
					")",
					fromId.c_str(),
					toId.c_str(),
					type,
					serverId.c_str(),
					key.c_str(),
					siteId.c_str(),
					livechat,
					request
					);
			bFlag = true;

		}
	}

	if( !bFlag ) {
		LogManager::GetLogManager()->Log(
				LOG_ERR_USER,
				"CamShareMiddleware::SendEnterConference2LiveChat( "
				"event : [外部服务(LiveChat)-发送命令:进入会议室认证-失败], "
				"fromId : %s, "
				"toId : %s, "
				"type : %u, "
				"serverId : %s, "
				"siteId : %s "
				")",
				fromId.c_str(),
				toId.c_str(),
				type,
				serverId.c_str(),
				siteId.c_str()
				);
	}

	return bFlag;
}

bool CamShareMiddleware::SendMsgEnterConference2LiveChat(
		ILiveChatClient* livechat,
		const string& fromId,
		const string& toId,
		const list<string>& userList
		) {
	bool bFlag = true;

	string siteId = "";
	// LiveChat client生成请求序列号
	int seq = -1;
	if( livechat != NULL ) {
		seq = livechat->GetSeq();
		siteId = livechat->GetSiteId().c_str();
	} else {
		bFlag = false;
	}

	SendMsgEnterConferenceRequest request;
	request.SetParam(&mFreeswitch, livechat, seq, fromId, toId, userList);

	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"CamShareMiddleware::SendMsgEnterConference2LiveChat( "
			"event : [外部服务(LiveChat)-发送命令:通知客户端用户进入会议室], "
			"fromId : %s, "
			"toId : %s, "
			"param : %s, "
			"siteId : %s "
			")",
			fromId.c_str(),
			toId.c_str(),
			request.ParamString().c_str(),
			siteId.c_str()
			);

	if( bFlag ) {
		bFlag = request.StartRequest();

		if( bFlag ) {
			// 发送请求成功
			LogManager::GetLogManager()->Log(
					LOG_MSG,
					"CamShareMiddleware::SendMsgEnterConference2LiveChat( "
					"event : [外部服务(LiveChat)-发送命令:通知客户端用户进入会议室-成功], "
					"fromId : %s, "
					"toId : %s, "
					"param : %s, "
					"siteId : %s "
					")",
					fromId.c_str(),
					toId.c_str(),
					request.ParamString().c_str(),
					siteId.c_str()
					);
		}
	}

	if( !bFlag ) {
		LogManager::GetLogManager()->Log(
				LOG_ERR_USER,
				"CamShareMiddleware::SendMsgEnterConference2LiveChat( "
				"event : [外部服务(LiveChat)-发送命令:通知客户端用户进入会议室-失败], "
				"fromId : %s, "
				"toId : %s, "
				"param : %s, "
				"siteId : %s "
				")",
				fromId.c_str(),
				toId.c_str(),
				request.ParamString().c_str(),
				siteId.c_str()
				);
	}

	return bFlag;
}

bool CamShareMiddleware::SendMsgExitConference2LiveChat(
		ILiveChatClient* livechat,
		const string& fromId,
		const string& toId,
		const list<string>& userList
		) {
	bool bFlag = true;

	string siteId = "";
	// LiveChat client生成请求序列号
	int seq = -1;
	if( livechat != NULL ) {
		seq = livechat->GetSeq();
		siteId = livechat->GetSiteId().c_str();
	} else {
		bFlag = false;
	}

	SendMsgExitConferenceRequest request;
	request.SetParam(&mFreeswitch, livechat, seq, fromId, toId, userList);

	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"CamShareMiddleware::SendMsgExitConference2LiveChat( "
			"event : [外部服务(LiveChat)-发送命令:通知客户端用户退出会议室], "
			"fromId : %s, "
			"toId : %s, "
			"param : %s, "
			"siteId : %s "
			")",
			fromId.c_str(),
			toId.c_str(),
			request.ParamString().c_str(),
			siteId.c_str()
			);

	if( bFlag ) {
		bFlag = request.StartRequest();

		if( bFlag ) {
			// 发送请求成功
			LogManager::GetLogManager()->Log(
					LOG_MSG,
					"CamShareMiddleware::SendMsgExitConference2LiveChat( "
					"event : [外部服务(LiveChat)-发送命令:通知客户端用户退出会议室-成功], "
					"fromId : %s, "
					"toId : %s, "
					"param : %s, "
					"siteId : %s "
					")",
					fromId.c_str(),
					toId.c_str(),
					request.ParamString().c_str(),
					siteId.c_str()
					);
		}
	}

	if( !bFlag ) {
		LogManager::GetLogManager()->Log(
				LOG_ERR_USER,
				"CamShareMiddleware::SendMsgExitConference2LiveChat( "
				"event : [外部服务(LiveChat)-发送命令:通知客户端用户退出会议室-失败], "
				"fromId : %s, "
				"toId : %s, "
				"param : %s, "
				"siteId : %s "
				")",
				fromId.c_str(),
				toId.c_str(),
				request.ParamString().c_str(),
				siteId.c_str()
				);
	}

	return bFlag;
}

bool CamShareMiddleware::SendOnlineStatus2LiveChat(
		ILiveChatClient* livechat,
		const string& userId,
		bool online
		) {
	bool bFlag = true;

	string siteId = "";
	// LiveChat client生成请求序列号
	int seq = -1;
	if( livechat != NULL ) {
		seq = livechat->GetSeq();
		siteId = livechat->GetSiteId().c_str();
	} else {
		bFlag = false;
	}

	OnlineStatusRequest request;
	request.SetParam(&mFreeswitch, livechat, seq, userId, online);

	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"CamShareMiddleware::SendOnlineStatus2LiveChat( "
			"event : [外部服务(LiveChat)-发送命令:通知用户在线状态改变], "
			"userId : %s, "
			"online : %s, "
			"siteId : %s "
			")",
			userId.c_str(),
			online?"true":"false",
			siteId.c_str()
			);

	if( bFlag ) {
		bFlag = request.StartRequest();

		if( bFlag ) {
			// 发送请求成功
			LogManager::GetLogManager()->Log(
					LOG_MSG,
					"CamShareMiddleware::SendOnlineStatus2LiveChat( "
					"event : [外部服务(LiveChat)-发送命令:通知用户在线状态改变-成功], "
					"userId : %s, "
					"online : %s, "
					"siteId : %s "
					")",
					userId.c_str(),
					online?"true":"false",
					siteId.c_str()
					);
		}
	}

	if( !bFlag ) {
		LogManager::GetLogManager()->Log(
				LOG_ERR_USER,
				"CamShareMiddleware::SendOnlineStatus2LiveChat( "
				"event : [外部服务(LiveChat)-发送命令:通知用户在线状态改变-失败], "
				"userId : %s, "
				"online : %s, "
				"siteId : %s "
				")",
				userId.c_str(),
				online?"true":"false",
				siteId.c_str()
				);
	}

	return bFlag;
}

bool CamShareMiddleware::SendOnlineList2LiveChat(
		ILiveChatClient* livechat,
		const list<string>& userList
		) {
	bool bFlag = true;

	string siteId = "";
	// LiveChat client生成请求序列号
	int seq = -1;
	if( livechat != NULL ) {
		seq = livechat->GetSeq();
		siteId = livechat->GetSiteId().c_str();
	} else {
		bFlag = false;
	}

	OnlineListRequest request;
	request.SetParam(&mFreeswitch, livechat, seq, userList);

	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"CamShareMiddleware::SendOnlineList2LiveChat( "
			"event : [外部服务(LiveChat)-发送命令:通知用户在线列表], "
			"count : %d, "
			"siteId : %s "
			")",
			userList.size(),
			siteId.c_str()
			);

	if( bFlag ) {
		bFlag = request.StartRequest();

		if( bFlag ) {
			// 发送请求成功
			LogManager::GetLogManager()->Log(
					LOG_MSG,
					"CamShareMiddleware::SendOnlineList2LiveChat( "
					"event : [外部服务(LiveChat)-发送命令:通知用户在线列表-成功], "
					"count : %d, "
					"siteId : %s "
					")",
					userList.size(),
					siteId.c_str()
					);
		}
	}

	if( !bFlag ) {
		LogManager::GetLogManager()->Log(
				LOG_ERR_USER,
				"CamShareMiddleware::SendOnlineList2LiveChat( "
				"event : [外部服务(LiveChat)-发送命令:通知用户在线列表-失败], "
				"count : %d, "
				"siteId : %s "
				")",
				userList.size(),
				siteId.c_str()
				);
	}

	return bFlag;
}

bool CamShareMiddleware::SendRecordFinish(
		HttpClient* client,
		const Record& record,
		bool &success,
		bool &errorRecord,
		string& errorCode
		) {
	bool bFlag = false;
	success = false;
	errorRecord = false;
	errorCode = "";

	long httpCode = 0;
	const char* respond = NULL;
	int respondSize = 0;
	HttpEntiy httpEntiy;

	httpEntiy.SetAuthorization("test", "5179");
	httpEntiy.AddContent("userId", record.conference);
	httpEntiy.AddContent("startTime", record.startTime);
	httpEntiy.AddContent("endTime", record.endTime);
	httpEntiy.AddContent("fileName", record.filePath);

	char content[128];
	SimulatorProtocolTool tool;
	unsigned int checkcode = tool.EncodeValue(true);
	sprintf(content, "%08x", checkcode);
	httpEntiy.AddContent("checkcode", content);

	string checkInfo = tool.EncodeDesc(record.filePath, checkcode);
	httpEntiy.AddContent("checkkey", checkInfo);

	string url = "";
	mSiteConfigMap.Lock();
	SiteConfigMap::iterator itr = mSiteConfigMap.Find(record.siteId);
	if( itr != mSiteConfigMap.End() ) {
		SiteConfig* pConfig = itr->second;
		url = pConfig->recordFinishUrl;
	}
	mSiteConfigMap.Unlock();

	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"CamShareMiddleware::SendRecordFinish( "
			"event : [发送录制文件完成记录到服务器], "
			"url : %s, "
			"id : %s, "
			"conference : %s, "
			"siteId : %s, "
			"filePath : %s, "
			"startTime : %s, "
			"endTime : %s "
			")",
			url.c_str(),
			record.id.c_str(),
			record.conference.c_str(),
			record.siteId.c_str(),
			record.filePath.c_str(),
			record.startTime.c_str(),
			record.endTime.c_str()
			);

	if( url.length() == 0 ) {
		// 找不到对应url
		errorRecord = true;
	}

	if( !errorRecord && client->Request(url.c_str(), &httpEntiy) ) {
		// 解析返回
		httpCode = client->GetRespondCode();
		client->GetBody(&respond, respondSize);
		LogManager::GetLogManager()->Log(
				LOG_WARNING,
				"CamShareMiddleware::SendRecordFinish( "
				"event : [发送录制文件完成记录到服务器, 返回], "
				"url : %s, "
				"id : %s, "
				"conference : %s, "
				"siteId : %s, "
				"filePath : %s, "
				"startTime : %s, "
				"endTime : %s, "
				"httpCode : %ld, "
				"respondSize : %d, "
				"respond : %s "
				")",
				url.c_str(),
				record.id.c_str(),
				record.conference.c_str(),
				record.siteId.c_str(),
				record.filePath.c_str(),
				record.startTime.c_str(),
				record.endTime.c_str(),
				httpCode,
				respondSize,
				respond
				);
		if( httpCode == 200 ) {
			if( respondSize > 0 ) {
				// 发送成功
				Json::Value root;
				Json::Reader reader;
				if( reader.parse(respond, root, false) ) {
					// 解析协议成功, 标记为发送成功
					bFlag = true;

					if( root["result"].isInt() ) {
						int result = root["result"].asInt();
						switch(result) {
						case 0:{
							// 上传失败
							if( root["errno"].isString() ) {
								// 解析错误码
								errorCode = root["errno"].asString();
							}
						}break;
						case 1:{
							// 上传成功
							success = true;

						}break;
						}
					}
				} else {
					// 标记为错误记录, 手工处理
					errorRecord = true;
				}
			} else {
				// 标记为错误记录, 手工处理
				errorRecord = true;
			}

		} else {
			// 不是200 OK记录继续发送
		}
	}

	if( !success ) {
		LogManager::GetLogManager()->Log(
				LOG_ERR_USER,
				"CamShareMiddleware::SendRecordFinish( "
				"event : [发送录制文件完成记录到服务器-失败], "
				"url : %s, "
				"id : %s, "
				"conference : %s, "
				"siteId : %s, "
				"filePath : %s, "
				"startTime : %s, "
				"endTime : %s, "
				"respondSize : %d, "
				"respond : %s "
				")",
				url.c_str(),
				record.id.c_str(),
				record.conference.c_str(),
				record.siteId.c_str(),
				record.filePath.c_str(),
				record.startTime.c_str(),
				record.endTime.c_str(),
				respondSize,
				respond
				);
	}

	return bFlag;
}

/***************************** 外部服务接口 end **************************************/

void CamShareMiddleware::GetExtParameters(const string& wholeLine, string& userId, string& siteId) {
	string key;
	string value;

	string line;
	string::size_type posSep;
	string::size_type index = 0;
	string::size_type pos;

	while( string::npos != index ) {
		pos = wholeLine.find("&", index);
		if( string::npos != pos ) {
			// 找到分隔符
			line = wholeLine.substr(index, pos - index);
			// 移动下标
			index = pos + 1;

		} else {
			// 是否最后一次
			if( index < wholeLine.length() ) {
				line = wholeLine.substr(index, pos - index);
				// 移动下标
				index = string::npos;

			} else {
				// 找不到
				index = string::npos;
				break;
			}
		}

		posSep = line.find("=");
		if( (string::npos != posSep) && (posSep + 1 < line.length()) ) {
			key = line.substr(0, posSep);
			value = line.substr(posSep + 1, line.length() - (posSep + 1));

		} else {
			key = line;
		}

		if ( key == "userId" ) {
			userId = value;
		} else if ( key == "siteId" ) {
			siteId = value;
		}

	}
}
/***************************** 测试函数 **************************************/
bool CamShareMiddleware::CheckTestAccount(
		const string& user
		) {
	bool bFlag = false;
	if( user.find("WW") == 0 || user.find("MM") == 0 ) {
		LogManager::GetLogManager()->Log(
				LOG_STAT,
				"CamShareMiddleware::CheckTestAccount( "
				"event : [检测到测试账号], "
				"user : %s "
				")",
				user.c_str()
				);
		bFlag = true;
	}
	return bFlag;
}
/***************************** 测试函数 end **************************************/

void CamShareMiddleware::SendAllOnlineList() {
	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"CamShareMiddleware::SendAllOnlineList( "
			"event : [发送所有站点在线用户列表到Livechat] "
			")"
			);

	// 通知Livechat重置在线列表
	mSiteUserMap.Lock();
	for(SiteUserMap::iterator siteItr = mSiteUserMap.Begin(); siteItr != mSiteUserMap.End(); siteItr++) {
		// 创建用户Id列表
		list<string> userIdList;

		UserMap *pUserMap = &siteItr->second;
		pUserMap->Lock();
		for(UserMap::iterator userItr = pUserMap->Begin(); userItr != pUserMap->End(); userItr++) {
			userIdList.push_back(userItr->first);
		}
		pUserMap->Unlock();

		ILiveChatClient* livechat = NULL;
		string siteId = siteItr->first;
		mLiveChatClientMap.Lock();
		LiveChatClientMap::iterator itr = mLiveChatClientMap.Find(siteId);
		if( itr != mLiveChatClientMap.End() ) {
			livechat = itr->second;
		}
		mLiveChatClientMap.Unlock();

		// 发送用户在线列表命令
		SendOnlineList2LiveChat(livechat, userIdList);
	}
	mSiteUserMap.Unlock();
}

bool CamShareMiddleware::SendExternalLoginCheck(const string& siteId, const string& param) {
	bool bFlag = false;

	HttpClient client;
	HttpEntiy httpEntiy;
	httpEntiy.SetAuthorization("test", "5179");

	bool loginCheck = true;

	string url = "";
	mSiteConfigMap.Lock();
	SiteConfigMap::iterator itr = mSiteConfigMap.Find(siteId);
	if( itr != mSiteConfigMap.End() ) {
		SiteConfig* pConfig = itr->second;
		url = pConfig->loginCheckUrl;
		loginCheck = pConfig->loginCheck;
	}
	mSiteConfigMap.Unlock();

	if ( loginCheck ) {
		LogManager::GetLogManager()->Log(
				LOG_WARNING,
				"CamShareMiddleware::SendExternalLoginCheck( "
				"event : [外部登录校验], "
				"siteId : %s, "
				"url : %s, "
				"param : %s "
				")",
				siteId.c_str(),
				url.c_str(),
				param.c_str()
				);

		if( url.length() > 0 ) {
			url += "&";
			url += param;

			if( client.Request(url.c_str(), &httpEntiy) ) {
				// 解析返回
				const char* respond = NULL;
				int respondSize = 0;
				int httpCode = client.GetRespondCode();
				client.GetBody(&respond, respondSize);

				LogManager::GetLogManager()->Log(
						LOG_WARNING,
						"CamShareMiddleware::SendExternalLoginCheck( "
						"event : [外部登录校验, 返回], "
						"siteId : %s, "
						"url : %s, "
						"param : %s, "
						"httpCode : %ld, "
						"respondSize : %d, "
						"respond : %s "
						")",
						siteId.c_str(),
						url.c_str(),
						param.c_str(),
						httpCode,
						respondSize,
						respond
						);

				if( httpCode == 200 ) {
					if( respondSize > 0 ) {
						// 发送成功
						Json::Value root;
						Json::Reader reader;
						if( reader.parse(respond, root, false) ) {
							// 解析协议成功, 标记为发送成功
							if( root["result"].isInt() ) {
								int result = root["result"].asInt();
								bFlag = (result == 1);
							}
						}
					}
				}
			}
		} else {
			LogManager::GetLogManager()->Log(
					LOG_ERR_USER,
					"CamShareMiddleware::SendExternalLoginCheck( "
					"event : [外部登录校验, 参数错误], "
					"siteId : %s, "
					"url : %s, "
					"param : %s "
					")",
					siteId.c_str(),
					url.c_str(),
					param.c_str()
					);
		}
	} else {
		bFlag = true;
	}

	return bFlag;
}
