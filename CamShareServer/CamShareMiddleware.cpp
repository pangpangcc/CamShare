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

	// 日志参数
	miStateTime = 0;
	miDebugMode = 0;
	miLogLevel = 0;

	// 统计参数
	mResponed = 0;
	mTotal = 0;

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

void CamShareMiddleware::Run(const string& config) {
	if( config.length() > 0 ) {
		mConfigFile = config;

		// Reload config
		if( Reload() ) {
			if( miDebugMode == 1 ) {
				LogManager::LogSetFlushBuffer(0);
			} else {
				LogManager::LogSetFlushBuffer(5 * BUFFER_SIZE_1K * BUFFER_SIZE_1K);
			}

			Run();
		} else {
			printf("# CamShareMiddleware can not load config file exit. \n");
		}

	} else {
		printf("# No config file can be use exit. \n");
	}
}

void CamShareMiddleware::Run() {
	/* log system */
	LogManager::GetLogManager()->Start(1000, miLogLevel, mLogDir);
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"CamShareMiddleware::Run( "
			"miPort : %d, "
			"miMaxClient : %d, "
			"miMaxHandleThread : %d, "
			"miMaxQueryPerThread : %d, "
			"miTimeout : %d, "
			"miStateTime, %d, "
			"miLogLevel : %d, "
			"mlogDir : %s "
			")",
			miPort,
			miMaxClient,
			miMaxHandleThread,
			miMaxQueryPerThread,
			miTimeout,
			miStateTime,
			miLogLevel,
			mLogDir.c_str()
			);

	/**
	 * 客户端解析包缓存buffer
	 */
	for(int i = 0; i < miMaxClient; i++) {
		/* create idle buffers */
		Message *m = new Message();
		mIdleMessageList.PushBack(m);
	}

	/* HTTP client server */
	/**
	 * 预估相应时间, 内存数目*超时间隔*每秒处理的任务
	 */
	mClientTcpServer.SetTcpServerObserver(this);
	mClientTcpServer.SetHandleSize(miTimeout * miMaxQueryPerThread);
	mClientTcpServer.Start(miMaxClient, miPort, miMaxHandleThread);
	LogManager::GetLogManager()->Log(LOG_STAT, "CamShareMiddleware::Run( TcpServer Init OK )");

	/* LiveChat client */
	list<string> ips;
	ips.push_back("127.0.0.1");
	ILiveChatClient* cl = ILiveChatClient::CreateClient();
	cl->Init(ips, 9877, this);
	mLiveChatClientMap.Lock();
	mLiveChatClientMap.Insert(SITE_TYPE_CL, cl);
	mLiveChatClientMap.Unlock();

	mIsRunning = true;

	// 开始Livechat连接线程
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
	printf("# CamShareMiddleware start OK \n");
	LogManager::GetLogManager()->Log(LOG_WARNING, "CamShareMiddleware::Run( Init OK )");

	while( true ) {
		/* do nothing here */
		sleep(5);
	}
}

bool CamShareMiddleware::Reload() {
	bool bFlag = false;
	mConfigMutex.lock();
	if( mConfigFile.length() > 0 ) {
		ConfFile conf;
		conf.InitConfFile(mConfigFile.c_str(), "");
		if ( conf.LoadConfFile() ) {
			// BASE
			miPort = atoi(conf.GetPrivate("BASE", "PORT", "9876").c_str());
			miMaxClient = atoi(conf.GetPrivate("BASE", "MAXCLIENT", "100000").c_str());
			miMaxHandleThread = atoi(conf.GetPrivate("BASE", "MAXHANDLETHREAD", "2").c_str());
			miMaxQueryPerThread = atoi(conf.GetPrivate("BASE", "MAXQUERYPERCOPY", "10").c_str());
			miTimeout = atoi(conf.GetPrivate("BASE", "TIMEOUT", "10").c_str());
			miStateTime = atoi(conf.GetPrivate("BASE", "STATETIME", "30").c_str());

			// LOG
			miLogLevel = atoi(conf.GetPrivate("LOG", "LOGLEVEL", "5").c_str());
			mLogDir = conf.GetPrivate("LOG", "LOGDIR", "log");
			miDebugMode = atoi(conf.GetPrivate("LOG", "DEBUGMODE", "0").c_str());

			mClientTcpServer.SetHandleSize(miTimeout * miMaxQueryPerThread);

			LogManager::GetLogManager()->Log(
					LOG_MSG,
					"CamShareMiddleware::Reload( "
					"miPort : %d, "
					"miMaxClient : %d, "
					"miMaxHandleThread : %d, "
					"miMaxQueryPerThread : %d, "
					"miTimeout : %d, "
					"miStateTime, %d, "
					"miLogLevel : %d, "
					"mlogDir : %s "
					")",
					miPort,
					miMaxClient,
					miMaxHandleThread,
					miMaxQueryPerThread,
					miTimeout,
					miStateTime,
					miLogLevel,
					mLogDir.c_str()
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
				"fd : [%d], "
				"[内部服务(HTTP), 建立连接] "
				")",
				(int)syscall(SYS_gettid),
				fd
				);

	}

	return true;
}

void CamShareMiddleware::OnDisconnect(TcpServer *ts, int fd) {
	if( ts == &mClientTcpServer ) {
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"CamShareMiddleware::OnDisconnect( "
				"tid : %d, "
				"[内部服务(HTTP), 断开连接], "
				"fd : [%d] "
				")",
				(int)syscall(SYS_gettid),
				fd
				);

		// 标记下线
		mClientMap.Lock();
		ClientMap::iterator itr = mClientMap.Find(fd);
		Client* client = itr->second;
		if( client != NULL ) {
			client->isOnline = false;
		}
		mClientMap.Unlock();

		LogManager::GetLogManager()->Log(
				LOG_STAT,
				"CamShareMiddleware::OnDisconnect( "
				"tid : %d, "
				"[内部服务(HTTP), 断开连接完成], "
				"fd : [%d] "
				")",
				(int)syscall(SYS_gettid),
				fd
				);

	}
}

void CamShareMiddleware::OnClose(TcpServer *ts, int fd) {
	if( ts == &mClientTcpServer ) {
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"CamShareMiddleware::OnClose( "
				"tid : %d, "
				"[内部服务(HTTP), 关闭socket], "
				"fd : [%d] "
				")",
				(int)syscall(SYS_gettid),
				fd
				);

		// 释放资源下线
		mClientMap.Lock();
		ClientMap::iterator itr = mClientMap.Find(fd);
		if( itr != mClientMap.End() ) {
			Client* client = itr->second;
			CloseSessionByClient(client);
			delete client;

			mClientMap.Erase(itr);
		}
		mClientMap.Unlock();

		LogManager::GetLogManager()->Log(
				LOG_STAT,
				"CamShareMiddleware::OnClose( "
				"tid : %d, "
				"[内部服务(HTTP), 关闭socket完成], "
				"fd : [%d] "
				")",
				(int)syscall(SYS_gettid),
				fd
				);

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

//void CamShareMiddleware::FreeswitchEventHandle(esl_event_t *event) {
//	LogManager::GetLogManager()->Log(
//			LOG_MSG,
//			"CamShareMiddleware::FreeswitchEventHandle( "
//			"tid : %d, "
//			"[内部服务(Freeswitch), 事件处理], "
//			"body : %s "
//			")",
//			(int)syscall(SYS_gettid),
//			event->body
//			);
//
//	Json::Value root;
//	Json::Reader reader;
//	if( reader.parse(event->body, root, false) ) {
//		if( root.isObject() ) {
//			if( root["Event-Calling-Function"].isString() ) {
//				string function = root["Event-Calling-Function"].asString();
//				LogManager::GetLogManager()->Log(
//						LOG_MSG,
//						"CamShareMiddleware::FreeswitchEventHandle( "
//						"tid : %d, "
//						"[内部服务(Freeswitch), 事件处理], "
//						"function : %s "
//						")",
//						(int)syscall(SYS_gettid),
//						function.c_str()
//						);
//
//				if( function == "conference_member_add" ) {
//					// 增加会议成员
//					FreeswitchAddConferenceMember(root);
//
//				} else if( function == "rtmp_session_login" ) {
//					// rtmp登陆成功
//					FreeswitchRtmpLogin(root);
//
//				} else if( function == "rtmp_session_login" ) {
//					// rtmp登陆成功
//					FreeswitchRtmpLogin(root);
//
//				}
//			}
//		}
//	}
//
//}

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
					"过去%u秒共收到%u个请求, "
					"平均收到%.1lf个/秒, "
					"平均响应时间%.1lf毫秒/个"
					")",
					(int)syscall(SYS_gettid),
					iStateTime,
					iTotal,
					iSecondTotal,
					iResponed
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
			client->ConnectServer(itr->first);
		}
		mLiveChatClientMap.Unlock();

		sleep(10);
	}
}

void CamShareMiddleware::ConnectFreeswitchHandle() {
	while( IsRunning() ) {
		if( !mFreeswitch.Proceed() ) {
			sleep(10);
		}
	}
}

bool CamShareMiddleware::StartSession(
		Client* client,
		ILiveChatClient* livechat,
		IRequest* request,
		int seq
		) {
	bool bFlag = false;

	int fd = -1;
	if( client != NULL ) {
		fd = client->fd;
	}

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"CamShareMiddleware::StartSession( "
			"tid : %d, "
			"[会话交互(Session), 创建客户端到LiveChat会话], "
			"fd : [%d], "
			"livechat : %p "
			")",
			(int)syscall(SYS_gettid),
			fd,
			livechat
			);

	// 是否需要等待返回的请求
	bFlag = request->IsNeedReturn();
	if( bFlag ) {
		mClient2SessionMap.Lock();
		Session* session = NULL;
		LiveChat2SessionMap::iterator itr = mLiveChat2SessionMap.Find(livechat);
		if( itr != mLiveChat2SessionMap.End() ) {
			session = itr->second;
			LogManager::GetLogManager()->Log(
					LOG_MSG,
					"CamShareMiddleware::StartSession( "
					"tid : %d, "
					"[会话交互(Session), Client -> LiveChat, 继续会话], "
					"fd : [%d], "
					"livechat : %p, "
					"session : %p, "
					"request : %p, "
					"seq : %d "
					")",
					(int)syscall(SYS_gettid),
					fd,
					livechat,
					session,
					request,
					seq
					);

		} else {
			session = new Session(client, livechat);
			if( client != NULL ) {
				mClient2SessionMap.Insert(client, session);
			}
			mLiveChat2SessionMap.Insert(livechat, session);

			LogManager::GetLogManager()->Log(
					LOG_MSG,
					"CamShareMiddleware::StartSession( "
					"tid : %d, "
					"[会话交互(Session), Client -> LiveChat, 开始新会话] "
					"fd : [%d] "
					"livechat : %p, "
					"session : %p, "
					"request : %p, "
					"seq : %d "
					")",
					(int)syscall(SYS_gettid),
					fd,
					livechat,
					session,
					request,
					seq
					);
		}

		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"CamShareMiddleware::StartSession( "
				"tid : %d, "
				"[会话交互(Session), Client -> LiveChat, 插入任务到会话], "
				"fd : [%d], "
				"livechat : %p, "
				"session : %p, "
				"request : %p, "
				"seq : %d "
				")",
				(int)syscall(SYS_gettid),
				fd,
				livechat,
				session,
				request,
				seq
				);

		// 增加到等待返回的列表
		session->AddRequest(seq, request);
		bFlag = true;

		mClient2SessionMap.Unlock();
	}

	// 不需要等待返回的请求, 释放
	if( !bFlag ) {
		delete request;
	}

	return bFlag;
}

IRequest* CamShareMiddleware::FinishSession(
		ILiveChatClient* livechat,
		int seq,
		Client** client
		) {
	IRequest* request = NULL;

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"CamShareMiddleware::FinishSession( "
			"tid : %d, "
			"[会话交互(Session), LiveChat -> Client, 返回响应到客户端], "
			"livechat : %p, "
			"seq : %d "
			")",
			(int)syscall(SYS_gettid),
			livechat
			);

	Session* session = NULL;
	mClient2SessionMap.Lock();
	LiveChat2SessionMap::iterator itr = mLiveChat2SessionMap.Find(livechat);
	if( itr != mLiveChat2SessionMap.End() ) {
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"CamShareMiddleware::FinishSession( "
				"tid : %d, "
				"[会话交互(Session), LiveChat -> Client, 返回响应到客户端, 找到对应会话], "
				"livechat : %p, "
				"seq : %d "
				")",
				(int)syscall(SYS_gettid),
				livechat,
				seq
				);

		session = itr->second;
		if( session != NULL ) {
			if( client != NULL ) {
				*client = session->client;
			}
			request = session->EraseRequest(seq);
			if( request != NULL ) {
				LogManager::GetLogManager()->Log(
						LOG_MSG,
						"CamShareMiddleware::FinishSession( "
						"tid : %d, "
						"[会话交互(Session), LiveChat -> Client, 返回响应到客户端, 找到对应请求], "
						"livechat : %p, "
						"seq : %d, "
						"request : %p "
						")",
						(int)syscall(SYS_gettid),
						livechat,
						seq,
						request
						);
			} else {
				LogManager::GetLogManager()->Log(
						LOG_MSG,
						"CamShareMiddleware::FinishSession( "
						"tid : %d, "
						"[会话交互(Session), LiveChat -> Client, 返回响应到客户端, 找不到对应请求], "
						"livechat : %p, "
						"seq : %d, "
						"request : %p "
						")",
						(int)syscall(SYS_gettid),
						livechat,
						seq,
						request
						);
			}

			// 根据逻辑判断是否释放会话, 暂时不释放, 等待客户端断开连接时候释放

		}

	}
	mClient2SessionMap.Unlock();

	return request;
}

bool CamShareMiddleware::CloseSessionByLiveChat(ILiveChatClient* livechat) {
	bool bFlag = true;

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"CamShareMiddleware::CloseSessionByLiveChat( "
			"tid : %d, "
			"[外部服务(LiveChat), 关闭所有会话], "
			"livechat : %p "
			")",
			(int)syscall(SYS_gettid),
			livechat
			);

	// 断开该站所有客户端连接
	mClient2SessionMap.Lock();
	for(LiveChat2SessionMap::iterator itr = mLiveChat2SessionMap.Begin(); itr != mLiveChat2SessionMap.End();) {
		Session* session = itr->second;

		if( session != NULL ) {
			Client* client = session->client;

			if( session->livechat == livechat ) {
				itr = mLiveChat2SessionMap.Erase(itr);

				if( client != NULL ) {
					mClient2SessionMap.Erase(client);
					mClientTcpServer.Disconnect(client->fd);
				}

			} else {
				itr++;
			}

			delete session;
		}

	}
	mClient2SessionMap.Unlock();

	return bFlag;
}

bool CamShareMiddleware::CloseSessionByClient(Client* client) {
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"CamShareMiddleware::CloseSessionByClient( "
			"tid : %d, "
			"[内部服务(HTTP), 关闭会话], "
			"fd : [%d] "
			")",
			(int)syscall(SYS_gettid),
			client->fd
			);
	bool bFlag = false;

	mClient2SessionMap.Lock();
	Client2SessionMap::iterator itr = mClient2SessionMap.Find(client);
	if( itr != mClient2SessionMap.End() ) {
		Session* session = itr->second;
		if( session != NULL ) {
			mLiveChat2SessionMap.Erase(session->livechat);

			delete session;
			session = NULL;
		}

		mClient2SessionMap.Erase(itr);
		bFlag = true;
	}
	mClient2SessionMap.Unlock();

	return bFlag;
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
void CamShareMiddleware::OnClientGetUserBySession(Client* client, const string& session) {
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"CamShareMiddleware::OnClientGetUserBySession( "
			"tid : %d, "
			"[内部服务(HTTP), 收到命令:获取用户名], "
			"fd : [%d], "
			"session : %s "
			")",
			(int)syscall(SYS_gettid),
			client->fd,
			session.c_str()
			);

	// 获根据rtmp session获取用户名
	string caller = mFreeswitch.GetUserBySession(session);

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
void CamShareMiddleware::OnFreeswitchEventConferenceAddMember(
		FreeswitchClient* freeswitch,
		const string& user,
		const string& conference,
		MemberType type
		) {
	if( !SendEnterConference2LiveChat(user, conference, type) ) {
		// 断开指定用户视频
		freeswitch->KickUserFromConference(user, conference);
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
				"livechat : %p "
				")",
				(int)syscall(SYS_gettid),
				livechat
				);
	} else {
		LogManager::GetLogManager()->Log(
				LOG_WARNING,
				"CamShareMiddleware::OnConnect( "
				"tid : %d, "
				"[外部服务(LiveChat), 连接服务器失败], "
				"livechat : %p, "
				"err : %d, "
				"errmsg : %s "
				")",
				(int)syscall(SYS_gettid),
				livechat,
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
			"err : %d, "
			"errmsg : %s "
			")",
			(int)syscall(SYS_gettid),
			livechat,
			err,
			errmsg.c_str()
			);

	CloseSessionByLiveChat(livechat);

}

void CamShareMiddleware::OnSendEnterConference(
		ILiveChatClient* livechat,
		int seq,
		const string& fromId,
		const string& toId,
		LCC_ERR_TYPE err,
		const string& errmsg
		) {

	// 结束会话
	Client* client = NULL;
	IRequest* request = FinishSession(livechat, seq, &client);

	if( err == LCC_ERR_SUCCESS ) {
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"CamShareMiddleware::OnSendEnterConference( "
				"tid : %d, "
				"[外部服务(LiveChat), 收到命令:进入会议室, 成功], "
				"livechat : %p, "
				"seq : %d "
				")",
				(int)syscall(SYS_gettid),
				livechat,
				seq
				);

		if( request != NULL ) {
			// 开放成员视频
			mFreeswitch.StartUserRecvVideo(fromId, toId, ((EnterConferenceRequest*)request)->GetMemberType());
		}

	} else {
		LogManager::GetLogManager()->Log(
				LOG_WARNING,
				"CamShareMiddleware::OnSendEnterConference( "
				"tid : %d, "
				"[外部服务(LiveChat), 收到命令:进入会议室, 失败], "
				"livechat : %p, "
				"seq : %d, "
				"err : %d, "
				"errmsg : %s "
				")",
				(int)syscall(SYS_gettid),
				livechat,
				seq,
				err,
				errmsg.c_str()
				);

		// 断开用户
		mFreeswitch.KickUserFromConference(fromId, toId);
	}

	if( request != NULL ) {
//		if( client != NULL ) {
//			// 发送HTTP响应请求
//			DialplanRespond* respond = new DialplanRespond();
//			SendRespond2Client(client, respond);
//		}
		delete request;
	}

}

void CamShareMiddleware::OnRecvDisconnectUserVideo(
		ILiveChatClient* livechat,
		int seq,
		const string& userId1,
		const string& userId2,
		LCC_ERR_TYPE err,
		const string& errmsg) {
	if( err == LCC_ERR_SUCCESS ) {
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"CamShareMiddleware::OnRecvDisconnectUserVideo( "
				"tid : %d, "
				"[外部服务(LiveChat), 收到命令:从会议室踢出用户], "
				"livechat : %p, "
				"seq : %d, "
				"userId1 : %s, "
				"userId2 : %s "
				")",
				(int)syscall(SYS_gettid),
				livechat,
				seq,
				userId1.c_str(),
				userId2.c_str()
				);
		mFreeswitch.KickUserFromConference(userId1, userId2);

	} else {
		LogManager::GetLogManager()->Log(
				LOG_WARNING,
				"CamShareMiddleware::OnRecvDisconnectUserVideo( "
				"tid : %d, "
				"[外部服务(LiveChat), 收到命令:从会议室踢出用户, 失败], "
				"livechat : %p, "
				"seq : %d, "
				"userId1 : %s, "
				"userId2 : %s, "
				"err : %d, "
				"errmsg : %s "
				")",
				(int)syscall(SYS_gettid),
				livechat,
				seq,
				userId1.c_str(),
				userId2.c_str(),
				err,
				errmsg.c_str()
				);
	}
}
/***************************** 外部服务(LiveChat) 回调处理 end **************************************/

/***************************** 外部服务接口 **************************************/
bool CamShareMiddleware::SendEnterConference2LiveChat(
		const string& fromId,
		const string& toId,
		MemberType type,
		Client* client
		) {
	bool bFlag = true;

	int fd = -1;
	if( client != NULL ) {
		fd = client->fd;
	}

	// 找出需要发送的LiveChat Client
	ILiveChatClient* livechat = NULL;
	mLiveChatClientMap.Lock();
	LiveChatClientMap::iterator itr = mLiveChatClientMap.Find(SITE_TYPE_CL);
	if( itr != mLiveChatClientMap.End() ) {
		livechat = itr->second;
	}
	mLiveChatClientMap.Unlock();

	// LiveChat client生成请求序列号
	int seq = -1;
	if( livechat != NULL ) {
		seq = livechat->GetSeq();

	} else {
		bFlag = false;
	}

	if( bFlag ) {
		bFlag = false;

		// 处理是否需要记录请求
		EnterConferenceRequest* request = new EnterConferenceRequest();
		request->SetParam(fromId, toId, type);

		// 生成会话, 处理request是否需要被delete
		if( StartSession(client, livechat, request, seq) ) {
			// 向LiveChat client发送进入聊天室内请求
			if( livechat->SendEnterConference(seq, fromId, toId) ) {
				// 发送请求成功
				LogManager::GetLogManager()->Log(
						LOG_WARNING,
						"CamShareMiddleware::SendEnterConference2LiveChat( "
						"tid : %d, "
						"[外部服务(LiveChat), 发送命令:进入会议室, 外部服务(LiveChat), 发送请求成功], "
						"fd : [%d] "
						")",
						(int)syscall(SYS_gettid),
						fd
						);
				bFlag = true;

			} else {
				LogManager::GetLogManager()->Log(
						LOG_WARNING,
						"CamShareMiddleware::SendEnterConference2LiveChat( "
						"tid : %d, "
						"[外部服务(LiveChat), 发送命令:进入会议室, 外部服务(LiveChat)不在线, 发送请求失败], "
						"fd : [%d] "
						")",
						(int)syscall(SYS_gettid),
						fd
						);
				Client* p = NULL;
				FinishSession(livechat, seq, &p);
			}
		}
	}

	return bFlag;
}
/***************************** 外部服务接口 end **************************************/
