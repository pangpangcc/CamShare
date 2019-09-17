/*
 * FreeswitchClientType.h
 *
 *  Created on: 2017年1月20日
 *      Author: max
 */

#ifndef FREESWITCHCLIENTTYPE_H_
#define FREESWITCHCLIENTTYPE_H_

#include <string>
// Alex, 增加ENTERCONFERENCETYPE
#include <livechat/ILiveChatClientDef.h>
using namespace std;

typedef enum MemberType {
	Member,
	Moderator
} MemberType;

typedef struct Channel {
	Channel() {
		channelId = "";
		user = "";
		conference = "";
		type = Member;
		memberId = "";
		rtmpSession = "";
		identify = "";
		serverId = "";
		siteId = "";
		recordFilePath = "";
        chatType = ENTERCONFERENCETYPE_CAMSHARE;
	}
	Channel(
			const string& channelId,
			const string& user,
			const string& conference,
			const string& serverId,
			const string& siteId
			) {
		this->channelId = channelId;
		this->user = user;
		this->conference = conference;
		this->type = Member;
		this->memberId = "";
		this->rtmpSession = "";
		this->serverId = serverId;
		this->siteId = siteId;
		this->identify = user + conference;
		this->recordFilePath = "";
		this->chatType = ENTERCONFERENCETYPE_CAMSHARE;
	}
	Channel(
			const string& channelId,
			const string& user,
			const string& conference,
			MemberType type,
			const string& memberId,
			const string& rtmp_session,
			const string& serverId,
			const string& siteId,
			ENTERCONFERENCETYPE chatType
			) {
		this->channelId = channelId;
		this->user = user;
		this->conference = conference;
		this->type = type;
		this->memberId = memberId;
		this->rtmpSession = rtmp_session;
		this->serverId = serverId;
		this->siteId = siteId;
		this->identify = user + conference;
		this->recordFilePath = "";
		this->chatType = chatType;
	}
	Channel(const Channel& item) {
		this->channelId = item.channelId;
		this->user = item.user;
		this->conference = item.conference;
		this->type = item.type;
		this->memberId = item.memberId;
		this->rtmpSession = item.rtmpSession;
		this->serverId = item.serverId;
		this->siteId = item.siteId;
		this->identify = this->user + this->conference;
		this->recordFilePath = item.recordFilePath;
		this->chatType = item.chatType;
	}
	Channel& operator=(const Channel& item) {
		this->channelId = item.channelId;
		this->user = item.user;
		this->conference = item.conference;
		this->type = item.type;
		this->memberId = item.memberId;
		this->rtmpSession = item.rtmpSession;
		this->serverId = item.serverId;
		this->siteId = item.siteId;
		this->identify = item.user + item.conference;
		this->recordFilePath = item.recordFilePath;
		this->chatType = item.chatType;
		return *this;
	}

	string GetIdentify() {
		return identify;
	}

	string channelId;
	string user;
	string conference;
	MemberType type;
	string memberId;
	string rtmpSession;
	string identify;
	string serverId;
	string siteId;
	string recordFilePath;
    /*
     * Alex, 验证类型
     */
    ENTERCONFERENCETYPE chatType;

} Channel;

#endif /* FREESWITCHCLIENTTYPE_H_ */
