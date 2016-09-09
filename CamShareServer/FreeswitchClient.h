/*
 * FreeswitchClient.h
 *
 *  Created on: 2016年3月17日
 *      Author: max
 */

#ifndef FREESWITCHCLIENT_H_
#define FREESWITCHCLIENT_H_

#include <common/KSafeMap.h>
#include <common/KSafeList.h>

#include <json/json/json.h>
#include <xml/tinyxml.h>

#include <CommonHeader.h>

#include <esl.h>

#include <map>
#include <string>
using namespace std;

typedef enum MemberType {
	Member,
	Moderator
} MemberType;

typedef struct Channel {
	Channel() {
		user = "";
		conference = "";
		type = Member;
		memberId = "";
		identify = "";
		serverId = "";
		siteId = "";
		recordFilePath = "";
	}
	Channel(
			const string& user,
			const string& conference,
			const string& serverId,
			const string& siteId
			) {
		this->user = user;
		this->conference = conference;
		this->type = Member;
		this->memberId = "";
		this->serverId = serverId;
		this->siteId = siteId;
		identify = user + conference;
		this->recordFilePath = "";
	}
	Channel(
			const string& user,
			const string& conference,
			MemberType type,
			const string& memberId,
			const string& serverId,
			const string& siteId
			) {
		this->user = user;
		this->conference = conference;
		this->type = type;
		this->memberId = memberId;
		this->serverId = serverId;
		this->siteId = siteId;
		identify = user + conference;
		this->recordFilePath = "";
	}
	Channel(const Channel& item) {
		this->user = item.user;
		this->conference = item.conference;
		this->type = item.type;
		this->memberId = item.memberId;
		this->serverId = item.serverId;
		this->siteId = item.siteId;
		this->identify = this->user + this->conference;
		this->recordFilePath = item.recordFilePath;
	}
	Channel& operator=(const Channel& item) {
		this->user = item.user;
		this->conference = item.conference;
		this->type = item.type;
		this->memberId = item.memberId;
		this->serverId = item.serverId;
		this->siteId = item.siteId;
		this->identify = item.user + item.conference;
		this->recordFilePath = item.recordFilePath;
		return *this;
	}

	string GetIdentify() {
		return identify;
	}

	string user;
	string conference;
	MemberType type;
	string memberId;
	string identify;
	string serverId;
	string siteId;
	string recordFilePath;

} Channel;

typedef KSafeList<string> RtmpList;

// username -> rmp_session list
typedef KSafeMap<string, RtmpList*> RtmpSessionMap;

// rmp_session -> username
typedef KSafeMap<string, string> RtmpUserMap;

//// <user,conference> -> channel
//typedef KSafeMap<string, string> RtmpUser2ChannelMap;

// channel -> <user,conference>
typedef KSafeMap<string, Channel*> RtmpChannel2UserMap;

class FreeswitchClient;
class FreeswitchClientListener {
public:
	virtual ~FreeswitchClientListener() {};

	virtual void OnFreeswitchConnect(FreeswitchClient* freeswitch) = 0;

	virtual void OnFreeswitchEventConferenceAuthorizationMember(
			FreeswitchClient* freeswitch,
			const Channel* channel
			) = 0;

	virtual void OnFreeswitchEventConferenceAddMember(
			FreeswitchClient* freeswitch,
			const Channel* channel
			) = 0;

	virtual void OnFreeswitchEventConferenceDelMember(
			FreeswitchClient* freeswitch,
			const Channel* channel
			) = 0;
};

class FreeswitchClient {
public:
	FreeswitchClient();
	virtual ~FreeswitchClient();

	/**
	 * 设置事件监听
	 */
	void SetFreeswitchClientListener(FreeswitchClientListener* listener);

	/**
	 * 设置录制视频参数
	 * @param	bRecording	是否录制视频
	 * @param	path		录制视频目录
	 */
	void SetRecording(bool bRecording = false, const string& path = "/usr/local/freeswitch/recordings");

	/**
	 * 开始连接, 接收事件, blocking
	 * @return true:曾经成功连接/false:连接失败
	 */
	bool Proceed(
			const string& ip,
			short port,
			const string& user,
			const string& password
			);

	/**
	 * 从会议室踢出用户
	 * @param user	用户名
	 * @param conference	会议室
	 */
	bool KickUserFromConference(
			const string& user,
			const string& conference,
			const string& exceptMemberId
			);

	/**
	 * 允许用户开始观看聊天室视频
	 */
	bool StartUserRecvVideo(
			const string& user,
			const string& conference,
			MemberType type
			);

	/**
	 * 获取用户名字
	 * @param session	rtmp_session
	 */
	string GetUserBySession(const string& session);

	/**
	 * 直接从Freeswitch获取用户名字
	 * @param session	rtmp_session
	 */
	string GetUserBySessionDirect(const string& session);

	/**
	 * 重新验证当前所有会议室用户
	 */
	bool AuthorizationAllConference();

	/**
	 * 创建频道
	 */
	bool CreateChannel(
			const string& channelId,
			const Channel& channel,
			bool cover = false
			);
	/**
	 * 销毁频道
	 */
	void DestroyChannel(
			const string& channelId
			);

	/**
	 * 获取通话数目
	 */
	unsigned int GetChannelCount();

	/**
	 * 获取在线用户数目
	 */
	unsigned int GetOnlineUserCount();

private:
	/**
	 * 发送命令
	 * @param cmd		命令
	 * @param result	返回命令结果
	 * @return 成功失败
	 */
	bool SendCommandGetResult(const string& cmd, string& result);

	/**
	 * 同步已经登陆的rtmp_session
	 */
	bool SyncRtmpSessionList();

	/**
	 * 解析单个rtmp_session
	 */
	bool ParseSessionInfo(
			const string& session,
			string& uuid,
			string& user,
			string& status
			);

	/**
	 * 解析单个会议室
	 */
	bool ParseConferenceInfo(
			const string& line,
			string& conference
			);

	/**
	 * 根据UUID获取rtmp_session
	 */
	string GetSessionIdByUUID(const string& uuid);

	/**
	 * 在聊天室获取用户member_id
	 */
	list<string> GetMemberIdByUserFromConference(
			const string& user,
			const string& conference
			);

	/**
	 * 开始录制会议视频
	 * @param	conference		会议名字
	 * @param	siteId			站点Id
	 * @param	filePath[出参]	录制文件路径
	 */
	bool StartRecordConference(const string& conference, const string& siteId, OUT string& filePath);

	/**
	 * 停止录制会议视频
	 * @param	conference	会议名字
	 * @param	filePath	录制文件路径
	 */
	bool StopRecordConference(const string& conference, string& filePath);

	/**
	 * 开始录制频道视频
	 * @param	uuid	频道Id
	 */
	bool StartRecordChannel(const string& uuid);

	/**
	 * 获取会话变量
	 */
	string GetChannelParam(
			const string& uuid,
			const string& key
			);

	/**
	 * 事件处理分发
	 */
	void FreeswitchEventHandle(esl_event_t *event);
	/**
	 * 事件处理, rtmp终端登陆
	 */
	void FreeswitchEventRtmpLogin(const Json::Value& root);
	/**
	 * 事件处理, rtmp终端断开
	 */
	void FreeswitchEventRtmpDestory(const Json::Value& root);
	/**
	 * 事件处理, 增加会议成员
	 */
	void FreeswitchEventConferenceAddMember(const Json::Value& root);
	/**
	 * 事件处理, 删除会议成员
	 */
	void FreeswitchEventConferenceDelMember(const Json::Value& root);
	/**
	 * 事件处理, 创建频道
	 */
	void FreeswitchEventChannelCreate(const Json::Value& root);
	/**
	 * 事件处理, 销毁频道
	 */
	void FreeswitchEventChannelDestroy(const Json::Value& root);

	/**
	 * Freeswitch实例
	 */
	esl_handle_t mFreeswitch;

	/**
	 * Freeswitch user -> rtmp_session map
	 */
	RtmpSessionMap mRtmpSessionMap;

	/**
	 * Freeswitch rtmp_session -> user map
	 */
	RtmpUserMap mRtmpUserMap;

//	/**
//	 * Freeswitch <user,conference> -> channel map
//	 */
//	RtmpUser2ChannelMap mRtmpUser2ChannelMap;

	/**
	 * Freeswitch channel -> <user,conference> map
	 */
	RtmpChannel2UserMap mRtmpChannel2UserMap;

	/**
	 * 事件监听器
	 */
	FreeswitchClientListener* mpFreeswitchClientListener;

	/**
	 * 是否录制视频
	 */
	bool mbIsRecording;

	/**
	 * 录制视频目录
	 */
	string mRecordingPath;

	/**
	 * 命令锁
	 */
	KMutex mCmdMutex;
};

#endif /* FREESWITCHCLIENT_H_ */
