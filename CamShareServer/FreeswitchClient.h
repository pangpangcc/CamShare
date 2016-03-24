/*
 * FreeswitchClient.h
 *
 *  Created on: 2016年3月17日
 *      Author: max
 */

#ifndef FREESWITCHCLIENT_H_
#define FREESWITCHCLIENT_H_

#include <common/KSafeMap.h>

#include <json/json/json.h>
#include <xml/tinyxml.h>

#include <esl.h>

#include <map>
#include <string>
using namespace std;

// username -> rmp_session
typedef KSafeMap<string, string> RtmpSessionMap;

// rmp_session -> username
typedef KSafeMap<string, string> RtmpUserMap;

typedef enum MemberType {
	Member,
	Moderator
};

class FreeswitchClient;
class FreeswitchClientListener {
public:
	virtual ~FreeswitchClientListener() {};
	virtual void OnFreeswitchEventConferenceAddMember(FreeswitchClient* freeswitch, const string& user, const string& conference, MemberType type) = 0;
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
	 * 开始连接, 接收事件, blocking
	 * @return true:曾经成功连接/false:连接失败
	 */
	bool Proceed();

	/**
	 * 从会议室踢出用户
	 * @param user	用户名
	 * @param conference	会议室
	 */
	bool KickUserFromConference(
			const string& user,
			const string& conference
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
	void ParseSessionInfo(const string& session);

	/**
	 * 重新验证当前所有会议室用户
	 */
	bool AuthorizationAllConference();

	/**
	 * 根据UUID获取rtmp_session
	 */
	string GetSessionIdByUUID(const string& uuid);

	/**
	 * 在聊天室获取用户member_id
	 */
	string GetMemberIdByUserFromConference(
			const string& user,
			const string& conference
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

	/**
	 * 事件监听器
	 */
	FreeswitchClientListener* mpFreeswitchClientListener;
};

#endif /* FREESWITCHCLIENT_H_ */
