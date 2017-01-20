/*
 * FreeswitchClientType.h
 *
 *  Created on: 2017年1月20日
 *      Author: max
 */

#ifndef FREESWITCHCLIENTTYPE_H_
#define FREESWITCHCLIENTTYPE_H_

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

#endif /* FREESWITCHCLIENTTYPE_H_ */
