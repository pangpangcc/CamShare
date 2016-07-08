/*
 * CamShareMiddleware.cpp
 *
 *  Created on: 2015-1-13
 *      Author: Max.Chiu
 *      Email: Kingsleyyau@gmail.com
 */

#include "CamShareMiddleware.h"

#include <sys/syscall.h>

#include <request/EnterConferenceRequest.h>
#include <request/SendMsgEnterConferenceRequest.h>
#include <request/SendMsgExitConferenceRequest.h>

#include <respond/DialplanRespond.h>

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
/***************************** 线程处理 end **************************************/

CamShareMiddleware::CamShareMiddleware() {
	// TODO Auto-generated constructor stub
	// 处理线程
	mpStateRunnable = new StateRunnable(this);
	mpStateThread = NULL;
	mpConnectLiveChatRunnable = new ConnectLiveChatRunnable(this);
	mpLiveChatConnectThread = NULL;
	mpConnectFreeswitchRunnable = new ConnectFreeswitchRunnable(this);
	mpConnectFreeswitchThread = NULL;

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
	mAuthorization = false;
	mLivechatName = "";

	// Freeswitch服务器参数
	miFreeswitchPort = 0;
	mFreeswitchIp = "";
	mFreeswitchUser = "";
	mFreeswitchPassword = "";
	mFreeswitchRecordingPath = "";
	mbFreeswitchIsRecording = false;

	// 统计参数
	mResponed = 0;
	mTotal = 0;
	miSendEnterConference = 0;

	// 其他
	mIsRunning = false;

	mFreeswitch.SetFreeswitchClientListener(this);
}

CamShareMiddleware::~CamShareMiddleware() {
	// TODO Auto-generated destructor stub
	if( mpStateRunnable ) {
		delete mpStateRunnable;
	}

	if( mpConnectLiveChatRunnable ) {
		delete mpConnectLiveChatRunnable;
	}

	if( mpConnectFreeswitchRunnable ) {
		delete mpConnectFreeswitchRunnable;
	}
}

bool CamShareMiddleware::Run(const string& config) {
	if( config.length() > 0 ) {
		mConfigFile = config;

		// Reload config
		if( Reload() ) {
			if( miDebugMode == 1 ) {
				LogManager::LogSetFlushBuffer(0);
			} else {
				LogManager::LogSetFlushBuffer(5 * BUFFER_SIZE_1K * BUFFER_SIZE_1K);
			}

			return Run();

		} else {
			printf("# CamShareMiddleware can not load config file exit. \n");
		}

	} else {
		printf("# No config file can be use exit. \n");
	}

	return false;
}

bool CamShareMiddleware::Run() {
	/* log system */
	LogManager::GetLogManager()->Start(1000, miLogLevel, mLogDir);
	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"CamShareMiddleware::Run( "
			"############## CamShare Middleware ############## "
			")"
			);

	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"CamShareMiddleware::Run( "
			"Version : %s "
			")",
			VERSION_STRING
			);

	// 基本参数
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"CamShareMiddleware::Run( "
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
			LOG_MSG,
			"CamShareMiddleware::Run( "
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
			LOG_MSG,
			"CamShareMiddleware::Run( "
			"miLivechatPort : %d, "
			"mLivechatIp : %s, "
			"mAuthorization : %d "
			"mLivechatName : %s "
			")",
			miLivechatPort,
			mLivechatIp.c_str(),
			mAuthorization,
			mLivechatName.c_str()
			);

	// Freeswitch参数
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"CamShareMiddleware::Run( "
			"miFreeswitchPort : %d, "
			"mFreeswitchIp : %s, "
			"mFreeswitchUser : %s, "
			"mFreeswitchPassword : %s, "
			"mbFreeswitchIsRecording : %d, "
			"mFreeswitchRecordingPath : %s "
			")",
			miFreeswitchPort,
			mFreeswitchIp.c_str(),
			mFreeswitchUser.c_str(),
			mFreeswitchPassword.c_str(),
			mbFreeswitchIsRecording,
			mFreeswitchRecordingPath.c_str()
			);

	bool bFlag = false;

	/**
	 * 客户端解析包缓存buffer
	 */
	for(int i = 0; i < miMaxClient; i++) {
		/* create idle buffers */
		Message *m = new Message();
		mIdleMessageList.PushBack(m);
	}

	/**
	 * 会话管理器
	 * 设置超时
	 */
	mSessionManager.SetRequestTimeout(miFlashTimeout);

	/* HTTP server */
	/**
	 * 预估相应时间, 内存数目*超时间隔*每秒处理的任务
	 */
	mClientTcpServer.SetTcpServerObserver(this);
	mClientTcpServer.SetHandleSize(miTimeout * miMaxQueryPerThread);
	bFlag = mClientTcpServer.Start(miMaxClient, miPort, miMaxHandleThread);
	if( !bFlag ) {
		LogManager::GetLogManager()->Log(LOG_STAT, "CamShareMiddleware::Run( TcpServer Init Fail )");
		return false;
	}
	LogManager::GetLogManager()->Log(LOG_STAT, "CamShareMiddleware::Run( TcpServer Init OK )");

	/* LiveChat server */
	/**
	 * 初始化LiveChat站点
	 */
	mLiveChatClientMap.Lock();
	list<string> ips;
	ips.push_back(mLivechatIp);
	ILiveChatClient* cl = ILiveChatClient::CreateClient();
	cl->Init(ips, miLivechatPort, this);
	mLiveChatClientMap.Insert(SITE_TYPE_CL, cl);
	LogManager::GetLogManager()->Log(LOG_STAT, "CamShareMiddleware::Run( "
			"[创建外部服务(LiveChat)], "
			"cl : %p, "
			"siteId : %d "
			")",
			cl,
			SITE_TYPE_CL
			);
	ILiveChatClient* ida = ILiveChatClient::CreateClient();
	ida->Init(ips, miLivechatPort, this);
	mLiveChatClientMap.Insert(SITE_TYPE_IDA, ida);
	LogManager::GetLogManager()->Log(LOG_STAT, "CamShareMiddleware::Run( "
			"[创建外部服务(LiveChat)], "
			"ida : %p "
			"siteId : %d "
			")",
			ida,
			SITE_TYPE_IDA
			);
	ILiveChatClient* cd = ILiveChatClient::CreateClient();
	cd->Init(ips, miLivechatPort, this);
	mLiveChatClientMap.Insert(SITE_TYPE_CD, cd);
	LogManager::GetLogManager()->Log(LOG_STAT, "CamShareMiddleware::Run( "
			"[创建外部服务(LiveChat)], "
			"cd : %p "
			"siteId : %d "
			")",
			cd,
			SITE_TYPE_CD
			);
	ILiveChatClient* ld = ILiveChatClient::CreateClient();
	ld->Init(ips, miLivechatPort, this);
	mLiveChatClientMap.Insert(SITE_TYPE_LD, ld);
	LogManager::GetLogManager()->Log(LOG_STAT, "CamShareMiddleware::Run( "
			"[创建外部服务(LiveChat)], "
			"ld : %p "
			"siteId : %d "
			")",
			ld,
			SITE_TYPE_LD
			);
	mLiveChatClientMap.Unlock();

	/**
	 * 初始化Freeswitch
	 */
	mFreeswitch.SetRecording(mbFreeswitchIsRecording, mFreeswitchRecordingPath);

	mIsRunning = true;

	// 开始LiveChat连接线程
	mpLiveChatConnectThread = new KThread(mpConnectLiveChatRunnable);
	if( mpLiveChatConnectThread->start() != 0 ) {
	}

	// 开始Freeswitch连接线程
	mpConnectFreeswitchThread = new KThread(mpConnectFreeswitchRunnable);
	if( mpConnectFreeswitchThread->start() != 0 ) {
	}

	// 开始状态监视线程
	mpStateThread = new KThread(mpStateRunnable);
	if( mpStateThread->start() != 0 ) {
	}

	// 服务启动成功
	printf("# CamShareMiddleware start OK. \n");
	LogManager::GetLogManager()->Log(LOG_WARNING, "CamShareMiddleware::Run( Init OK )");

	return true;
}

bool CamShareMiddleware::Reload() {
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

			LogManager::GetLogManager()->Log(
					LOG_MSG,
					"CamShareMiddleware::Reload( "
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

			mClientTcpServer.SetHandleSize(miTimeout * miMaxQueryPerThread);

			// 日志参数
			miLogLevel = atoi(conf.GetPrivate("LOG", "LOGLEVEL", "5").c_str());
			mLogDir = conf.GetPrivate("LOG", "LOGDIR", "log");
			miDebugMode = atoi(conf.GetPrivate("LOG", "DEBUGMODE", "0").c_str());

			LogManager::GetLogManager()->Log(
					LOG_MSG,
					"CamShareMiddleware::Reload( "
					"miDebugMode : %d, "
					"miLogLevel : %d, "
					"mlogDir : %s "
					")",
					miDebugMode,
					miLogLevel,
					mLogDir.c_str()
					);

			// Livechat参数
			miLivechatPort = atoi(conf.GetPrivate("LIVECHAT", "PORT", "9877").c_str());
			mLivechatIp = conf.GetPrivate("LIVECHAT", "IP", "127.0.0.1");
			mAuthorization = atoi(conf.GetPrivate("LIVECHAT", "AUTHORIZATION_ALL_CONFERENCE", "0").c_str());
			mLivechatName = conf.GetPrivate("LIVECHAT", "NAME", "CAM");

			LogManager::GetLogManager()->Log(
					LOG_MSG,
					"CamShareMiddleware::Reload( "
					"miLivechatPort : %d, "
					"mLivechatIp : %s, "
					"mAuthorization : %d, "
					"mLivechatName : %s, "
					")",
					miLivechatPort,
					mLivechatIp.c_str(),
					mAuthorization,
					mLivechatName.c_str()
					);

			// Freeswitch参数
			miFreeswitchPort = atoi(conf.GetPrivate("FREESWITCH", "PORT", "8021").c_str());
			mFreeswitchIp = conf.GetPrivate("FREESWITCH", "IP", "127.0.0.1");
			mFreeswitchUser = conf.GetPrivate("FREESWITCH", "USER", "");
			mFreeswitchPassword = conf.GetPrivate("FREESWITCH", "PASSWORD", "ClueCon");
			mbFreeswitchIsRecording = atoi(conf.GetPrivate("FREESWITCH", "ISRECORDING", "0").c_str());
			mFreeswitchRecordingPath = conf.GetPrivate("FREESWITCH", "RECORDINGPATH", "/usr/local/freeswitch/recordings");

			LogManager::GetLogManager()->Log(
					LOG_MSG,
					"CamShareMiddleware::Reload( "
					"miFreeswitchPort : %d, "
					"mFreeswitchIp : %s, "
					"mFreeswitchUser : %s, "
					"mFreeswitchPassword : %s, "
					"mbFreeswitchIsRecording : %d, "
					"mFreeswitchRecordingPath : %s "
					")",
					miFreeswitchPort,
					mFreeswitchIp.c_str(),
					mFreeswitchUser.c_str(),
					mFreeswitchPassword.c_str(),
					mbFreeswitchIsRecording,
					mFreeswitchRecordingPath.c_str()
					);

			bFlag = true;
		}
	}
	mConfigMutex.unlock();
	return bFlag;
}

bool CamShareMiddleware::IsRunning() {
	return mIsRunning;
}

bool CamShareMiddleware::OnAccept(TcpServer *ts, int fd, char* ip) {
	if( ts == &mClientTcpServer ) {
		Client *client = new Client();
		client->SetClientCallback(this);
		client->SetMessageList(&mIdleMessageList);
		client->fd = fd;
		client->ip = ip;
		client->isOnline = true;

		// 记录在线客户端
		mClientMap.Lock();
		mClientMap.Insert(fd, client);
		mClientMap.Unlock();

		// 统计请求
		mCountMutex.lock();
		mTotal++;
		mCountMutex.unlock();

		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"CamShareMiddleware::OnAccept( "
				"tid : %d, "
				"[内部服务(HTTP), 建立连接], "
				"fd : [%d], "
				"client : %p "
				")",
				(int)syscall(SYS_gettid),
				fd,
				client
				);

	}

	return true;
}

void CamShareMiddleware::OnDisconnect(TcpServer *ts, int fd) {
	if( ts == &mClientTcpServer ) {
//		LogManager::GetLogManager()->Log(
//				LOG_MSG,
//				"CamShareMiddleware::OnDisconnect( "
//				"tid : %d, "
//				"[内部服务(HTTP), 断开连接], "
//				"fd : [%d] "
//				")",
//				(int)syscall(SYS_gettid),
//				fd
//				);

		// 标记下线
		mClientMap.Lock();
		ClientMap::iterator itr = mClientMap.Find(fd);
		if( itr != mClientMap.End() ) {
			Client* client = itr->second;
			if( client != NULL ) {
				client->isOnline = false;

				LogManager::GetLogManager()->Log(
						LOG_MSG,
						"CamShareMiddleware::OnDisconnect( "
						"tid : %d, "
						"[内部服务(HTTP), 断开连接, 找到对应连接], "
						"fd : [%d], "
						"client : %p "
						")",
						(int)syscall(SYS_gettid),
						fd,
						client
						);

			} else {
				LogManager::GetLogManager()->Log(
						LOG_WARNING,
						"CamShareMiddleware::OnDisconnect( "
						"tid : %d, "
						"[内部服务(HTTP), 断开连接, 找不到对应连接, client ==  NULL], "
						"fd : [%d] "
						")",
						(int)syscall(SYS_gettid),
						fd
						);
			}
		} else {
			LogManager::GetLogManager()->Log(
					LOG_WARNING,
					"CamShareMiddleware::OnDisconnect( "
					"tid : %d, "
					"[内部服务(HTTP), 断开连接, 找不到对应连接], "
					"fd : [%d] "
					")",
					(int)syscall(SYS_gettid),
					fd
					);
		}
		mClientMap.Unlock();

//		LogManager::GetLogManager()->Log(
//				LOG_STAT,
//				"CamShareMiddleware::OnDisconnect( "
//				"tid : %d, "
//				"[内部服务(HTTP), 断开连接完成], "
//				"fd : [%d] "
//				")",
//				(int)syscall(SYS_gettid),
//				fd
//				);

	}
}

void CamShareMiddleware::OnClose(TcpServer *ts, int fd) {
	if( ts == &mClientTcpServer ) {
//		LogManager::GetLogManager()->Log(
//				LOG_MSG,
//				"CamShareMiddleware::OnClose( "
//				"tid : %d, "
//				"[内部服务(HTTP), 关闭socket], "
//				"fd : [%d] "
//				")",
//				(int)syscall(SYS_gettid),
//				fd
//				);

		// 释放资源下线
		mClientMap.Lock();
		ClientMap::iterator itr = mClientMap.Find(fd);
		if( itr != mClientMap.End() ) {
			Client* client = itr->second;

			if( client != NULL ) {
				LogManager::GetLogManager()->Log(
						LOG_MSG,
						"CamShareMiddleware::OnClose( "
						"tid : %d, "
						"[内部服务(HTTP), 关闭socket, 找到对应连接], "
						"fd : [%d], "
						"client : %p "
						")",
						(int)syscall(SYS_gettid),
						fd,
						client
						);

				delete client;
			} else {
				LogManager::GetLogManager()->Log(
						LOG_WARNING,
						"CamShareMiddleware::OnClose( "
						"tid : %d, "
						"[内部服务(HTTP), 关闭socket, 找不到对应连接, client ==  NULL], "
						"fd : [%d] "
						")",
						(int)syscall(SYS_gettid),
						fd
						);
			}

			mClientMap.Erase(itr);

		} else {
			LogManager::GetLogManager()->Log(
					LOG_WARNING,
					"CamShareMiddleware::OnClose( "
					"tid : %d, "
					"[内部服务(HTTP), 关闭socket, 找不到对应连接], "
					"fd : [%d] "
					")",
					(int)syscall(SYS_gettid),
					fd
					);
		}
		mClientMap.Unlock();

//		LogManager::GetLogManager()->Log(
//				LOG_STAT,
//				"CamShareMiddleware::OnClose( "
//				"tid : %d, "
//				"[内部服务(HTTP), 关闭socket完成], "
//				"fd : [%d] "
//				")",
//				(int)syscall(SYS_gettid),
//				fd
//				);

	}
}

void CamShareMiddleware::OnRecvMessage(TcpServer *ts, Message *m) {
	if( &mClientTcpServer == ts ) {
		// 内部服务(HTTP), 接收完成处理
		TcpServerRecvMessageHandle(ts, m);

	}
}

void CamShareMiddleware::OnSendMessage(TcpServer *ts, Message *m) {
	if( &mClientTcpServer == ts ) {
		// 内部服务(HTTP)
		mClientMap.Lock();
		ClientMap::iterator itr = mClientMap.Find(m->fd);
		Client* client = itr->second;
		if( client != NULL ) {
			client->AddSentPacket();
			if( client->IsAllPacketSent() ) {
				// 发送完成处理
				ts->Disconnect(m->fd);
			}
		}
		mClientMap.Unlock();
	}
}

void CamShareMiddleware::OnTimeoutMessage(TcpServer *ts, Message *m) {
	if( &mClientTcpServer == ts ) {
		// 内部服务(HTTP), 超时处理
		TcpServerTimeoutMessageHandle(ts, m);

	}

}

int CamShareMiddleware::TcpServerRecvMessageHandle(TcpServer *ts, Message *m) {
	int ret = 0;

	if( m->buffer != NULL && m->len > 0 ) {
		Client *client = NULL;

		/**
		 * 因为还有数据包处理队列中, TcpServer不会回调OnClose, 所以不怕client被释放
		 * 放开锁就可以使多个client并发解数据包, 单个client解包只能同步解包, 在client内部加锁
		 */
		mClientMap.Lock();
		ClientMap::iterator itr = mClientMap.Find(m->fd);
		if( itr != mClientMap.End() ) {
			client = itr->second;
			mClientMap.Unlock();

		} else {
			mClientMap.Unlock();

		}

		if( client != NULL ) {
			LogManager::GetLogManager()->Log(
					LOG_MSG,
					"CamShareMiddleware::TcpServerRecvMessageHandle( "
					"tid : %d, "
					"[内部服务(HTTP), 客户端发起请求], "
					"fd : [%d], "
					"buffer : [\n%s\n]"
					")",
					(int)syscall(SYS_gettid),
					client->fd,
					m->buffer
					);

			ret = client->ParseData(m);
		}

	}

	if( ret == -1 ) {
		ts->Disconnect(m->fd);
	}

	return ret;
}

int CamShareMiddleware::TcpServerTimeoutMessageHandle(TcpServer *ts, Message *m) {
	int ret = -1;

	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"CamShareMiddleware::TcpServerTimeoutMessageHandle( "
			"tid : %d, "
			"[请求超时], "
			"fd : [%d] "
			")",
			(int)syscall(SYS_gettid),
			m->fd
			);

	// 断开连接
	ts->Disconnect(m->fd);

	return ret;
}

void CamShareMiddleware::StateHandle() {
	unsigned int iCount = 0;
	unsigned int iStateTime = miStateTime;

	unsigned int iTotal = 0;
	double iSecondTotal = 0;
	double iResponed = 0;

	while( IsRunning() ) {
		if ( iCount < iStateTime ) {
			iCount++;

		} else {
			iCount = 0;
			iSecondTotal = 0;
			iResponed = 0;

			mCountMutex.lock();
			iTotal = mTotal;

			if( iStateTime != 0 ) {
				iSecondTotal = 1.0 * iTotal / iStateTime;
			}
			if( iTotal != 0 ) {
				iResponed = 1.0 * mResponed / iTotal;
			}

			mTotal = 0;
			mResponed = 0;
			mCountMutex.unlock();

			LogManager::GetLogManager()->Log(
					LOG_WARNING,
					"CamShareMiddleware::StateHandle( "
					"tid : %d, "
					"[内部服务(HTTP)], "
					"过去%u秒共收到%u个请求, "
					"平均收到%.1lf个/秒, "
//					"平均响应时间%.1lf毫秒/个"
					")",
					(int)syscall(SYS_gettid),
					iStateTime,
					iTotal,
					iSecondTotal
//					iResponed
					);

			LogManager::GetLogManager()->Log(
					LOG_WARNING,
					"CamShareMiddleware::StateHandle( "
					"tid : %d, "
					"[内部服务(Freeswitch)], "
					"通话数目 : %u, "
					"在线用户数目 : %u "
					")",
					(int)syscall(SYS_gettid),
					mFreeswitch.GetChannelCount(),
					mFreeswitch.GetOnlineUserCount()
					);

			iStateTime = miStateTime;

		}
		sleep(1);
	}
}

void CamShareMiddleware::ConnectLiveChatHandle() {
	while( IsRunning() ) {
		mLiveChatClientMap.Lock();
		for(LiveChatClientMap::iterator itr = mLiveChatClientMap.Begin(); itr != mLiveChatClientMap.End(); itr++) {
			ILiveChatClient* client = itr->second;
			client->ConnectServer(itr->first, mLivechatName);
		}
		mLiveChatClientMap.Unlock();

		sleep(30);
	}
}

void CamShareMiddleware::ConnectFreeswitchHandle() {
	bool bFlag = false;
	int i = 0;
	while( IsRunning() ) {
		// 先让LiveChat连接服务器
		mLiveChatClientMap.Lock();
		for(LiveChatClientMap::iterator itr = mLiveChatClientMap.Begin(); itr != mLiveChatClientMap.End(); itr++) {
			ILiveChatClient* client = itr->second;
			bFlag = client->IsConnected();

			if( !bFlag ) {
				// 其中一个站点未连上
				break;
			}
		}
		mLiveChatClientMap.Unlock();

		if( bFlag || i >= 30 ) {
			// 开始连接Freeswitch
			if( !mFreeswitch.Proceed(
					mFreeswitchIp,
					miFreeswitchPort,
					mFreeswitchUser,
					mFreeswitchPassword
					) ) {
				LogManager::GetLogManager()->Log(
						LOG_WARNING,
						"CamShareMiddleware::ConnectFreeswitchHandle( "
						"tid : %d, "
						"[内部服务(Freeswitch), 连接Freeswitch失败] "
						")",
						(int)syscall(SYS_gettid)
						);
			}

			// Freeswitch连接断开
			i = 0;
			sleep(10);

		} else {
			// 继续等待
			i++;
			sleep(1);
		}

	}
}

/***************************** 内部服务(HTTP)接口 **************************************/
bool CamShareMiddleware::SendRespond2Client(
		Client* client,
		IRespond* respond
		) {
	bool bFlag = false;

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"CamShareMiddleware::SendRespond2Client( "
			"tid : %d, "
			"[内部服务(HTTP), 返回请求到客户端], "
			"fd : [%d], "
			"respond : %p "
			")",
			(int)syscall(SYS_gettid),
			client->fd,
			respond
			);

	// 发送头部
	Message* m = mClientTcpServer.GetIdleMessageList()->PopFront();
	if( m != NULL ) {
		client->AddSendingPacket();

		m->fd = client->fd;
		snprintf(
				m->buffer,
				MAX_BUFFER_LEN - 1,
				"HTTP/1.1 200 OK\r\n"
				"Connection:Keep-Alive\r\n"
				"Content-Type:text/html; charset=utf-8\r\n"
				"\r\n"
				);
		m->len = strlen(m->buffer);

		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"CamShareMiddleware::SendRespond2Client( "
				"tid : %d, "
				"[内部服务(HTTP), 返回请求头部到客户端], "
				"fd : [%d], "
				"buffer : [\n%s\n]"
				")",
				(int)syscall(SYS_gettid),
				client->fd,
				m->buffer
				);

		mClientTcpServer.SendMessageByQueue(m);
	}

	if( respond != NULL ) {
		// 发送内容
		bool more = false;
		while( true ) {
			Message* m = mClientTcpServer.GetIdleMessageList()->PopFront();
			if( m != NULL ) {
				m->fd = client->fd;
				client->AddSendingPacket();

				m->len = respond->GetData(m->buffer, MAX_BUFFER_LEN, more);
				LogManager::GetLogManager()->Log(
						LOG_MSG,
						"CamShareMiddleware::SendRespond2Client( "
						"tid : %d, "
						"[内部服务(HTTP), 返回请求内容到客户端], "
						"fd : [%d], "
						"buffer : [\n%s\n]"
						")",
						(int)syscall(SYS_gettid),
						client->fd,
						m->buffer
						);

				mClientTcpServer.SendMessageByQueue(m);
				if( !more ) {
					// 全部发送完成
					bFlag = true;
					break;
				}

			} else {
				break;
			}
		}

		delete respond;
	}

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"CamShareMiddleware::SendRespond2Client( "
			"tid : %d, "
			"[内部服务(HTTP), 返回请求到客户端], "
			"fd : [%d], "
			"bFlag : %s "
			")",
			(int)syscall(SYS_gettid),
			client->fd,
			bFlag?"true":"false"
			);

	return bFlag;
}
/***************************** 内部服务(HTTP)接口 end **************************************/

/***************************** 内部服务(HTTP) 回调处理 **************************************/
void CamShareMiddleware::OnClientGetDialplan(
		Client* client,
		const string& rtmpSession,
		const string& channelId,
		const string& conference,
		const string& serverId,
		const string& siteId
		) {
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"CamShareMiddleware::OnClientGetDialplan( "
			"tid : %d, "
			"[内部服务(HTTP), 收到命令:获取拨号计划], "
			"fd : [%d], "
			"rtmpSession : %s, "
			"channelId : %s, "
			"conference : %s, "
			"serverId : %s, "
			"siteId : %s "
			")",
			(int)syscall(SYS_gettid),
			client->fd,
			rtmpSession.c_str(),
			channelId.c_str(),
			conference.c_str(),
			serverId.c_str(),
			siteId.c_str()
			);

	// 获根据rtmp session获取用户名
	string caller = mFreeswitch.GetUserBySession(rtmpSession);

	// 插入channel
    Channel channel(caller, conference, serverId, siteId);
    mFreeswitch.CreateChannel(channelId, channel, true);

	// 马上返回数据
	DialplanRespond* respond = new DialplanRespond();
	respond->SetParam(caller);
	SendRespond2Client(client, respond);

}

void CamShareMiddleware::OnClientUndefinedCommand(Client* client) {
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"CamShareMiddleware::OnClientUndefinedCommand( "
			"tid : %d, "
			"[内部服务(HTTP), 收到命令:未知命令], "
			"fd : [%d] "
			")",
			(int)syscall(SYS_gettid),
			client->fd
			);

	mClientTcpServer.Disconnect(client->fd);
}
/***************************** 内部服务(HTTP) 回调处理 end **************************************/

/***************************** 内部服务(Freeswitch) 回调处理 **************************************/
void CamShareMiddleware::OnFreeswitchEventConferenceAuthorizationMember(
		FreeswitchClient* freeswitch,
		const Channel* channel
		) {
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"CamShareMiddleware::OnFreeswitchEventConferenceAuthorizationMember( "
			"tid : %d, "
			"[内部服务(Freeswitch), 收到命令:重新验证会议成员], "
			"user : %s, "
			"conference : %s, "
			"type : %d, "
			"memberId : %s, "
			"serverId : %s, "
			"siteId : %s "
			")",
			(int)syscall(SYS_gettid),
			channel->user.c_str(),
			channel->conference.c_str(),
			channel->type,
			channel->memberId.c_str(),
			channel->serverId.c_str(),
			channel->siteId.c_str()
			);

	// 踢出相同账户已经进入的其他连接
	mFreeswitch.KickUserFromConference(channel->user, channel->conference, channel->memberId);
	if( channel->user != channel->conference && !CheckTestAccount(channel->user) ) {
		// 进入别人的会议室
		string serverId = channel->serverId;
		string siteId = channel->siteId;
		int siteType = atoi(siteId.c_str());

		// 找出需要发送的LiveChat Client
		ILiveChatClient* livechat = NULL;
		mLiveChatClientMap.Lock();
		LiveChatClientMap::iterator itr = mLiveChatClientMap.Find((SITE_TYPE)siteType);
		if( itr != mLiveChatClientMap.End() ) {
			livechat = itr->second;
		}
		mLiveChatClientMap.Unlock();

		// 发送进入聊天室认证命令
		if( !SendEnterConference2LiveChat(livechat, channel->user, channel->conference, channel->type, channel->serverId) ) {
			// 断开指定用户视频
			freeswitch->KickUserFromConference(channel->user, channel->conference, "");
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
			"tid : %d, "
			"[内部服务(Freeswitch), 收到命令:增加会议成员], "
			"user : %s, "
			"conference : %s, "
			"type : %d, "
			"memberId : %s, "
			"serverId : %s, "
			"siteId : %s "
			")",
			(int)syscall(SYS_gettid),
			channel->user.c_str(),
			channel->conference.c_str(),
			channel->type,
			channel->memberId.c_str(),
			channel->serverId.c_str(),
			channel->siteId.c_str()
			);

	// 踢出相同账户已经进入的其他连接
	mFreeswitch.KickUserFromConference(channel->user, channel->conference, channel->memberId);

	if( channel->user != channel->conference && !CheckTestAccount(channel->user) ) {
		// 进入别人聊天室
//		// 开放成员视频
//		mFreeswitch.StartUserRecvVideo(user, conference, type);

		// 进入别人的会议室
		string serverId = channel->serverId;
		string siteId = channel->siteId;
		int siteType = atoi(siteId.c_str());

		// 找出需要发送的LiveChat Client
		ILiveChatClient* livechat = NULL;
		mLiveChatClientMap.Lock();
		LiveChatClientMap::iterator itr = mLiveChatClientMap.Find((SITE_TYPE)siteType);
		if( itr != mLiveChatClientMap.End() ) {
			livechat = itr->second;
		}
		mLiveChatClientMap.Unlock();

		// 发送进入聊天室认证命令
		if( !SendEnterConference2LiveChat(livechat, channel->user, channel->conference, channel->type, channel->serverId) ) {
			// 断开指定用户视频
			freeswitch->KickUserFromConference(channel->user, channel->conference, "");
		}

		// 发送通知客户端进入聊天室命令
		SendMsgEnterConference2LiveChat(livechat, channel->user, channel->conference);

	} else {
		// 进入自己会议室, 直接通过
		// 开放成员视频
		mFreeswitch.StartUserRecvVideo(channel->user, channel->conference, channel->type);
	}

}

void CamShareMiddleware::OnFreeswitchEventConferenceDelMember(
		FreeswitchClient* freeswitch,
		const Channel* channel
		) {
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"CamShareMiddleware::OnFreeswitchEventConferenceDelMember( "
			"tid : %d, "
			"[内部服务(Freeswitch), 收到命令:退出会议成员], "
			"user : %s, "
			"conference : %s, "
			"type : %d, "
			"memberId : %s, "
			"serverId : %s, "
			"siteId : %s "
			")",
			(int)syscall(SYS_gettid),
			channel->user.c_str(),
			channel->conference.c_str(),
			channel->type,
			channel->memberId.c_str(),
			channel->serverId.c_str(),
			channel->siteId.c_str()
			);

	if( channel->user != channel->conference ) {
		// 进入别人聊天室
		string siteId = channel->siteId;
		int siteType = atoi(siteId.c_str());

		// 通知客户端
		ILiveChatClient* livechat = NULL;
		mLiveChatClientMap.Lock();
		LiveChatClientMap::iterator itr = mLiveChatClientMap.Find((SITE_TYPE)siteType);
		if( itr != mLiveChatClientMap.End() ) {
			livechat = itr->second;
		}
		mLiveChatClientMap.Unlock();

		// 发送通知客户端退出聊天室命令
		SendMsgExitConference2LiveChat(livechat, channel->user, channel->conference);

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
				LOG_MSG,
				"CamShareMiddleware::OnConnect( "
				"tid : %d, "
				"[外部服务(LiveChat), 连接服务器成功], "
				"livechat : %p, "
				"siteId : %d "
				")",
				(int)syscall(SYS_gettid),
				livechat,
				livechat->GetType()
				);

		if( mAuthorization ) {
			// 重新验证所有用户
			mFreeswitch.AuthorizationAllConference();
		}

	} else {
		LogManager::GetLogManager()->Log(
				LOG_WARNING,
				"CamShareMiddleware::OnConnect( "
				"tid : %d, "
				"[外部服务(LiveChat), 连接服务器失败], "
				"livechat : %p, "
				"siteId : %d, "
				"err : %d, "
				"errmsg : %s "
				")",
				(int)syscall(SYS_gettid),
				livechat,
				livechat->GetType(),
				err,
				errmsg.c_str()
				);
	}

}

void CamShareMiddleware::OnDisconnect(
		ILiveChatClient* livechat,
		LCC_ERR_TYPE err,
		const string& errmsg
		) {
	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"CamShareMiddleware::OnDisconnect( "
			"tid : %d, "
			"[外部服务(LiveChat), 断开服务器], "
			"livechat : %p, "
			"siteId : %d "
			"err : %d, "
			"errmsg : %s "
			")",
			(int)syscall(SYS_gettid),
			livechat,
			livechat->GetType(),
			err,
			errmsg.c_str()
			);

	mSessionManager.CloseSessionByLiveChat(livechat);

}

void CamShareMiddleware::OnSendEnterConference(ILiveChatClient* livechat, int seq, const string& fromId, const string& toId, LCC_ERR_TYPE err, const string& errmsg) {
	if( err == LCC_ERR_SUCCESS ) {
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"CamShareMiddleware::OnSendEnterConference( "
				"tid : %d, "
				"[外部服务(LiveChat), 发送命令:进入会议室认证, 成功], "
				"livechat : %p, "
				"siteId : %d, "
				"seq : %d, "
				"fromId : %s, "
				"toId : %s "
				")",
				(int)syscall(SYS_gettid),
				livechat,
				livechat->GetType(),
				seq,
				fromId.c_str(),
				toId.c_str()
				);

	} else {
		LogManager::GetLogManager()->Log(
				LOG_WARNING,
				"CamShareMiddleware::OnSendEnterConference( "
				"tid : %d, "
				"[外部服务(LiveChat), 发送命令:进入会议室认证, 失败], "
				"livechat : %p, "
				"siteId : %d, "
				"seq : %d, "
				"fromId : %s, "
				"toId : %s "
				"err : %d, "
				"errmsg : %s "
				")",
				(int)syscall(SYS_gettid),
				livechat,
				livechat->GetType(),
				seq,
				fromId.c_str(),
				toId.c_str(),
				err,
				errmsg.c_str()
				);

		// 结束会话
		IRequest* request = mSessionManager.FinishSessionByCustomIdentify(livechat, fromId + toId);
		if( request != NULL ) {
			delete request;
			request = NULL;
		}

		// 断开用户
		mFreeswitch.KickUserFromConference(fromId, toId, "");
	}

}

void CamShareMiddleware::OnRecvEnterConference(
		ILiveChatClient* livechat,
		int seq,
		const string& fromId,
		const string& toId,
		bool bAuth,
		LCC_ERR_TYPE err,
		const string& errmsg
		) {

	// 结束会话任务
	IRequest* request = mSessionManager.FinishSessionByCustomIdentify(livechat, fromId + toId);

	if( err == LCC_ERR_SUCCESS && bAuth && request ) {
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"CamShareMiddleware::OnRecvEnterConference( "
				"tid : %d, "
				"[外部服务(LiveChat), 收到命令:进入会议室认证结果, 成功], "
				"livechat : %p, "
				"siteId : %d, "
				"seq : %d, "
				"fromId : %s, "
				"toId : %s "
				")",
				(int)syscall(SYS_gettid),
				livechat,
				livechat->GetType(),
				seq,
				fromId.c_str(),
				toId.c_str()
				);

		// 开放成员视频
		mFreeswitch.StartUserRecvVideo(fromId, toId, ((EnterConferenceRequest*)request)->GetMemberType());

	} else {
		LogManager::GetLogManager()->Log(
				LOG_WARNING,
				"CamShareMiddleware::OnRecvEnterConference( "
				"tid : %d, "
				"[外部服务(LiveChat), 收到命令:进入会议室认证结果, 失败], "
				"livechat : %p, "
				"siteId : %d, "
				"seq : %d, "
				"fromId : %s, "
				"toId : %s, "
				"bAuth : %s, "
				"err : %d, "
				"errmsg : %s "
				")",
				(int)syscall(SYS_gettid),
				livechat,
				livechat->GetType(),
				seq,
				fromId.c_str(),
				toId.c_str(),
				bAuth?"true":"false",
				err,
				errmsg.c_str()
				);

		// 断开用户
		mFreeswitch.KickUserFromConference(fromId, toId, "");
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
				LOG_MSG,
				"CamShareMiddleware::OnRecvKickUserFromConference( "
				"tid : %d, "
				"[外部服务(LiveChat), 收到命令:从会议室踢出用户, 成功], "
				"livechat : %p, "
				"siteId : %d, "
				"seq : %d, "
				"fromId : %s, "
				"toId : %s "
				")",
				(int)syscall(SYS_gettid),
				livechat,
				livechat->GetType(),
				seq,
				fromId.c_str(),
				toId.c_str()
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

	} else {
		LogManager::GetLogManager()->Log(
				LOG_WARNING,
				"CamShareMiddleware::OnRecvKickUserFromConference( "
				"tid : %d, "
				"[外部服务(LiveChat), 收到命令:从会议室踢出用户, 失败], "
				"livechat : %p, "
				"siteId : %d, "
				"seq : %d, "
				"fromId : %s, "
				"toId : %s, "
				"err : %d, "
				"errmsg : %s "
				")",
				(int)syscall(SYS_gettid),
				livechat,
				livechat->GetType(),
				seq,
				fromId.c_str(),
				toId.c_str(),
				err,
				errmsg.c_str()
				);
	}
}
/***************************** 外部服务(LiveChat) 回调处理 end **************************************/

/***************************** 外部服务接口 **************************************/
bool CamShareMiddleware::SendEnterConference2LiveChat(
		ILiveChatClient* livechat,
		const string& fromId,
		const string& toId,
		MemberType type,
		const string& serverId
		) {
	bool bFlag = true;

	SITE_TYPE siteId = SITE_TYPE_UNKNOW;
	// LiveChat client生成请求序列号
	int seq = -1;
	if( livechat != NULL ) {
		seq = livechat->GetSeq();
		siteId = livechat->GetType();

	} else {
		bFlag = false;
	}

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"CamShareMiddleware::SendEnterConference2LiveChat( "
			"tid : %d, "
			"[外部服务(LiveChat), 发送命令:进入会议室], "
			"siteId : %d, "
			"fromId : %s, "
			"toId : %s, "
			"type : %u, "
			"serverId : %s "
			")",
			(int)syscall(SYS_gettid),
			siteId,
			fromId.c_str(),
			toId.c_str(),
			type,
			serverId.c_str()
			);

	if( bFlag ) {
		bFlag = false;

		// 处理是否需要记录请求
		EnterConferenceRequest* request = new EnterConferenceRequest();
		char key[32] = {'\0'};
		sprintf(key, "%u", miSendEnterConference++);
		request->SetParam(&mFreeswitch, livechat, seq, serverId, fromId, toId, type, key);

		// 生成会话
		if( mSessionManager.StartSessionByCustomIdentify(livechat, request, fromId + toId) ) {
			// 发送请求成功
			LogManager::GetLogManager()->Log(
					LOG_MSG,
					"CamShareMiddleware::SendEnterConference2LiveChat( "
					"tid : %d, "
					"[外部服务(LiveChat), 发送命令:进入会议室, 生成会话成功], "
					"siteId : %d, "
					"fromId : %s, "
					"toId : %s, "
					"type : %u, "
					"serverId : %s, "
					"key : %s "
					")",
					(int)syscall(SYS_gettid),
					siteId,
					fromId.c_str(),
					toId.c_str(),
					type,
					serverId.c_str(),
					key
					);
			bFlag = true;

		}
	}

	if( !bFlag ) {
		LogManager::GetLogManager()->Log(
				LOG_WARNING,
				"CamShareMiddleware::SendEnterConference2LiveChat( "
				"tid : %d, "
				"[外部服务(LiveChat), 发送命令:进入会议室, 失败], "
				"siteId : %d, "
				"fromId : %s, "
				"toId : %s, "
				"type : %u, "
				"serverId : %s "
				")",
				(int)syscall(SYS_gettid),
				siteId,
				fromId.c_str(),
				toId.c_str(),
				type,
				serverId.c_str()
				);
	}

	return bFlag;
}

bool CamShareMiddleware::SendMsgEnterConference2LiveChat(
		ILiveChatClient* livechat,
		const string& fromId,
		const string& toId
		) {
	bool bFlag = true;

	SITE_TYPE siteId = SITE_TYPE_UNKNOW;
	// LiveChat client生成请求序列号
	int seq = -1;
	if( livechat != NULL ) {
		seq = livechat->GetSeq();
		siteId = livechat->GetType();
	} else {
		bFlag = false;
	}

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"CamShareMiddleware::SendMsgEnterConference2LiveChat( "
			"tid : %d, "
			"[外部服务(LiveChat), 发送命令:通知客户端进入聊天室], "
			"siteId : %d, "
			"fromId : %s, "
			"toId : %s "
			")",
			(int)syscall(SYS_gettid),
			siteId,
			fromId.c_str(),
			toId.c_str()
			);

	if( bFlag ) {
		bFlag = false;

		SendMsgEnterConferenceRequest request;
		request.SetParam(&mFreeswitch, livechat, seq, fromId, toId);
		bFlag = request.StartRequest();

		if( bFlag ) {
			// 发送请求成功
			LogManager::GetLogManager()->Log(
					LOG_MSG,
					"CamShareMiddleware::SendMsgEnterConference2LiveChat( "
					"tid : %d, "
					"[外部服务(LiveChat), 发送命令:通知客户端进入聊天室, 成功], "
					"siteId : %d, "
					"fromId : %s, "
					"toId : %s "
					")",
					(int)syscall(SYS_gettid),
					siteId,
					fromId.c_str(),
					toId.c_str()
					);
		}
	}

	if( !bFlag ) {
		LogManager::GetLogManager()->Log(
				LOG_WARNING,
				"CamShareMiddleware::SendMsgEnterConference2LiveChat( "
				"tid : %d, "
				"[外部服务(LiveChat), 发送命令:通知客户端进入聊天室, 失败], "
				"siteId : %d, "
				"fromId : %s, "
				"toId : %s "
				")",
				(int)syscall(SYS_gettid),
				siteId,
				fromId.c_str(),
				toId.c_str()
				);
	}

	return bFlag;
}

bool CamShareMiddleware::SendMsgExitConference2LiveChat(
		ILiveChatClient* livechat,
		const string& fromId,
		const string& toId
		) {
	bool bFlag = true;

	SITE_TYPE siteId = SITE_TYPE_UNKNOW;
	// LiveChat client生成请求序列号
	int seq = -1;
	if( livechat != NULL ) {
		seq = livechat->GetSeq();
		siteId = livechat->GetType();
	} else {
		bFlag = false;
	}
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"CamShareMiddleware::SendMsgExitConference2LiveChat( "
			"tid : %d, "
			"[外部服务(LiveChat), 发送命令:通知客户端退出聊天室], "
			"siteId : %d, "
			"fromId : %s, "
			"toId : %s "
			")",
			(int)syscall(SYS_gettid),
			siteId,
			fromId.c_str(),
			toId.c_str()
			);

	if( bFlag ) {
		bFlag = false;

		SendMsgExitConferenceRequest request;
		request.SetParam(&mFreeswitch, livechat, seq, fromId, toId);
		bFlag = request.StartRequest();

		if( bFlag ) {
			// 发送请求成功
			LogManager::GetLogManager()->Log(
					LOG_MSG,
					"CamShareMiddleware::SendMsgExitConference2LiveChat( "
					"tid : %d, "
					"[外部服务(LiveChat), 发送命令:通知客户端退出聊天室, 成功], "
					"siteId : %d, "
					"fromId : %s, "
					"toId : %s "
					")",
					(int)syscall(SYS_gettid),
					siteId,
					fromId.c_str(),
					toId.c_str()
					);
		}
	}

	if( !bFlag ) {
		LogManager::GetLogManager()->Log(
				LOG_WARNING,
				"CamShareMiddleware::SendMsgExitConference2LiveChat( "
				"tid : %d, "
				"[外部服务(LiveChat), 发送命令:通知客户端退出聊天室, 失败], "
				"siteId : %d, "
				"fromId : %s, "
				"toId : %s "
				")",
				(int)syscall(SYS_gettid),
				siteId,
				fromId.c_str(),
				toId.c_str()
				);
	}

	return bFlag;
}
/***************************** 外部服务接口 end **************************************/

/***************************** 会话管理器 回调处理 **************************************/
bool CamShareMiddleware::CheckTestAccount(
		const string& user
		) {
	bool bFlag = false;
	if( user.find("WW") == 0 || user.find("MM") == 0 ) {
		LogManager::GetLogManager()->Log(
				LOG_STAT,
				"CamShareMiddleware::CheckTestAccount( "
				"tid : %d, "
				"[检测到测试账号], "
				"user : %s "
				")",
				(int)syscall(SYS_gettid),
				user.c_str()
				);
		bFlag = true;
	}
	return bFlag;
}
/***************************** 会话管理器 回调处理 end **************************************/

