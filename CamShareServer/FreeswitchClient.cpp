/*
 * FreeswitchClient.cpp
 *
 *  Created on: 2016年3月17日
 *      Author: max
 */

#include "FreeswitchClient.h"
#include "LogManager.h"

#include <common/StringHandle.h>

static const char *LEVEL_NAMES[] = {
	"EMERG",
	"ALERT",
	"CRIT",
	"ERROR",
	"WARNING",
	"NOTICE",
	"INFO",
	"DEBUG",
	NULL
};

static int esl_log_level = -1;

static const char *cut_path(const char *in)
{
	const char *p, *ret = in;
	char delims[] = "/\\";
	char *i;

	for (i = delims; *i; i++) {
		p = in;
		while ((p = strchr(p, *i)) != 0) {
			ret = ++p;
		}
	}
	return ret;
}

static void freeswitchLogger(const char *file, const char *func, int line, int level, const char *fmt, ...) {
	const char *fp;
	char *data;
	va_list ap;
	int ret;

	if (level < 0 || level > 7) {
		level = 7;
	}
	if (level > esl_log_level) {
		return;
	}

	fp = cut_path(file);

	va_start(ap, fmt);

	ret = esl_vasprintf(&data, fmt, ap);

	if (ret != -1) {
		LogManager::GetLogManager()->Log(
				LOG_STAT,
				"FreeswitchClient::freeswitchLogger( "
				"tid : %d, "
				"[Freeswitch, event socket log], "
				"[%s] %s:%d %s() %s"
				")",
				(int)syscall(SYS_gettid),
				LEVEL_NAMES[level], fp, line, func, data
				);

		free(data);
	}

	va_end(ap);
}

FreeswitchClient::FreeswitchClient() {
	// TODO Auto-generated constructor stub
	mpFreeswitchClientListener = NULL;
	esl_global_set_default_logger(7);
	esl_global_set_logger(freeswitchLogger);
}

FreeswitchClient::~FreeswitchClient() {
	// TODO Auto-generated destructor stub
}

void FreeswitchClient::SetFreeswitchClientListener(FreeswitchClientListener* listener) {
	mpFreeswitchClientListener = listener;
}

bool FreeswitchClient::Proceed() {
	bool bFlag = false;
	memset(&mFreeswitch, 0, sizeof(mFreeswitch));
	esl_status_t status = esl_connect(&mFreeswitch, "127.0.0.1", 8021, NULL, "ClueCon");

	if( status == ESL_SUCCESS ) {
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"FreeswitchClient::Proceed( "
				"tid : %d, "
				"[Freeswitch, 连接成功] "
				")",
				(int)syscall(SYS_gettid)
				);

		status = esl_events(&mFreeswitch, ESL_EVENT_TYPE_JSON, "CUSTOM conference::maintenance rtmp::login rtmp::disconnect");
		if( status == ESL_SUCCESS ) {
			LogManager::GetLogManager()->Log(
					LOG_MSG,
					"FreeswitchClient::Proceed( "
					"tid : %d, "
					"[Freeswitch, 事件监听成功] "
					")",
					(int)syscall(SYS_gettid)
					);

			// 同步rtmp_seesion
			bFlag = SyncRtmpSessionList();

			// 重新验证当前所有会议室用户
			if( bFlag ) {
				bFlag = AuthorizationAllConference();
			}
			bFlag = true;
			esl_event_t *event = NULL;
			while( bFlag ) {
				status = esl_recv_event_timed(&mFreeswitch, 100, 0, &event);
				if( status == ESL_SUCCESS ) {
					if( event != NULL ) {
						FreeswitchEventHandle(event);
					}
				} else if( status == ESL_BREAK ) {
					continue;
				} else {
					break;
				}
			}

			LogManager::GetLogManager()->Log(
					LOG_WARNING,
					"CamShareiddleware::Proceed( "
					"tid : %d, "
					"[Freeswitch, 连接断开] "
					")",
					(int)syscall(SYS_gettid)
					);

			esl_disconnect(&mFreeswitch);

		} else {
			LogManager::GetLogManager()->Log(
					LOG_WARNING,
					"CamShareiddleware::Proceed( "
					"tid : %d, "
					"[Freeswitch, 事件监听失败] "
					")",
					(int)syscall(SYS_gettid)
					);

			esl_disconnect(&mFreeswitch);
		}

	} else {
		LogManager::GetLogManager()->Log(
				LOG_WARNING,
				"FreeswitchClient::Proceed( "
				"tid : %d, "
				"[Freeswitch, 连接失败] "
				")",
				(int)syscall(SYS_gettid)
				);
	}

	return bFlag;
}

bool FreeswitchClient::KickUserFromConference(
		const string& user,
		const string& conference
		) {
	bool bFlag = false;

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"FreeswitchClient::KickUserFromConference( "
			"tid : %d, "
			"[Freeswitch, 从会议室踢出用户], "
			"user : %s, "
			"conference : %s "
			")",
			(int)syscall(SYS_gettid),
			user.c_str(),
			conference.c_str()
			);

	string memberId = GetMemberIdByUserFromConference(user, conference);
	if( memberId.length() > 0 ) {
		// 踢出用户
		char temp[1024] = {'\0'};
		string result = "";
		snprintf(temp, sizeof(temp), "api conference %s kick %s", conference.c_str(), memberId.c_str());
		if( SendCommandGetResult(temp, result) ) {
			LogManager::GetLogManager()->Log(
					LOG_MSG,
					"FreeswitchClient::KickUserFromConference( "
					"tid : %d, "
					"[Freeswitch, 从会议室踢出用户, 成功] "
					")",
					(int)syscall(SYS_gettid)
					);

			bFlag = true;
		}
	}

	return bFlag;

}

bool FreeswitchClient::StartUserRecvVideo(
		const string& user,
		const string& conference,
		MemberType type
		) {

	bool bFlag = false;

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"FreeswitchClient::StartUserRecvVideo( "
			"tid : %d, "
			"[Freeswitch, 允许用户开始观看聊天室视频], "
			"user : %s, "
			"conference : %s, "
			"type : %d "
			")",
			(int)syscall(SYS_gettid),
			user.c_str(),
			conference.c_str(),
			type
			);

	string memberId = GetMemberIdByUserFromConference(user, conference);
	if( memberId.length() > 0 ) {
		// 允许用户开始观看聊天室视频
		char temp[1024] = {'\0'};
		string result = "";
		snprintf(temp, sizeof(temp), "api conference %s relate %s %s clear", conference.c_str(), memberId.c_str(), memberId.c_str());
		if( SendCommandGetResult(temp, result) ) {
			LogManager::GetLogManager()->Log(
					LOG_MSG,
					"FreeswitchClient::StartUserRecvVideo( "
					"tid : %d, "
					"[Freeswitch, 允许用户开始观看聊天室视频, 还原权限] "
					")",
					(int)syscall(SYS_gettid)
					);

			bFlag = true;
		}

		if( bFlag ) {
			switch(type) {
			case Member:{
				LogManager::GetLogManager()->Log(
						LOG_MSG,
						"FreeswitchClient::StartUserRecvVideo( "
						"tid : %d, "
						"[Freeswitch, 允许用户开始观看聊天室视频, 成功] "
						")",
						(int)syscall(SYS_gettid)
						);
			}break;
			case Moderator:{
				bFlag = false;
				snprintf(temp, sizeof(temp), "api conference %s unvmute %s ", conference.c_str(), memberId.c_str());
				if( SendCommandGetResult(temp, result) ) {
					LogManager::GetLogManager()->Log(
							LOG_MSG,
							"FreeswitchClient::StartUserRecvVideo( "
							"tid : %d, "
							"[Freeswitch, 允许用户开始观看聊天室视频, 成功] "
							")",
							(int)syscall(SYS_gettid)
							);

					bFlag = true;
				}
			}break;
			default:break;
			}

		}
	}

	return bFlag;
}

string FreeswitchClient::GetUserBySession(const string& session) {
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"FreeswitchClient::GetUserBySession( "
			"tid : %d, "
			"[Freeswitch, 获取用户名字], "
			"session : %s "
			")",
			(int)syscall(SYS_gettid),
			session.c_str()
			);

	string user = "";
	mRtmpSessionMap.Lock();
	RtmpUserMap::iterator itr = mRtmpUserMap.Find(session);
	if( itr != mRtmpUserMap.End() ) {
		user = itr->second;
	}
	mRtmpSessionMap.Unlock();

	return user;
}

bool FreeswitchClient::SendCommandGetResult(const string& command, string& result) {
	bool bFlag = false;

	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"FreeswitchClient::SendCommandGetResult( "
			"tid : %d, "
			"[Freeswitch, 发送命令], "
			"command : %s "
			")",
			(int)syscall(SYS_gettid),
			command.c_str()
			);

	esl_event_t* event = NULL;
	esl_status_t status = esl_send_recv(&mFreeswitch, command.c_str());
	if( status == ESL_SUCCESS ) {
		event = mFreeswitch.last_sr_event;
		if( event != NULL && event->body != NULL ) {
			result = event->body;
			bFlag = true;

			LogManager::GetLogManager()->Log(
					LOG_STAT,
					"FreeswitchClient::SendCommandGetResult( "
					"tid : %d, "
					"[Freeswitch, 发送命令, 成功], "
					"result : \n%s\n"
					")",
					(int)syscall(SYS_gettid),
					result.c_str()
					);
		}
	}
	return bFlag;
}

bool FreeswitchClient::SyncRtmpSessionList() {
	bool bFlag = false;

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"FreeswitchClient::SyncRtmpSessionList( "
			"tid : %d, "
			"[Freeswitch, 同步所有用户rtmp_session] "
			")",
			(int)syscall(SYS_gettid)
			);

	// 获取rtmp sessions
	char temp[1024] = {'\0'};
	string result = "";
	snprintf(temp, sizeof(temp), "api rtmp status profile default sessions");
	if( SendCommandGetResult(temp, result) ) {
		LogManager::GetLogManager()->Log(
				LOG_STAT,
				"FreeswitchClient::SyncRtmpSessionList( "
				"tid : %d, "
				"[Freeswitch, 同步所有用户rtmp_session, 获取rtmp_session列表, 成功] "
				")",
				(int)syscall(SYS_gettid)
				);

		bFlag = true;
	}

	if( bFlag ) {
		bFlag = false;

		string::size_type index = 0;
		string::size_type nextIndex = string::npos;

		// 头解析
		string header = StringHandle::findFirstString(result, "uuid,address,user,domain,flashVer,state\n", 0, nextIndex);
		if( nextIndex != string::npos ) {
			string sessions = result.substr(nextIndex, result.length() - nextIndex);
			LogManager::GetLogManager()->Log(
					LOG_STAT,
					"FreeswitchClient::SyncRtmpSessionList( "
					"tid : %d, "
					"[Freeswitch, 同步所有用户rtmp_session, 头解析], "
					"sessions : \n%s\n"
					")",
					(int)syscall(SYS_gettid),
					sessions.c_str()
					);

			bFlag = true;

			// 行解析
			string session = "";
			while ( true ) {
				session = StringHandle::findFirstString(sessions, "\n", index, nextIndex);
				if( nextIndex != string::npos ) {
					LogManager::GetLogManager()->Log(
							LOG_MSG,
							"FreeswitchClient::SyncRtmpSessionList( "
							"tid : %d, "
							"[Freeswitch, 同步所有用户rtmp_session, 行解析], "
							"session : %s "
							")",
							(int)syscall(SYS_gettid),
							session.c_str()
							);

					// 切换下一个
					index = nextIndex;

					// 列解析
					ParseSessionInfo(session);

				} else {
					// 行跳出
					break;
				}
			}
		}
	}

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"FreeswitchClient::SyncRtmpSessionList( "
			"tid : %d, "
			"[Freeswitch, 同步所有用户rtmp_session, 完成], "
			"bFlag : %s "
			")",
			(int)syscall(SYS_gettid),
			bFlag?"true":"false"
			);

	return bFlag;
}

void FreeswitchClient::ParseSessionInfo(const string& session) {
	// 列分隔
	string column;
	string::size_type index = 0;
	string::size_type nextIndex = string::npos;

	int i = 0;
	bool bBreak = false;

	string uuid = "";
	string user = "";
	string status = "";

	while( true ) {
		column = StringHandle::findFirstString(session, ",", index, nextIndex);
		LogManager::GetLogManager()->Log(
				LOG_STAT,
				"FreeswitchClient::ParseSessionInfo( "
				"tid : %d, "
				"[Freeswitch, 解析单个rtmp_session, 列解析], "
				"column : %s "
				")",
				(int)syscall(SYS_gettid),
				column.c_str()
				);

		if( nextIndex != string::npos ) {
			index = nextIndex;

		} else {
			// 最后一个
			if( index < session.length() ) {
				column = session.substr(index, session.length() - index);
			}

			bBreak = true;
		}

		switch(i) {
		case 0:{
			// uuid
			uuid = column;
		}break;
		case 2:{
			// user
			user = column;
		}break;
		case 8:{
			// status
			status = column;
		}break;
		default:break;
		}

		if( uuid.length() > 0 && user.length() > 0 && status.length() > 0 ) {
			if( status == "ESTABLISHED" ) {
				// 插入在线用户列表
				LogManager::GetLogManager()->Log(
						LOG_MSG,
						"FreeswitchClient::ParseSessionInfo( "
						"tid : %d, "
						"[Freeswitch, 解析单个rtmp_session, 插入在线用户列表], "
						"user : %s, "
						"uuid : %s "
						")",
						(int)syscall(SYS_gettid),
						user.c_str(),
						uuid.c_str()
						);

				mRtmpSessionMap.Lock();
				// 插入 user -> session
				RtmpSessionMap::iterator itr = mRtmpSessionMap.Find(user);
				if( itr == mRtmpSessionMap.End() ) {
					mRtmpSessionMap.Insert(user, uuid);
				} else {
					itr->second = uuid;
				}
		    	// 插入 session -> user
		    	RtmpUserMap::iterator itr2 = mRtmpUserMap.Find(uuid);
		    	if( itr2 == mRtmpUserMap.End() ) {
		    		mRtmpUserMap.Insert(uuid, user);
		    	} else {
		    		itr2->second = user;
		    	}
				mRtmpSessionMap.Unlock();
			}

		    // 列跳出
			bBreak = true;
		}

		i++;

		if( bBreak ) {
			// 列跳出
			break;
		}
	}
}

bool FreeswitchClient::AuthorizationAllConference() {
	bool bFlag = true;
	return bFlag;
}

string FreeswitchClient::GetSessionIdByUUID(const string& uuid) {
	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"FreeswitchClient::GetSessionIdByUUID( "
			"tid : %d, "
			"[Freeswitch, 获取用户rtmp_session] "
			")",
			(int)syscall(SYS_gettid)
			);

	// 获取rtmp_session
	char temp[1024] = {'\0'};
	string result = "";
	snprintf(temp, sizeof(temp), "api uuid_getvar %s rtmp_session", uuid.c_str());
	if( SendCommandGetResult(temp, result) ) {
		LogManager::GetLogManager()->Log(
				LOG_STAT,
				"FreeswitchClient::GetSessionIdByUUID( "
				"tid : %d, "
				"[Freeswitch, 获取用户rtmp_session, 成功], "
				"uuid : %s, "
				"session : %s "
				")",
				(int)syscall(SYS_gettid),
				uuid.c_str(),
				result.c_str()
				);
	}

	return result;
}

string FreeswitchClient::GetMemberIdByUserFromConference(
		const string& user,
		const string& conference
		) {
	string memberId = "";

	// 找出匹配的用户(fromId)
	string rtmp_session = "";
	mRtmpSessionMap.Lock();
	RtmpSessionMap::iterator itr = mRtmpSessionMap.Find(user);
	if( itr != mRtmpSessionMap.End() ) {
		rtmp_session = itr->second;
	}
	mRtmpSessionMap.Unlock();

	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"FreeswitchClient::GetMemberIdByUserFromConference( "
			"tid : %d, "
			"[Freeswitch, 在聊天室获取用户member_id], "
			"user : %s, "
			"conference : %s, "
			"rtmp_session : %s "
			")",
			(int)syscall(SYS_gettid),
			user.c_str(),
			conference.c_str(),
			rtmp_session.c_str()
			);

	bool bFlag = false;

	// 获取会议(conference), 用户(user)UUID列表
	char temp[1024] = {'\0'};
	string result = "";
	snprintf(temp, sizeof(temp), "api conference %s xml_list", conference.c_str());
	if( SendCommandGetResult(temp, result) ) {
		LogManager::GetLogManager()->Log(
				LOG_STAT,
				"FreeswitchClient::GetMemberIdByUserFromConference( "
				"tid : %d, "
				"[Freeswitch, 在聊天室获取用户member_id, 获取会议会员列表, 成功] "
				")",
				(int)syscall(SYS_gettid)
				);

		bFlag = true;
	}

	if( bFlag ) {
		TiXmlDocument doc;
		doc.Parse(result.c_str());
		if ( !doc.Error() ) {
			int i = 0;
			string uuId = "";
			string sessionId = "";

			TiXmlHandle handle( &doc );
			while ( true ) {
				TiXmlHandle memberHandle = handle.FirstChild( "conferences" ).FirstChild( "conference" ).FirstChild( "members" ).Child( "member", i );
				if ( memberHandle.ToNode() == NULL ) {
					// 没有更多用户了
					break;
				}

				TiXmlElement* xmlUuId = memberHandle.FirstChild("uuid").ToElement();
				if( xmlUuId ) {
					uuId = xmlUuId->GetText();
					sessionId = GetSessionIdByUUID(uuId);

					// 当前会议用户rtmp_session与需要的用户rtmp_session
					if( sessionId == rtmp_session ) {
						TiXmlElement* xmlMemberId = memberHandle.FirstChild("id").ToElement();
						if( xmlMemberId ) {
							memberId = xmlMemberId->GetText();
							break;
						}
					}
				}

				i++;
			}
		}
	}

	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"FreeswitchClient::GetMemberIdByUserFromConference( "
			"tid : %d, "
			"[Freeswitch, 在聊天室获取用户member_id, 完成], "
			"memberId : %s "
			")",
			(int)syscall(SYS_gettid),
			memberId.c_str()
			);

	return memberId;
}

void FreeswitchClient::FreeswitchEventHandle(esl_event_t *event) {
	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"FreeswitchClient::FreeswitchEventHandle( "
			"tid : %d, "
			"[Freeswitch, 事件处理], "
			"body : \n%s\n"
			")",
			(int)syscall(SYS_gettid),
			event->body
			);

	Json::Value root;
	Json::Reader reader;
	if( reader.parse(event->body, root, false) ) {
		if( root.isObject() ) {
			if( root["Event-Calling-Function"].isString() ) {
				string function = root["Event-Calling-Function"].asString();
				LogManager::GetLogManager()->Log(
						LOG_STAT,
						"FreeswitchClient::FreeswitchEventHandle( "
						"tid : %d, "
						"[Freeswitch, 事件处理], "
						"function : %s "
						")",
						(int)syscall(SYS_gettid),
						function.c_str()
						);

				if( function == "conference_member_add" ) {
					// 增加会议成员
					FreeswitchEventConferenceAddMember(root);

				} else if( function == "rtmp_session_login" ) {
					// rtmp登陆成功
					FreeswitchEventRtmpLogin(root);

				} else if( function == "rtmp_real_session_destroy" ) {
					// rtmp销毁成功
					FreeswitchEventRtmpDestory(root);

				}
			}
		}
	}

}

void FreeswitchClient::FreeswitchEventRtmpLogin(const Json::Value& root) {
	string user;
    if( root["User"].isString() ) {
    	user = root["User"].asString();
    }
    string rtmp_session = "";
    if( root["RTMP-Session-ID"].isString() ) {
    	rtmp_session = root["RTMP-Session-ID"].asString();
    }

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"FreeswitchClient::FreeswitchEventRtmpLogin( "
			"tid : %d, "
			"[Freeswitch, 事件处理, rtmp终端登陆], "
			"user : %s, "
			"rtmp_session : %s "
			")",
			(int)syscall(SYS_gettid),
			user.c_str(),
			rtmp_session.c_str()
			);

	// 插入在线用户列表
    if( user.length() > 0 && rtmp_session.length() > 0 ) {
    	mRtmpSessionMap.Lock();

    	// 插入 user -> session
    	RtmpSessionMap::iterator itr = mRtmpSessionMap.Find(user);
    	if( itr == mRtmpSessionMap.End() ) {
    		mRtmpSessionMap.Insert(user, rtmp_session);
    	} else {
    		// 踢出旧
    		mRtmpUserMap.Erase(itr->second);
        	itr->second = rtmp_session;
    	}

    	// 插入 session -> user
    	RtmpUserMap::iterator itr2 = mRtmpUserMap.Find(rtmp_session);
    	if( itr2 == mRtmpUserMap.End() ) {
    		mRtmpUserMap.Insert(rtmp_session, user);
    	} else {
    		itr2->second = user;
    	}

    	mRtmpSessionMap.Unlock();
    }

}

void FreeswitchClient::FreeswitchEventRtmpDestory(const Json::Value& root) {
    string rtmp_session = "";
    if( root["RTMP-Session-ID"].isString() ) {
    	rtmp_session = root["RTMP-Session-ID"].asString();
    }

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"FreeswitchClient::FreeswitchEventRtmpDestory( "
			"tid : %d, "
			"[Freeswitch, 事件处理, rtmp终端断开], "
			"rtmp_session : %s "
			")",
			(int)syscall(SYS_gettid),
			rtmp_session.c_str()
			);

	// 移出在线用户列表
    if( rtmp_session.length() > 0 ) {
    	mRtmpSessionMap.Lock();
    	RtmpUserMap::iterator itr = mRtmpUserMap.Find(rtmp_session);
    	if( itr != mRtmpUserMap.End() ) {
			mRtmpSessionMap.Erase(itr->second);
    		mRtmpUserMap.Erase(itr);
    	}
    	mRtmpSessionMap.Unlock();
    }
}

void FreeswitchClient::FreeswitchEventConferenceAddMember(const Json::Value& root) {
    string conference_name = "";
    if( root["Conference-Name"].isString() ) {
    	conference_name = root["Conference-Name"].asString();
    }
    string member_id = "";
    if( root["Member-ID"].isString() ) {
    	member_id = root["Member-ID"].asString();
    }
    string member_type = "";
    MemberType type = Member;
    if( root["Member-Type"].isString() ) {
    	member_type = root["Member-Type"].asString();
    	if( member_type == "moderator" ) {
    		type = Moderator;
    	}
    }
    string channel_uuid = "";
    if( root["Channel-Call-UUID"].isString() ) {
    	channel_uuid = root["Channel-Call-UUID"].asString();
    }

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"FreeswitchClient::FreeswitchEventConferenceAddMember( "
			"tid : %d, "
			"[Freeswitch, 事件处理, 增加会议成员], "
			"conference_name : %s, "
			"member_id : %s, "
			"channel_uuid: %s "
			")",
			(int)syscall(SYS_gettid),
			conference_name.c_str(),
			member_id.c_str(),
			channel_uuid.c_str()
			);

	string session = GetSessionIdByUUID(channel_uuid);
	string user = GetUserBySession(session);

	if( mpFreeswitchClientListener ) {
		mpFreeswitchClientListener->OnFreeswitchEventConferenceAddMember(this, user, conference_name, type);
	}

}

