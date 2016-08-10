/*
 * CamShareMiddleware.h
 *
 *  Created on: 2015-1-13
 *      Author: Max.Chiu
 *      Email: Kingsleyyau@gmail.com
 */

#ifndef CamShareMiddleware_H_
#define CamShareMiddleware_H_

#include "Session.h"
#include "Client.h"
#include "MessageList.h"
#include "TcpServer.h"
#include "FreeswitchClient.h"
#include "SessionManager.h"
#include "DBHandler.h"
#include "LogManager.h"

#include <common/ConfFile.hpp>
#include <common/KSafeMap.h>
#include <common/TimeProc.hpp>
#include <common/StringHandle.h>

#include <request/IRequest.h>
#include <respond/IRespond.h>

#include <livechat/ILiveChatClient.h>

#include <map>
#include <list>
using namespace std;

#define VERSION_STRING "1.0.5"

typedef struct SiteConfig {
	SiteConfig() {
		siteId = "";
		recordFinishUrl ="";
	}
	SiteConfig(
			const string& siteId,
			const string& recordFinishUrl
			) {
		this->siteId = siteId;
		this->recordFinishUrl = recordFinishUrl;
	}
	~SiteConfig() {

	}
	string siteId;
	string recordFinishUrl;
}SiteConfig;

// socket -> client
typedef KSafeMap<int, Client*> ClientMap;
// siteId -> livechat client
typedef KSafeMap<string, ILiveChatClient*> LiveChatClientMap;
// siteId -> site config
typedef KSafeMap<string, SiteConfig*> SiteConfigMap;
// client -> session
typedef KSafeMap<Client*, Session*> Client2SessionMap;
// livechat client -> session
typedef KSafeMap<ILiveChatClient*, Session*> LiveChat2SessionMap;

class StateRunnable;
class ConnectLiveChatRunnable;
class ConnectFreeswitchRunnable;
class UploadRecordsRunnable;
class HttpClient;

class CamShareMiddleware : public TcpServerObserver,
								ClientCallback,
								ILiveChatClientListener,
								FreeswitchClientListener {
public:
	CamShareMiddleware();
	virtual ~CamShareMiddleware();

	bool Run(const string& config);
	bool Run();
	bool IsRunning();
	bool Stop();

	/***************************** TcpServer回调 **************************************/
	bool OnAccept(TcpServer *ts, int fd, char* ip);
	void OnRecvMessage(TcpServer *ts, Message *m);
	void OnSendMessage(TcpServer *ts, Message *m);
	void OnDisconnect(TcpServer *ts, int fd);
	void OnClose(TcpServer *ts, int fd);
	void OnTimeoutMessage(TcpServer *ts, Message *m);
	/***************************** TcpServer回调 end **************************************/

	/***************************** 线程处理函数 **************************************/
	/**
	 * 检测状态线程处理
	 */
	void StateHandle();

	/**
	 * 连接LiveChat线程处理
	 */
	void ConnectLiveChatHandle();

	/**
	 * 连接Freeswitch
	 */
	void ConnectFreeswitchHandle();

	/**
	 * 同步本地录制完成记录
	 */
	void UploadRecordsHandle();
	/***************************** 线程处理函数 end **************************************/

	/***************************** 内部服务(HTTP), 命令回调 **************************************/
	void OnClientGetDialplan(
			Client* client,
			const string& rtmpSession,
			const string& channelId,
			const string& conference,
			const string& serverId,
			const string& siteId
			);
	void OnClientRecordFinish(
			Client* client,
			const string& conference,
			const string& siteId,
			const string& filePath,
			const string& startTime,
			const string& endTime
			);
	void OnClientReloadLogConfig(Client* client);
	void OnClientUndefinedCommand(Client* client);
	/***************************** 内部服务(HTTP), 命令回调 end **************************************/

	/***************************** 内部服务(Freeswitch), 命令回调 **************************************/
	void OnFreeswitchConnect(FreeswitchClient* freeswitch);
	void OnFreeswitchEventConferenceAuthorizationMember(
			FreeswitchClient* freeswitch,
			const Channel* channel
			);
	 void OnFreeswitchEventConferenceAddMember(
			 FreeswitchClient* freeswitch,
			 const Channel* channel
			 );
	void OnFreeswitchEventConferenceDelMember(
			FreeswitchClient* freeswitch,
			const Channel* channel
			);
	/***************************** 内部服务(Freeswitch), 命令回调 end **************************************/

	/***************************** 外部服务(LiveChat), 任务回调 **************************************/
	void OnConnect(ILiveChatClient* livechat, LCC_ERR_TYPE err, const string& errmsg);
	void OnDisconnect(ILiveChatClient* livechat, LCC_ERR_TYPE err, const string& errmsg);
	void OnSendEnterConference(ILiveChatClient* livechat, int seq, const string& fromId, const string& toId, LCC_ERR_TYPE err, const string& errmsg);
	void OnRecvEnterConference(ILiveChatClient* livechat, int seq, const string& fromId, const string& toId, bool bAuth, LCC_ERR_TYPE err, const string& errmsg);
	void OnRecvKickUserFromConference(ILiveChatClient* livechat, int seq, const string& fromId, const string& toId, LCC_ERR_TYPE err, const string& errmsg);
	/***************************** 外部服务(LiveChat), 任务回调 end **************************************/

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
	 * HTTP server处理
	 */
	void TcpServerRecvMessageHandle(TcpServer *ts, Message *m);
	void TcpServerTimeoutMessageHandle(TcpServer *ts, Message *m);

	/**
	 * Freeswitch事件处理
	 */
	void FreeswitchEventHandle(esl_event_t *event);

	/***************************** 外部服务接口 **************************************/
	/**
	 * 外部服务(LiveChat), 发送进入聊天室命令
	 * @param	livechat
	 * @param 	fromId		用户Id
	 * @param 	toId		对方Id
	 * @param	type		会员类型
	 * @param   serverId	服务器Id
	 */
	bool SendEnterConference2LiveChat(
			ILiveChatClient* livechat,
			const string& fromId,
			const string& toId,
			MemberType type,
			const string& serverId
			);

	/**
	 * 外部服务(LiveChat), 发送通知客户端进入聊天室命令
	 * @param	livechat
	 * @param 	fromId		用户Id
	 * @param 	toId		对方Id
	 */
	bool SendMsgEnterConference2LiveChat(
			ILiveChatClient* livechat,
			const string& fromId,
			const string& toId
			);

	/**
	 * 外部服务(LiveChat), 发送通知客户端退出聊天室命令
	 * @param	livechat
	 * @param 	fromId		用户Id
	 * @param 	toId		对方Id
	 */
	bool SendMsgExitConference2LiveChat(
			ILiveChatClient* livechat,
			const string& fromId,
			const string& toId
			);

	/**
	 * 发送录制文件完成记录到HTTP服务器
	 * @param	client			HTTP Client
	 * @param 	record			录制记录
	 * @param 	success			服务器返回结果
	 * @param 	errorRecord		是否参数错误
	 * @param 	errorCode		服务器错误码
	 * @return	发送处理			(成功/失败)
	 */
	bool SendRecordFinish(
			HttpClient* client,
			const Record& record,
			bool &success,
			bool &errorRecord,
			string& errorCode
			);

	/***************************** 外部服务接口 end **************************************/

	/***************************** 内部服务接口 **************************************/
	/**
	 * 内部服务(HTTP), 发送请求响应
	 * @param client		客户端
	 * @param respond		响应实例
	 * @return true:发送成功/false:发送失败
	 */
	bool SendRespond2Client(
			Client* client,
			IRespond* respond
			);
	/***************************** 内部服务接口 end **************************************/

	/**
	 * 判断是否测试账号
	 */
	bool CheckTestAccount(
			const string& user
			);

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
	 * 每条线程处理任务速度(个/秒)
	 */
	int miMaxQueryPerThread;

	/**
	 * 请求超时(秒)
	 */
	unsigned int miTimeout;

	/**
	 * flash请求超时(秒)
	 */
	unsigned int miFlashTimeout;
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

	/***************************** Livechat参数 **************************************/
	/**
	 * Livechat服务器端口
	 */
	short miLivechatPort;

	/**
	 * Livechat服务器Ip
	 */
	string mLivechatIp;

	/**
	 * Livechat服务器对本应用唯一标识
	 */
	string mLivechatName;

	/**
	 * Livechat连接成功是否重新验所有证会议室用户
	 */
	bool mAuthorization;

	/***************************** Livechat参数 end **************************************/

	/***************************** Freeswitch参数 **************************************/
	/**
	 * Freeswitch服务器端口
	 */
	short miFreeswitchPort;

	/**
	 * Freeswitch服务器Ip
	 */
	string mFreeswitchIp;

	/**
	 * Freeswitch服务器用户名
	 */
	string mFreeswitchUser;

	/**
	 * Freeswitch服务器密码
	 */
	string mFreeswitchPassword;

	/**
	 * Freeswitch服务器是否录制视频
	 */
	bool mbFreeswitchIsRecording;

	/**
	 * Freeswitch服务器用录制视频目录
	 */
	string mFreeswitchRecordingPath;

	/***************************** Freeswitch参数 end **************************************/

	/***************************** 站点参数 **************************************/
	/**
	 * 站点数量
	 */
	int miSiteCount;

	/**
	 * 上传视频录制记录时间间隔(秒)
	 */
	int miUploadTime;

	/**
	 * 站点配置
	 */
	SiteConfig* mpSiteConfig;

	/***************************** 站点参数 end **************************************/

	/***************************** 处理线程 **************************************/
	/**
	 * 状态监视线程
	 */
	StateRunnable* mpStateRunnable;
	KThread mStateThread;
	/**
	 * 连接Livechat线程
	 */
	ConnectLiveChatRunnable* mpConnectLiveChatRunnable;
	KThread mLiveChatConnectThread;
	/**
	 * 连接Freeswitch线程
	 */
	ConnectFreeswitchRunnable* mpConnectFreeswitchRunnable;
	KThread mConnectFreeswitchThread;
	/**
	 * 同步本地录制完成记录线程
	 */
	UploadRecordsRunnable* mpUploadRecordsRunnable;
	KThread mUploadRecordsThread;
	/***************************** 处理线程 end **************************************/

	/***************************** 统计参数 **************************************/
	/**
	 * 统计请求总数
	 */
	unsigned int mTotal;
	unsigned int mResponed;
	KMutex mCountMutex;

	/**
	 * 发送进入会议室请求数目
	 */
	unsigned int miSendEnterConference;

	/**
	 * 监听线程输出间隔
	 */
	unsigned int miStateTime;

	/***************************** 统计参数 end **************************************/

	/***************************** 运行参数 **************************************/
	/**
	 * 是否运行
	 */
	bool mIsRunning;

	/**
	 * 配置文件锁
	 */
	KMutex mConfigMutex;
	/**
	 * 配置文件
	 */
	string mConfigFile;

	/**
	 * 站点查找配置
	 */
	SiteConfigMap mSiteConfigMap;

	/**
	 * 内部服务(HTTP)
	 */
	TcpServer mClientTcpServer;

	/*
	 * 内部服务(HTTP), 在线客户端列表
	 */
	ClientMap mClientMap;

	/**
	 * 外部服务(LiveChat), 实例列表
	 */
	LiveChatClientMap mLiveChatClientMap;

	/**
	 * 会话管理器
	 */
	SessionManager mSessionManager;

	/**
	 * 客户端缓存数据包buffer
	 */
	MessageList mIdleMessageList;

	/**
	 * Freeswitch实例
	 */
	FreeswitchClient mFreeswitch;

	/**
	 * 请求缓存数据持久化实例
	 */
	DBHandler mDBHandler;

	/***************************** 运行参数 end **************************************/
};

#endif /* CAMSHAREMIDDLEWARE_H_ */
