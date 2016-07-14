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

static int esl_log_level = 0;

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
	}

	va_end(ap);
}

FreeswitchClient::FreeswitchClient() {
	// TODO Auto-generated constructor stub
	esl_global_set_default_logger(7);
	esl_global_set_logger(freeswitchLogger);

	mpFreeswitchClientListener = NULL;
	mRecordingPath = "";
	mbIsRecording = false;
}

FreeswitchClient::~FreeswitchClient() {
	// TODO Auto-generated destructor stub
}

void FreeswitchClient::SetFreeswitchClientListener(FreeswitchClientListener* listener) {
	mpFreeswitchClientListener = listener;
}

void FreeswitchClient::SetRecording(bool bRecording, const string& path) {
	mbIsRecording = bRecording;
	mRecordingPath = path;
}

bool FreeswitchClient::Proceed(
		const string& ip,
		short port,
		const string& user,
		const string& password
		) {
	bool bFlag = false;
	memset(&mFreeswitch, 0, sizeof(mFreeswitch));
	esl_status_t status = esl_connect(&mFreeswitch, ip.c_str(), port, user.c_str(), password.c_str());

	if( status == ESL_SUCCESS ) {
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"FreeswitchClient::Proceed( "
				"tid : %d, "
				"[Freeswitch, 连接成功] "
				")",
				(int)syscall(SYS_gettid)
				);
		if( mpFreeswitchClientListener ) {
			mpFreeswitchClientListener->OnFreeswitchConnect(this);
		}

		status = esl_events(&mFreeswitch, ESL_EVENT_TYPE_JSON,
				"CHANNEL_CREATE CHANNEL_DESTROY "
				"CUSTOM conference::maintenance rtmp::login rtmp::disconnect");

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

			esl_event_t *event = NULL;
			while( bFlag ) {
				status = esl_recv_event_timed(&mFreeswitch, 100, 1, &event);
				if( status == ESL_SUCCESS ) {
					if( event != NULL ) {
						FreeswitchEventHandle(event);
						esl_event_safe_destroy(&event);
					}
				} else if( status == ESL_BREAK ) {
					usleep(100 * 1000);
					continue;
				} else {
					break;
				}
			}

			LogManager::GetLogManager()->Log(
					LOG_MSG,
					"CamShareiddleware::Proceed( "
					"tid : %d, "
					"[Freeswitch, 连接断开] "
					")",
					(int)syscall(SYS_gettid)
					);

			esl_disconnect(&mFreeswitch);

			// 清空通话
			mRtmpChannel2UserMap.Lock();
			mRtmpChannel2UserMap.Clear();
			mRtmpChannel2UserMap.Unlock();

			// 清空在线用户
	    	mRtmpSessionMap.Lock();
	    	mRtmpSessionMap.Clear();
	    	mRtmpUserMap.Clear();
	    	mRtmpSessionMap.Unlock();

		} else {
			LogManager::GetLogManager()->Log(
					LOG_MSG,
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
				LOG_MSG,
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
		const string& conference,
		const string& exceptMemberId
		) {
	bool bFlag = false;

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"FreeswitchClient::KickUserFromConference( "
			"tid : %d, "
			"[Freeswitch, 从会议室踢出用户], "
			"user : %s, "
			"conference : %s, "
			"exceptMemberId : %s "
			")",
			(int)syscall(SYS_gettid),
			user.c_str(),
			conference.c_str(),
			exceptMemberId.c_str()
			);

//	string memberId = GetMemberIdByUserFromConference(user, conference);
	list<string> memberIds = GetMemberIdByUserFromConference(user, conference);
	for(list<string>::iterator itr = memberIds.begin(); itr != memberIds.end(); itr++) {
		string memberId = *itr;

		if( memberId.length() > 0 && memberId != exceptMemberId ) {
			// 踢出用户
			char temp[1024] = {'\0'};
			string result = "";
			snprintf(temp, sizeof(temp), "api conference %s kick %s", conference.c_str(), memberId.c_str());
			if( SendCommandGetResult(temp, result) ) {
				LogManager::GetLogManager()->Log(
						LOG_STAT,
						"FreeswitchClient::KickUserFromConference( "
						"tid : %d, "
						"[Freeswitch, 从会议室踢出用户, 成功] "
						"user : %s, "
						"conference : %s, "
						"exceptMemberId : %s "
						")",
						(int)syscall(SYS_gettid),
						user.c_str(),
						conference.c_str(),
						exceptMemberId.c_str()
						);

				bFlag = true;
			}
		}
	}

	if( !bFlag ) {
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"FreeswitchClient::KickUserFromConference( "
				"tid : %d, "
				"[Freeswitch, 从会议室踢出用户, 会议室不存在需要踢出的用户], "
				"user : %s, "
				"conference : %s, "
				"exceptMemberId : %s "
				")",
				(int)syscall(SYS_gettid),
				user.c_str(),
				conference.c_str(),
				exceptMemberId.c_str()
				);
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

//	string memberId = GetMemberIdByUserFromConference(user, conference);
	list<string> memberIds = GetMemberIdByUserFromConference(user, conference);
	for(list<string>::iterator itr = memberIds.begin(); itr != memberIds.end(); itr++) {
		string memberId = *itr;
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
						"[Freeswitch, 允许用户开始观看聊天室视频, 还原权限], "
						"user : %s, "
						"conference : %s, "
						"type : %d "
						")",
						(int)syscall(SYS_gettid),
						user.c_str(),
						conference.c_str(),
						type
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
							"[Freeswitch, 允许用户开始观看聊天室视频, 开放普通成员权限, 成功], "
							"user : %s, "
							"conference : %s, "
							"type : %d "
							")",
							(int)syscall(SYS_gettid),
							user.c_str(),
							conference.c_str(),
							type
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
								"[Freeswitch, 允许用户开始观看聊天室视频, 开放主持人权限, 成功], "
								"user : %s, "
								"conference : %s, "
								"type : %d "
								")",
								(int)syscall(SYS_gettid),
								user.c_str(),
								conference.c_str(),
								type
								);

						bFlag = true;
					}
				}break;
				default:break;
				}

			}
		}
	}

	if( !bFlag ) {
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"FreeswitchClient::StartUserRecvVideo( "
				"tid : %d, "
				"[Freeswitch, 允许用户开始观看聊天室视频, 失败], "
				"user : %s, "
				"conference : %s, "
				"type : %d "
				")",
				(int)syscall(SYS_gettid),
				user.c_str(),
				conference.c_str(),
				type
				);
	}

	return bFlag;
}

string FreeswitchClient::GetUserBySession(const string& session) {
	string user = "";
	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"FreeswitchClient::GetUserBySession( "
			"tid : %d, "
			"[Freeswitch, 获取用户名字], "
			"session : %s "
			")",
			(int)syscall(SYS_gettid),
			session.c_str()
			);

	mRtmpSessionMap.Lock();
	RtmpUserMap::iterator itr = mRtmpUserMap.Find(session);
	if( itr != mRtmpUserMap.End() ) {
		user = itr->second;
	}
	mRtmpSessionMap.Unlock();

	if( user.length() == 0 ) {
		user = GetUserBySessionDirect(session);
	}

	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"FreeswitchClient::GetUserBySession( "
			"tid : %d, "
			"[Freeswitch, 获取用户名字, 完成], "
			"session : %s, "
			"user : %s "
			")",
			(int)syscall(SYS_gettid),
			session.c_str(),
			user.c_str()
			);

	return user;
}

string FreeswitchClient::GetUserBySessionDirect(const string& session) {
	string userName = "";

	bool bFlag = false;

	// 获取rtmp sessions
	char temp[1024] = {'\0'};
	string result = "";
	snprintf(temp, sizeof(temp), "api rtmp status profile default sessions");
	if( SendCommandGetResult(temp, result) ) {
		LogManager::GetLogManager()->Log(
				LOG_STAT,
				"FreeswitchClient::GetUserBySessionDirect( "
				"tid : %d, "
				"[Freeswitch, 直接从Freeswitch获取用户名字, 获取rtmp_session列表, 成功] "
				")",
				(int)syscall(SYS_gettid)
				);

		bFlag = true;
	}

	if( bFlag ) {
		string::size_type index = 0;
		string::size_type nextIndex = string::npos;

		// 头解析
		string header = StringHandle::findFirstString(result, "uuid,address,user,domain,flashVer,state\n", 0, nextIndex);
		if( nextIndex != string::npos ) {
			string sessions = result.substr(nextIndex, result.length() - nextIndex);
			LogManager::GetLogManager()->Log(
					LOG_STAT,
					"FreeswitchClient::GetUserBySessionDirect( "
					"tid : %d, "
					"[Freeswitch, 直接从Freeswitch获取用户名字, 头解析], "
					"sessions : \n%s\n"
					")",
					(int)syscall(SYS_gettid),
					sessions.c_str()
					);

			bool bContinue = true;

			// 行解析
			string sessionLine = "";
			while ( bContinue ) {
				sessionLine = StringHandle::findFirstString(sessions, "\n", index, nextIndex);
				if( nextIndex != string::npos ) {
					LogManager::GetLogManager()->Log(
							LOG_STAT,
							"FreeswitchClient::GetUserBySessionDirect( "
							"tid : %d, "
							"[Freeswitch, 直接从Freeswitch获取用户名字, 行解析], "
							"sessionLine : %s "
							")",
							(int)syscall(SYS_gettid),
							sessionLine.c_str()
							);

					// 切换下一个
					index = nextIndex;

					// 列解析
					string rtmp_session = "";
					string user = "";
					string status = "";
					ParseSessionInfo(sessionLine, rtmp_session, user, status);

					if( rtmp_session.length() > 0 && user.length() > 0 && status.length() > 0 ) {
						if( rtmp_session == session ) {
							LogManager::GetLogManager()->Log(
									LOG_MSG,
									"FreeswitchClient::GetUserBySessionDirect( "
									"tid : %d, "
									"[Freeswitch, 直接从Freeswitch获取用户名字, 成功], "
									"user : %s, "
									"rtmp_session : %s "
									")",
									(int)syscall(SYS_gettid),
									user.c_str(),
									rtmp_session.c_str()
									);

							userName = user;
							bContinue = false;
							break;
						}
					}

				} else {
					// 行跳出
					break;
				}
			}
		}
	}

	return userName;
}

bool FreeswitchClient::SendCommandGetResult(const string& command, string& result) {
	bool bFlag = false;

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"FreeswitchClient::SendCommandGetResult( "
			"tid : %d, "
			"[Freeswitch, 发送命令], "
			"command : %s "
			")",
			(int)syscall(SYS_gettid),
			command.c_str()
			);

	if( mFreeswitch.connected == 1 ) {
		esl_event_t* event = NULL;
		esl_status_t status = esl_send_recv(&mFreeswitch, command.c_str());
		if( status == ESL_SUCCESS ) {
			bFlag = true;
			LogManager::GetLogManager()->Log(
					LOG_STAT,
					"FreeswitchClient::SendCommandGetResult( "
					"tid : %d, "
					"[Freeswitch, 发送命令, 成功] "
					")",
					(int)syscall(SYS_gettid)
					);

			event = mFreeswitch.last_sr_event;
			if( event != NULL && event->body != NULL ) {
				result = event->body;
			} else if( mFreeswitch.last_sr_reply != NULL ){
				result = mFreeswitch.last_sr_reply;
			}

//			if( event != NULL && event->body != NULL ) {
//				LogManager::GetLogManager()->Log(
//						LOG_STAT,
//						"FreeswitchClient::SendCommandGetResult( "
//						"tid : %d, "
//						"[Freeswitch, 发送命令, 命令返回], "
//						"mFreeswitch.last_sr_event->body : \n%s\n"
//						")",
//						(int)syscall(SYS_gettid),
//						mFreeswitch.last_sr_event->body
//						);
//			}
//			if( mFreeswitch.last_sr_reply != NULL ){
//				LogManager::GetLogManager()->Log(
//						LOG_STAT,
//						"FreeswitchClient::SendCommandGetResult( "
//						"tid : %d, "
//						"[Freeswitch, 发送命令, 命令返回], "
//						"mFreeswitch.last_sr_reply : \n%s\n"
//						")",
//						(int)syscall(SYS_gettid),
//						mFreeswitch.last_sr_reply
//						);
//			}

			LogManager::GetLogManager()->Log(
					LOG_STAT,
					"FreeswitchClient::SendCommandGetResult( "
					"tid : %d, "
					"[Freeswitch, 发送命令, 命令返回], "
					"result : \n%s\n"
					")",
					(int)syscall(SYS_gettid),
					result.c_str()
					);

		}
	}

	if( !bFlag ) {
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"FreeswitchClient::SendCommandGetResult( "
				"tid : %d, "
				"[Freeswitch, 发送命令, 失败], "
				"command : %s "
				")",
				(int)syscall(SYS_gettid),
				command.c_str()
				);
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
			string sessionLine = "";
			while ( true ) {
				sessionLine = StringHandle::findFirstString(sessions, "\n", index, nextIndex);
				if( nextIndex != string::npos ) {
					LogManager::GetLogManager()->Log(
							LOG_STAT,
							"FreeswitchClient::SyncRtmpSessionList( "
							"tid : %d, "
							"[Freeswitch, 同步所有用户rtmp_session, 行解析], "
							"sessionLine : %s "
							")",
							(int)syscall(SYS_gettid),
							sessionLine.c_str()
							);

					// 切换下一个
					index = nextIndex;

					// 列解析
					string rtmp_session = "";
					string user = "";
					string status = "";
					ParseSessionInfo(sessionLine, rtmp_session, user, status);

					if( rtmp_session.length() > 0 && user.length() > 0 && status.length() > 0 ) {
						if( status == "ESTABLISHED" ) {
							// 插入在线用户列表
							LogManager::GetLogManager()->Log(
									LOG_MSG,
									"FreeswitchClient::SyncRtmpSessionList( "
									"tid : %d, "
									"[Freeswitch, 同步所有用户rtmp_session, 插入在线用户列表], "
									"user : %s, "
									"rtmp_session : %s "
									")",
									(int)syscall(SYS_gettid),
									user.c_str(),
									rtmp_session.c_str()
									);

							mRtmpSessionMap.Lock();
							// 插入 user -> session
							RtmpSessionMap::iterator itr = mRtmpSessionMap.Find(user);
					    	if( itr == mRtmpSessionMap.End() ) {
					    		// 插入新的用户
					    		RtmpList* pRtmpList = new RtmpList();
					    		pRtmpList->PushBack(rtmp_session);
					    		mRtmpSessionMap.Insert(user, pRtmpList);

					    	} else {
					    		// 加入用户的rtmp session列表
					    		RtmpList* pRtmpList = itr->second;
					    		pRtmpList->PushBack(rtmp_session);

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

				} else {
					// 行跳出
					break;
				}
			}
		}
	}

	if( !bFlag ) {
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"FreeswitchClient::SyncRtmpSessionList( "
				"tid : %d, "
				"[Freeswitch, 同步所有用户rtmp_session, 失败] "
				")",
				(int)syscall(SYS_gettid)
				);
	}

	return bFlag;
}

bool FreeswitchClient::ParseSessionInfo(
		const string& session,
		string& uuid,
		string& user,
		string& status
		) {
	bool bFlag = false;

	// 列分隔
	string column;
	string::size_type index = 0;
	string::size_type nextIndex = string::npos;

	int i = 0;
	bool bBreak = false;

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
		case 5:{
			// status
			status = column;
		}break;
		default:break;
		}

		if( uuid.length() > 0 && user.length() > 0 && status.length() > 0 ) {
		    // 列跳出
			bBreak = true;
			bFlag = true;
		}

		i++;

		if( bBreak ) {
			// 列跳出
			break;
		}
	}

	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"FreeswitchClient::ParseSessionInfo( "
			"tid : %d, "
			"[Freeswitch, 解析单个rtmp_session, 列解析], "
			"uuid : %s, "
			"user : %s, "
			"status : %s "
			")",
			(int)syscall(SYS_gettid),
			uuid.c_str(),
			user.c_str(),
			status.c_str()
			);

	return bFlag;
}

bool FreeswitchClient::AuthorizationAllConference() {
	bool bFlag = false;

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"FreeswitchClient::AuthorizationAllConference( "
			"tid : %d, "
			"[Freeswitch, 重新验证当前所有会议室用户] "
			")",
			(int)syscall(SYS_gettid)
			);

	if( mFreeswitch.connected != 1 ) {
		// 未连接
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"FreeswitchClient::AuthorizationAllConference( "
				"tid : %d, "
				"[Freeswitch, 重新验证当前所有会议室用户, 未连接成功, 不处理返回] "
				")",
				(int)syscall(SYS_gettid)
				);
		return false;
	}

	char temp[1024] = {'\0'};
	string result = "";
	snprintf(temp, sizeof(temp), "api conference xml_list");
	if( SendCommandGetResult(temp, result) ) {
		LogManager::GetLogManager()->Log(
				LOG_STAT,
				"FreeswitchClient::AuthorizationAllConference( "
				"tid : %d, "
				"[Freeswitch, 重新验证当前所有会议室用户, 获取会议室列表] "
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
			string conferenceName = "";

			TiXmlHandle handle( &doc );
			while ( true ) {
				TiXmlHandle conferenceHandle = handle.FirstChild( "conferences" ).Child( "conference", i );
				if ( conferenceHandle.ToNode() == NULL ) {
					// 没有更多会议室
					LogManager::GetLogManager()->Log(
							LOG_MSG,
							"FreeswitchClient::AuthorizationAllConference( "
							"tid : %d, "
							"[Freeswitch, 重新验证当前所有会议室用户, 成功] "
							")",
							(int)syscall(SYS_gettid)
							);
					break;
				}

				TiXmlElement* conferenceElement = conferenceHandle.ToElement();
				if( conferenceElement ) {
					conferenceName = conferenceElement->Attribute("name");
					LogManager::GetLogManager()->Log(
							LOG_STAT,
							"FreeswitchClient::AuthorizationAllConference( "
							"tid : %d, "
							"[Freeswitch, 重新验证当前所有会议室用户, 获取到会议室], "
							"conferenceName : %s "
							")",
							(int)syscall(SYS_gettid),
							conferenceName.c_str()
							);

					string sessionId = "";
					string uuId = "";
					string user = "";
					string memberId = "";
					MemberType type = Member;
					int j = 0;

					while( true ) {
						TiXmlHandle memberHandle = conferenceHandle.FirstChild( "members" ).Child( "member", j );
						if ( memberHandle.ToNode() == NULL ) {
							// 没有更多会员
							LogManager::GetLogManager()->Log(
									LOG_STAT,
									"FreeswitchClient::AuthorizationAllConference( "
									"tid : %d, "
									"[Freeswitch, 重新验证当前所有会议室用户, 没有更多会员], "
									"conferenceName : %s "
									")",
									(int)syscall(SYS_gettid),
									conferenceName.c_str()
									);
							break;
						}

						TiXmlElement* uuidElement = memberHandle.FirstChild("uuid").ToElement();
						if( uuidElement ) {
							uuId = uuidElement->GetText();
							sessionId = GetSessionIdByUUID(uuId);
							user = GetUserBySession(sessionId);
						}

						TiXmlElement* memberIdElement = memberHandle.FirstChild("id").ToElement();
						if( memberIdElement ) {
							memberId = memberIdElement->GetText();
						}

						TiXmlElement* moderatorElement = memberHandle.FirstChild("flags").FirstChild("is_moderator").ToElement();
						if( moderatorElement ) {
							if( strcmp(moderatorElement->GetText(), "true") == 0 ) {
								type = Moderator;
							}
						}

						if( user.length() > 0 && conferenceName.length() > 0 ) {
							string serverId = GetChannelParam(uuId, "serverId");
							string siteId = GetChannelParam(uuId, "siteId");

							// 插入channel
							Channel channel(user, conferenceName, type, memberId, serverId, siteId);
						    CreateChannel(uuId, channel);

						    // 回调重新验证聊天室用户
							if( mpFreeswitchClientListener ) {
								mpFreeswitchClientListener->OnFreeswitchEventConferenceAuthorizationMember(this, &channel);
							}
						}

						j++;
					}
				}

				i++;
			}
		}

	}

	if( !bFlag ) {
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"FreeswitchClient::AuthorizationAllConference( "
				"tid : %d, "
				"[Freeswitch, 重新验证当前所有会议室用户, 失败] "
				")",
				(int)syscall(SYS_gettid)
				);
	}

	return bFlag;
}

bool FreeswitchClient::CreateChannel(
		const string& channelId,
		const Channel& channel,
		bool cover
		) {
	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"FreeswitchClient::CreateChannel( "
			"tid : %d, "
			"[Freeswitch, 创建频道], "
			"channelId : %s, "
			"user : %s, "
			"conference : %s, "
			"serverId : %s, "
			"siteId : %s "
			")",
			(int)syscall(SYS_gettid),
			channelId.c_str(),
			channel.user.c_str(),
			channel.conference.c_str(),
			channel.serverId.c_str(),
			channel.siteId.c_str()
			);

	bool bFlag = false;

	// 插入channel
	mRtmpChannel2UserMap.Lock();
	RtmpChannel2UserMap::iterator itr = mRtmpChannel2UserMap.Find(channelId);
	Channel* newChannel = NULL;
	if( itr != mRtmpChannel2UserMap.End() ) {
		if( cover ) {
			newChannel = itr->second;
			*newChannel = channel;
		}
	} else {
		newChannel = new Channel(channel);
		mRtmpChannel2UserMap.Insert(channelId, newChannel);
		bFlag = true;
	}
	mRtmpChannel2UserMap.Unlock();

	return bFlag;
}

void FreeswitchClient::DestroyChannel(
		const string& channelId
		) {
	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"FreeswitchClient::DestroyChannel( "
			"tid : %d, "
			"[Freeswitch, 销毁频道], "
			"channelId : %s "
			")",
			(int)syscall(SYS_gettid),
			channelId.c_str()
			);
	// 删除channel
	mRtmpChannel2UserMap.Lock();
	RtmpChannel2UserMap::iterator itr = mRtmpChannel2UserMap.Find(channelId);
	if( itr != mRtmpChannel2UserMap.End() ) {
		// 清除channel -> <user,conference> 关系
		mRtmpChannel2UserMap.Erase(itr);

		Channel* channel = itr->second;
		if( channel ) {
			delete channel;
		}
	}
	mRtmpChannel2UserMap.Unlock();
}

unsigned int FreeswitchClient::GetChannelCount() {
	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"FreeswitchClient::GetChannelCount( "
			"tid : %d, "
			"[Freeswitch, 获取通话数目] "
			")",
			(int)syscall(SYS_gettid)
			);
	unsigned int count = 0;
	mRtmpChannel2UserMap.Lock();
	count = mRtmpChannel2UserMap.Size();
	mRtmpChannel2UserMap.Unlock();
	return count;
}

unsigned int FreeswitchClient::GetOnlineUserCount() {
	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"FreeswitchClient::GetOnlineUserCount( "
			"tid : %d, "
			"[Freeswitch, 获取在线用户数目] "
			")",
			(int)syscall(SYS_gettid)
			);
	unsigned int count = 0;
	mRtmpSessionMap.Lock();
	count = mRtmpUserMap.Size();
	mRtmpSessionMap.Unlock();
	return count;
}

string FreeswitchClient::GetChannelParam(
		const string& uuid,
		const string& key
		) {
	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"FreeswitchClient::GetChannelParam( "
			"tid : %d, "
			"[Freeswitch, 获取会话变量], "
			"uuid : %s, "
			"key : %s "
			")",
			(int)syscall(SYS_gettid),
			uuid.c_str(),
			key.c_str()
			);

	// 获取会话变量
	char temp[1024] = {'\0'};
	string result = "";
	snprintf(temp, sizeof(temp), "api uuid_getvar %s %s", uuid.c_str(), key.c_str());
	if( SendCommandGetResult(temp, result) ) {
		LogManager::GetLogManager()->Log(
				LOG_STAT,
				"FreeswitchClient::GetChannelParam( "
				"tid : %d, "
				"[Freeswitch, 获取会话变量, 成功], "
				"uuid : %s, "
				"key : %s, "
				"value : %s "
				")",
				(int)syscall(SYS_gettid),
				uuid.c_str(),
				key.c_str(),
				result.c_str()
				);
	} else {
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"FreeswitchClient::GetChannelParam( "
				"tid : %d, "
				"[Freeswitch, 获取会话变量, 失败], "
				"uuid : %s, "
				"key : %s, "
				"value : %s "
				")",
				(int)syscall(SYS_gettid),
				uuid.c_str(),
				key.c_str(),
				result.c_str()
				);
	}

	return result;
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
	} else {
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"FreeswitchClient::GetSessionIdByUUID( "
				"tid : %d, "
				"[Freeswitch, 获取用户rtmp_session, 失败], "
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

list<string> FreeswitchClient::GetMemberIdByUserFromConference(
		const string& user,
		const string& conference
		) {
	string memberId = "";
	list<string> memberIds;
	// 找出匹配的用户(fromId)
	RtmpList* pRtmpList = NULL;
	mRtmpSessionMap.Lock();
	RtmpSessionMap::iterator itr = mRtmpSessionMap.Find(user);
	if( itr != mRtmpSessionMap.End() ) {
		pRtmpList = itr->second;
	}
	mRtmpSessionMap.Unlock();

	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"FreeswitchClient::GetMemberIdByUserFromConference( "
			"tid : %d, "
			"[Freeswitch, 在聊天室获取用户member_id], "
			"user : %s, "
			"conference : %s "
			")",
			(int)syscall(SYS_gettid),
			user.c_str(),
			conference.c_str()
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
		bFlag = false;
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

					if( pRtmpList != NULL ) {
						pRtmpList->Lock();
						for(RtmpList::iterator itr = pRtmpList->Begin(); itr != pRtmpList->End(); itr++) {
							// 当前会议用户rtmp_session与需要的用户rtmp_session
							if( sessionId == *itr ) {
								LogManager::GetLogManager()->Log(
										LOG_STAT,
										"FreeswitchClient::GetMemberIdByUserFromConference( "
										"tid : %d, "
										"[Freeswitch, 在聊天室获取用户member_id, 找到对应rtmp_session], "
										"user : %s, "
										"conference : %s, "
										"sessionId : %s "
										")",
										(int)syscall(SYS_gettid),
										user.c_str(),
										conference.c_str(),
										sessionId.c_str()
										);

								TiXmlElement* xmlMemberId = memberHandle.FirstChild("id").ToElement();
								if( xmlMemberId ) {
									memberId = xmlMemberId->GetText();
									memberIds.push_back(memberId);
									bFlag = true;
								}
//								break;
							}
						}
						pRtmpList->Unlock();
					}

				}

				i++;
			}
		}
	}

	if( !bFlag ) {
		LogManager::GetLogManager()->Log(
				LOG_STAT,
				"FreeswitchClient::GetMemberIdByUserFromConference( "
				"tid : %d, "
				"[Freeswitch, 在聊天室获取用户member_id, 失败], "
				"user : %s, "
				"conference : %s "
				")",
				(int)syscall(SYS_gettid),
				user.c_str(),
				conference.c_str()
				);
	}

	return memberIds;
//	return memberId;
}

bool FreeswitchClient::StartRecordConference(
		const string& conference,
		const string& siteId
		) {
	bool bFlag = false;
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"FreeswitchClient::StartRecordConference( "
			"tid : %d, "
			"[Freeswitch, 开始录制会议视频], "
			"conference : %s, "
			"siteId : %s "
			")",
			(int)syscall(SYS_gettid),
			conference.c_str(),
			siteId.c_str()
			);

	char temp[1024] = {'\0'};
	string result = "";

	// get current time
	char timeBuffer[64];
	time_t stm = time(NULL);
	struct tm tTime;
	localtime_r(&stm, &tTime);
	snprintf(timeBuffer, 64, "%d%02d%02d%02d%02d%02d", tTime.tm_year + 1900, tTime.tm_mon + 1, tTime.tm_mday, tTime.tm_hour, tTime.tm_min, tTime.tm_sec);

	snprintf(temp, sizeof(temp), "api conference %s record %s/%s_%s_%s.h264",
			conference.c_str(),
			mRecordingPath.c_str(),
			conference.c_str(),
			siteId.c_str(),
			timeBuffer
			);

	if( SendCommandGetResult(temp, result) ) {
		LogManager::GetLogManager()->Log(
				LOG_STAT,
				"FreeswitchClient::StartRecordConference( "
				"tid : %d, "
				"[Freeswitch, 开始录制会议视频, 成功], "
				"conference : %s, "
				"siteId : %s "
				")",
				(int)syscall(SYS_gettid),
				conference.c_str(),
				siteId.c_str()
				);
		bFlag = true;
	}

	if( !bFlag ) {
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"FreeswitchClient::StartRecordConference( "
				"tid : %d, "
				"[Freeswitch, 开始录制会议视频, 失败], "
				"conference : %s, "
				"siteId : %s "
				")",
				(int)syscall(SYS_gettid),
				conference.c_str(),
				siteId.c_str()
				);
	}

	return bFlag;
}

bool FreeswitchClient::StartRecordChannel(const string& uuid) {
	bool bFlag = false;
	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"FreeswitchClient::StartRecordChannel( "
			"tid : %d, "
			"[Freeswitch, 开始录制频道视频], "
			"uuid : %s "
			")",
			(int)syscall(SYS_gettid),
			uuid.c_str()
			);

	char temp[1024] = {'\0'};
	string result = "";
	snprintf(temp, sizeof(temp), "api uuid_setvar %s enable_file_write_buffering false", uuid.c_str());
	if( SendCommandGetResult(temp, result) ) {
		LogManager::GetLogManager()->Log(
				LOG_STAT,
				"FreeswitchClient::StartRecordChannel( "
				"tid : %d, "
				"[Freeswitch, 开始录制频道视频, 设置不使用buffer], "
				"uuid : %s "
				")",
				(int)syscall(SYS_gettid),
				uuid.c_str()
				);
		bFlag = true;
	}

	if( bFlag ) {
		bFlag = false;

		string session = GetSessionIdByUUID(uuid);
		string user = GetUserBySession(session);

	    // get current time
		char timeBuffer[64];
	    time_t stm = time(NULL);
        struct tm tTime;
        localtime_r(&stm, &tTime);
        snprintf(timeBuffer, 64, "%d-%02d-%02d-%02d-%02d-%02d", tTime.tm_year + 1900, tTime.tm_mon + 1, tTime.tm_mday, tTime.tm_hour, tTime.tm_min, tTime.tm_sec);

		snprintf(temp, sizeof(temp), "api uuid_record %s start %s/%s-%s.h264",
				uuid.c_str(),
				mRecordingPath.c_str(),
				user.c_str(),
				timeBuffer
				);

		if( SendCommandGetResult(temp, result) ) {
			LogManager::GetLogManager()->Log(
					LOG_STAT,
					"FreeswitchClient::StartRecordChannel( "
					"tid : %d, "
					"[Freeswitch, 开始录制频道视频, 成功], "
					"uuid : %s "
					")",
					(int)syscall(SYS_gettid),
					uuid.c_str()
					);
			bFlag = true;
		}
	}

	if( !bFlag ) {
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"FreeswitchClient::StartRecordChannel( "
				"tid : %d, "
				"[Freeswitch, 开始录制频道视频, 失败], "
				"uuid : %s "
				")",
				(int)syscall(SYS_gettid),
				uuid.c_str()
				);
	}

	return bFlag;
}

void FreeswitchClient::FreeswitchEventHandle(esl_event_t *event) {
//	LogManager::GetLogManager()->Log(
//			LOG_STAT,
//			"FreeswitchClient::FreeswitchEventHandle( "
//			"tid : %d, "
//			"[Freeswitch, 事件处理], "
//			"body : \n%s\n"
//			")",
//			(int)syscall(SYS_gettid),
//			event->body
//			);

	Json::Value root;
	Json::Reader reader;
	if( event->body != NULL && reader.parse(event->body, root, false) ) {
		if( root.isObject() && root["Event-Name"].isString() ) {
			string event = root["Event-Name"].asString();
			LogManager::GetLogManager()->Log(
					LOG_STAT,
					"FreeswitchClient::FreeswitchEventHandle( "
					"tid : %d, "
					"[Freeswitch, 事件处理], "
					"event : %s "
					")",
					(int)syscall(SYS_gettid),
					event.c_str()
					);

			if( event == "CUSTOM" ) {
				// 自定义模块事件
				if( root["Event-Calling-Function"].isString() ) {
					string function = root["Event-Calling-Function"].asString();
					LogManager::GetLogManager()->Log(
							LOG_STAT,
							"FreeswitchClient::FreeswitchEventHandle( "
							"tid : %d, "
							"[Freeswitch, 事件处理, 自定义模块事件], "
							"function : %s "
							")",
							(int)syscall(SYS_gettid),
							function.c_str()
							);

					if( function == "rtmp_session_login" ) {
						// rtmp登陆成功
						FreeswitchEventRtmpLogin(root);

					} else if( function == "rtmp_real_session_destroy" ) {
						// rtmp销毁成功
						FreeswitchEventRtmpDestory(root);
					} else if( function == "conference_member_add" ) {
						// 增加会议成员
						FreeswitchEventConferenceAddMember(root);

					} else if( function == "conference_member_del" ) {
						// 删除会议成员
						FreeswitchEventConferenceDelMember(root);
					}
	 			}
			} else {
				// 标准事件
				if( event == "CHANNEL_CREATE" ) {
					// 创建频道
					FreeswitchEventChannelCreate(root);

				} else if( event == "CHANNEL_DESTROY" ) {
					// 销毁频道
					FreeswitchEventChannelDestroy(root);
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
    		// 插入新的用户
    		RtmpList* pRtmpList = new RtmpList();
    		pRtmpList->PushBack(rtmp_session);
    		mRtmpSessionMap.Insert(user, pRtmpList);

    	} else {
    		// 加入用户的rtmp session列表
    		RtmpList* pRtmpList = itr->second;
    		pRtmpList->PushBack(rtmp_session);

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

	// 移出在线用户列表
    if( rtmp_session.length() > 0 ) {
    	mRtmpSessionMap.Lock();
    	RtmpUserMap::iterator itr = mRtmpUserMap.Find(rtmp_session);
    	if( itr != mRtmpUserMap.End() ) {
    		LogManager::GetLogManager()->Log(
    				LOG_MSG,
    				"FreeswitchClient::FreeswitchEventRtmpDestory( "
    				"tid : %d, "
    				"[Freeswitch, 事件处理, rtmp终端断开, 已登陆], "
    				"rtmp_session : %s, "
    				"user : %s "
    				")",
    				(int)syscall(SYS_gettid),
    				rtmp_session.c_str(),
					itr->second.c_str()
    				);

    		// 查找用户的对应的rtmp_session list
        	RtmpSessionMap::iterator itr2 = mRtmpSessionMap.Find(itr->second);
        	if( itr2 != mRtmpSessionMap.End() ) {
        		RtmpList* pRtmpList = itr2->second;

        		if( pRtmpList->Size() == 0 ) {
        			// 清除user -> rtmp_session关系
        			delete pRtmpList;
        			pRtmpList = NULL;
        			mRtmpSessionMap.Erase(itr2);

        		} else {
        			// 移除用户的对应的一个rtmp_session
        			pRtmpList->Lock();
        			for(RtmpList::iterator itr = pRtmpList->Begin(); itr != pRtmpList->End();) {
        				if( rtmp_session == *itr ) {
        					pRtmpList->PopValueUnSafe(itr++);
        					break;
        				} else {
        					itr++;
        				}
        			}
        			pRtmpList->Unlock();
        		}
        	}

			// 清除rtmp_session -> user关系
			mRtmpUserMap.Erase(itr);
    	} else {
    		LogManager::GetLogManager()->Log(
    				LOG_MSG,
    				"FreeswitchClient::FreeswitchEventRtmpDestory( "
    				"tid : %d, "
    				"[Freeswitch, 事件处理, rtmp终端断开, 未登陆], "
    				"rtmp_session : %s "
    				")",
    				(int)syscall(SYS_gettid),
    				rtmp_session.c_str()
    				);
    	}
    	mRtmpSessionMap.Unlock();
    }
}

void FreeswitchClient::FreeswitchEventConferenceAddMember(const Json::Value& root) {
	string session = "";
	string user = "";

    string channelId = "";
    if( root["Channel-Call-UUID"].isString() ) {
    	channelId = root["Channel-Call-UUID"].asString();
    	session = GetSessionIdByUUID(channelId);
    	user = GetUserBySession(session);
    }
    string conference = "";
    if( root["Conference-Name"].isString() ) {
    	conference = root["Conference-Name"].asString();
    }
    string memberId = "";
    if( root["Member-ID"].isString() ) {
    	memberId = root["Member-ID"].asString();
    }
    string memberType = "";
    MemberType type = Member;
    if( root["Member-Type"].isString() ) {
    	memberType = root["Member-Type"].asString();
    	if( memberType == "moderator" ) {
    		type = Moderator;
    	}
    }

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"FreeswitchClient::FreeswitchEventConferenceAddMember( "
			"tid : %d, "
			"[Freeswitch, 事件处理, 增加会议成员], "
			"channelId: %s, "
			"conference : %s, "
			"user : %s, "
			"type : %d, "
			"memberId : %s, "
			"session : %s "
			")",
			(int)syscall(SYS_gettid),
			channelId.c_str(),
			conference.c_str(),
			user.c_str(),
			type,
			memberId.c_str(),
			session.c_str()
			);

	// 收到事件时候, 连接还没断开
	if( user.length() > 0 ) {
		// 插入channel
		Channel* channel = NULL;
		mRtmpChannel2UserMap.Lock();
		RtmpChannel2UserMap::iterator itr = mRtmpChannel2UserMap.Find(channelId);
		if( itr != mRtmpChannel2UserMap.End() ) {
			channel = itr->second;
			if( channel ) {
				channel->type = type;
				channel->memberId = memberId;
			}
		}
		mRtmpChannel2UserMap.Unlock();

		// Freeswitch只在一条线程处理消息, channel不会销毁
		if( channel != NULL ) {
	    	if( type == Moderator ) {
	    		if( mbIsRecording && conference.length() > 0 && channel->siteId.length() > 0 ) {
	        		// 开始录制视频
	        		StartRecordConference(conference, channel->siteId);
	    		}
	    	}

			// 回调用户进入聊天室
			if( mpFreeswitchClientListener ) {
				mpFreeswitchClientListener->OnFreeswitchEventConferenceAddMember(this, channel);
			}
		}

	} else {
		// 踢出用户
		char temp[1024] = {'\0'};
		string result = "";
		snprintf(temp, sizeof(temp), "api conference %s kick %s", conference.c_str(), memberId.c_str());
		if( SendCommandGetResult(temp, result) ) {
			LogManager::GetLogManager()->Log(
					LOG_MSG,
					"FreeswitchClient::FreeswitchEventConferenceAddMember( "
					"tid : %d, "
					"[Freeswitch, 事件处理, 增加会议成员, 踢出未登陆用户] "
					"channelId: %s, "
					"conference : %s, "
					"memberId : %s "
					")",
					(int)syscall(SYS_gettid),
					channelId.c_str(),
					conference.c_str(),
					memberId.c_str()
					);
		}
	}

}

void FreeswitchClient::FreeswitchEventConferenceDelMember(const Json::Value& root) {
    string channelId = "";
    if( root["Channel-Call-UUID"].isString() ) {
    	channelId = root["Channel-Call-UUID"].asString();
    }

    string conference = "";
    if( root["Conference-Name"].isString() ) {
    	conference = root["Conference-Name"].asString();
    }

    string memberId = "";
    if( root["Member-ID"].isString() ) {
    	memberId = root["Member-ID"].asString();
    }

	if( channelId.length() > 0 ) {
		Channel* channel = NULL;

		mRtmpChannel2UserMap.Lock();
		RtmpChannel2UserMap::iterator itr = mRtmpChannel2UserMap.Find(channelId);
		if( itr != mRtmpChannel2UserMap.End() ) {
			// 清除<user,conference> -> channel关系
			channel = itr->second;
		}
		mRtmpChannel2UserMap.Unlock();

		if( channel ) {
			// 回调用户进入聊天室
			if( mpFreeswitchClientListener ) {
				mpFreeswitchClientListener->OnFreeswitchEventConferenceDelMember(this, channel);
			}
		}
	}

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"FreeswitchClient::FreeswitchEventConferenceDelMember( "
			"tid : %d, "
			"[Freeswitch, 事件处理, 删除会议成员], "
			"conference : %s, "
			"channelId: %s, "
			"memberId : %s "
			")",
			(int)syscall(SYS_gettid),
			channelId.c_str(),
			conference.c_str(),
			memberId.c_str()
			);

}

void FreeswitchClient::FreeswitchEventChannelCreate(const Json::Value& root) {
    string channelId = "";
    if( root["Channel-Call-UUID"].isString() ) {
    	channelId = root["Channel-Call-UUID"].asString();
    }

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"FreeswitchClient::FreeswitchEventChannelCreate( "
			"tid : %d, "
			"[Freeswitch, 事件处理, 创建频道], "
			"channelId: %s "
			")",
			(int)syscall(SYS_gettid),
			channelId.c_str()
			);

	// 插入channel
    Channel channel;
    CreateChannel(channelId, channel);

}

void FreeswitchClient::FreeswitchEventChannelDestroy(const Json::Value& root) {
    string channelId = "";
    if( root["Channel-Call-UUID"].isString() ) {
    	channelId = root["Channel-Call-UUID"].asString();
    }

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"FreeswitchClient::FreeswitchEventChannelDestroy( "
			"tid : %d, "
			"[Freeswitch, 事件处理, 销毁频道], "
			"channelId : %s "
			")",
			(int)syscall(SYS_gettid),
			channelId.c_str()
			);

    // 销毁频道
	if( channelId.length() > 0 ) {
		DestroyChannel(channelId);
	}

}
