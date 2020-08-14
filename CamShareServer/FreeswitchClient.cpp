/*
 * FreeswitchClient.cpp
 *
 *  Created on: 2016年3月17日
 *      Author: max
 */

#include "FreeswitchClient.h"

#include <common/LogManager.h>
#include <common/StringHandle.h>
#include <common/CommonFunc.h>
#include <common/Arithmetic.h>
#include <algorithm>

#define CHANNEL_CREATE "CHANNEL_CREATE"
#define CHANNEL_DESTROY "CHANNEL_DESTROY"

#define CONF_EVENT_MAINT "conference::maintenance"
#define CONF_EVENT_MEMBER_ADD_FUCTION "conference_member_add"
#define CONF_EVENT_MEMBER_DEL_FUCTION "conference_member_del"

#define RTMP_EVENT_LOGIN "rtmp::login"
#define RTMP_EVENT_LOGIN_FUNCTION "rtmp_session_login"
#define RTMP_EVENT_DISCONNECT "rtmp::disconnect"
#define RTMP_EVENT_DISCONNECT_FUNCTION "rtmp_real_session_destroy"

#define WS_EVENT_MAINT "ws::maintenance"
#define WS_EVENT_CONNECT_FUNCTION "ws_connect"
#define WS_EVENT_LOGIN_FUNCTION "ws_login"
#define WS_EVENT_DISCONNECT_FUNCTION "ws_disconnect"

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
				"event : [Freeswitch-event_socket_log], "
				"event : [%s] %s:%d %s() %s"
				")",
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
	mbIsConnected = false;
	mWebSocketUserCount = 0;
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
				LOG_WARNING,
				"FreeswitchClient::Proceed( "
				"event : [Freeswitch-连接成功] "
				")"
				);
		if( mpFreeswitchClientListener ) {
			mpFreeswitchClientListener->OnFreeswitchConnect(this);
		}

		mCmdMutex.lock();
		status = esl_events(&mFreeswitch, ESL_EVENT_TYPE_JSON,
				"CHANNEL_CREATE CHANNEL_DESTROY "
				"CUSTOM "
				CONF_EVENT_MAINT" "
				RTMP_EVENT_LOGIN" "RTMP_EVENT_DISCONNECT" "
				WS_EVENT_MAINT" "
				"");
		mCmdMutex.unlock();

		if( status == ESL_SUCCESS ) {
			LogManager::GetLogManager()->Log(
					LOG_WARNING,
					"FreeswitchClient::Proceed( "
					"event : [Freeswitch-事件监听成功] "
					")"
					);
			// 由于和监听freeswitch事件监听在同一线程, 不存在异步问题, 可以执行同步操作
			// 标记已经监听成功
			mConnectedMutex.lock();
			mbIsConnected = true;
			mConnectedMutex.unlock();

			// 同步rtmp_seesion
			bFlag = SyncRtmpSessionList();

			// 重新验证当前所有会议室用户, 并同步到内存
			if( bFlag ) {
				bFlag = AuthorizationAllConference(true);
			}

			esl_event_t *event = NULL;
			while( bFlag ) {
				mCmdMutex.lock();
				status = esl_recv_event_timed(&mFreeswitch, 100, 1, &event);
				mCmdMutex.unlock();
				if( status == ESL_SUCCESS ) {
					if( event != NULL ) {
						FreeswitchEventHandle(event);
						esl_event_safe_destroy(&event);
					}
				} else if( status == ESL_BREAK ) {
					usleep(100 * 1000);
					continue;
				} else {
					LogManager::GetLogManager()->Log(
							LOG_ERR_USER,
							"FreeswitchClient::Proceed( "
							"event : [Freeswitch-连接断开] "
							")"
							);
					esl_disconnect(&mFreeswitch);
					break;
				}
			}

		} else {
			LogManager::GetLogManager()->Log(
					LOG_ERR_USER,
					"FreeswitchClient::Proceed( "
					"event : [Freeswitch-事件监听失败] "
					")"
					);

			esl_disconnect(&mFreeswitch);
		}

	} else {
		LogManager::GetLogManager()->Log(
				LOG_ERR_USER,
				"FreeswitchClient::Proceed( "
				"event : [Freeswitch-连接失败] "
				")"
				);
		esl_disconnect(&mFreeswitch);
	}

	// 标记已经断开
	mConnectedMutex.lock();
	mbIsConnected = false;

	// 清空通话
	mRtmpChannel2UserMap.Lock();
	mRtmpChannel2UserMap.Clear();
	mRtmpChannel2UserMap.Unlock();

	// 清空Rtmp在线用户
	mRtmpSessionMap.Lock();
	mRtmpSessionMap.Clear();
	for(RtmpSessionMap::iterator itr = mRtmpSessionMap.Begin(); itr != mRtmpSessionMap.End();) {
		RtmpObjectList* list = itr->second;
		delete list;
		mRtmpSessionMap.Erase(itr++);
	}
	mRtmpSessionMap.Clear();
	mRtmpUserMap.Clear();
	mRtmpSessionMap.Unlock();

	// 清空WebSocket登录数
	mWebSocketUserCount = 0;

	mConnectedMutex.unlock();

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
			"event : [Freeswitch-检查会议室用户], "
			"user : %s, "
			"conference : %s, "
			"exceptMemberId : %s "
			")",
			user.c_str(),
			conference.c_str(),
			exceptMemberId.c_str()
			);

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
						LOG_WARNING,
						"FreeswitchClient::KickUserFromConference( "
						"event : [Freeswitch-检查会议室用户-从会议室踢出用户], "
						"user : %s, "
						"conference : %s, "
						"exceptMemberId : %s "
						")",
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
				"event : [Freeswitch-检查会议室用户-会议室不存在需要踢出的用户], "
				"user : %s, "
				"conference : %s, "
				"exceptMemberId : %s "
				")",
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
			"event : [Freeswitch-用户开始观看会议室视频], "
			"user : %s, "
			"conference : %s, "
			"type : %d "
			")",
			user.c_str(),
			conference.c_str(),
			type
			);

	list<string> memberIds = GetMemberIdByUserFromConference(user, conference);
	for(list<string>::iterator itr = memberIds.begin(); itr != memberIds.end(); itr++) {
		string memberId = *itr;
		if( memberId.length() > 0 ) {
			// 允许用户开始观看会议室视频
			char temp[1024] = {'\0'};
			string result = "";
			snprintf(temp, sizeof(temp), "api conference %s relate %s %s clear", conference.c_str(), memberId.c_str(), memberId.c_str());
			if( SendCommandGetResult(temp, result) ) {
				LogManager::GetLogManager()->Log(
						LOG_MSG,
						"FreeswitchClient::StartUserRecvVideo( "
						"event : [Freeswitch-用户开始观看会议室视频-还原权限], "
						"user : %s, "
						"conference : %s, "
						"type : %d "
						")",
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
							LOG_WARNING,
							"FreeswitchClient::StartUserRecvVideo( "
							"event : [Freeswitch-用户开始观看会议室视频-普通成员], "
							"user : %s, "
							"conference : %s, "
							"type : %d "
							")",
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
								LOG_WARNING,
								"FreeswitchClient::StartUserRecvVideo( "
								"event : [Freeswitch-用户开始观看会议室视频-主持人], "
								"user : %s, "
								"conference : %s, "
								"type : %d "
								")",
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
				LOG_ERR_USER,
				"FreeswitchClient::StartUserRecvVideo( "
				"event : [Freeswitch-允许用户开始观看会议室视频-失败], "
				"user : %s, "
				"conference : %s, "
				"type : %d "
				")",
				user.c_str(),
				conference.c_str(),
				type
				);
	}

	return bFlag;
}

bool FreeswitchClient::SendCommandGetResult(const string& command, string& result) {
	bool bFlag = false;

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"FreeswitchClient::SendCommandGetResult( "
			"event : [Freeswitch-发送命令], "
			"command : %s "
			")",
			command.c_str()
			);

	mCmdMutex.lock();
	esl_event_t* event = NULL;
	esl_status_t status = esl_send_recv(&mFreeswitch, command.c_str());
	if( status == ESL_SUCCESS ) {
		bFlag = true;
		LogManager::GetLogManager()->Log(
				LOG_STAT,
				"FreeswitchClient::SendCommandGetResult( "
				"event : [Freeswitch-发送命令-成功] "
				")"
				);

		event = mFreeswitch.last_sr_event;
		if( event != NULL && event->body != NULL ) {
			result = event->body;
		} else if( mFreeswitch.last_sr_reply != NULL ){
			result = mFreeswitch.last_sr_reply;
		}

		LogManager::GetLogManager()->Log(
				LOG_STAT,
				"FreeswitchClient::SendCommandGetResult( "
				"event : [Freeswitch-发送命令-命令返回], "
				"result : \n%s\n"
				")",
				result.c_str()
				);

	}
	mCmdMutex.unlock();

	if( !bFlag ) {
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"FreeswitchClient::SendCommandGetResult( "
				"event : [Freeswitch-发送命令-失败], "
				"command : %s "
				")",
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
			"event : [Freeswitch-同步所有用户rtmp_session] "
			")"
			);

	RtmpObjectList rtmpObjectList;

	// 获取rtmp sessions
	char temp[1024] = {'\0'};
	string result = "";
	snprintf(temp, sizeof(temp), "api rtmp status profile default sessions");
	if( SendCommandGetResult(temp, result) ) {
		LogManager::GetLogManager()->Log(
				LOG_STAT,
				"FreeswitchClient::SyncRtmpSessionList( "
				"event : [Freeswitch-同步所有用户rtmp_session-获取列表命令-成功] "
				")"
				);

		bFlag = true;
	}

	if( bFlag ) {
		bFlag = false;

		string::size_type index = 0;
		string::size_type nextIndex = string::npos;

		// 头解析
		string header = StringHandle::findFirstString(result, "uuid,address,user,domain,site,flashVer,state\n", 0, nextIndex);
		if( nextIndex != string::npos ) {
			string sessions = result.substr(nextIndex, result.length() - nextIndex);
			LogManager::GetLogManager()->Log(
					LOG_STAT,
					"FreeswitchClient::SyncRtmpSessionList( "
					"event : [Freeswitch-同步所有用户rtmp_session-头解析], "
					"sessions : \n%s\n"
					")",
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
							"event : [Freeswitch-同步所有用户rtmp_session-行解析], "
							"sessionLine : %s "
							")",
							sessionLine.c_str()
							);

					// 切换下一个
					index = nextIndex;

					// 列解析
					string rtmp_session = "";
					string user = "";
					string siteId = "";
					string status = "";
					ParseSessionInfo(sessionLine, rtmp_session, user, siteId, status);

					if( rtmp_session.length() > 0 && user.length() > 0 && siteId.length() > 0 && status.length() > 0 ) {
						if( status == "ESTABLISHED" ) {
							// 插入在线用户列表
							LogManager::GetLogManager()->Log(
									LOG_WARNING,
									"FreeswitchClient::SyncRtmpSessionList( "
									"event : [Freeswitch-同步所有用户rtmp_session-插入在线用户列表], "
									"user : %s, "
									"siteId : %s, "
									"rtmp_session : %s "
									")",
									user.c_str(),
									siteId.c_str(),
									rtmp_session.c_str()
									);

				    		RtmpObject rtmpObj;
				    		rtmpObj.user = user;
				    		rtmpObj.siteId = siteId;
				    		rtmpObj.rtmpSession = rtmp_session;

				    		// 插入 user -> session
							mRtmpSessionMap.Lock();
							RtmpSessionMap::iterator itr = mRtmpSessionMap.Find(user);
					    	if( itr == mRtmpSessionMap.End() ) {
					    		// 插入新的用户
					    		RtmpObjectList* pRtmpObjectList = new RtmpObjectList();
					    		pRtmpObjectList->PushBack(rtmpObj);
					    		mRtmpSessionMap.Insert(user, pRtmpObjectList);

					    	} else {
					    		// 加入用户的rtmp session列表
					    		RtmpObjectList* pRtmpObjectList = itr->second;
					    		pRtmpObjectList->PushBack(rtmpObj);

					    	}

					    	// 插入 session -> user
					    	RtmpUserMap::iterator itr2 = mRtmpUserMap.Find(rtmp_session);
					    	if( itr2 == mRtmpUserMap.End() ) {
					    		mRtmpUserMap.Insert(rtmp_session, user);
					    	} else {
					    		itr2->second = user;
					    	}

							mRtmpSessionMap.Unlock();

							// 插入连接列表
							rtmpObjectList.PushBack(rtmpObj);

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
				"event : [Freeswitch-同步所有用户rtmp_session-失败] "
				")"
				);
	} else {
		if( mpFreeswitchClientListener ) {
			mpFreeswitchClientListener->OnFreeswitchEventOnlineList(this, rtmpObjectList);
		}
	}

	return bFlag;
}

bool FreeswitchClient::ParseSessionInfo(
		const string& session,
		string& uuid,
		string& user,
		string& siteId,
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
				"event : [Freeswitch-解析单个rtmp_session-列解析], "
				"column : %s "
				")",
				column.c_str()
				);

		if( nextIndex != string::npos ) {
			index = nextIndex;

		} else {
			// 最后一个
			if( index < session.length() ) {
				column = session.substr(index, session.length() - index);

				// status
				status = column;
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
		case 4:{
			// site
			siteId = column;
		}break;
		case 6:{
//			// status
//			status = column;
		}break;
		default:break;
		}

		if( uuid.length() > 0 && user.length() > 0 && siteId.length() > 0 && status.length() > 0 ) {
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
			"event : [Freeswitch-解析单个rtmp_session-列解析], "
			"uuid : %s, "
			"user : %s, "
			"siteId : %s, "
			"status : %s "
			")",
			uuid.c_str(),
			user.c_str(),
			siteId.c_str(),
			status.c_str()
			);

	return bFlag;
}

bool FreeswitchClient::AuthorizationAllConference(bool bCreateChannel) {
	bool bFlag = false;

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"FreeswitchClient::AuthorizationAllConference( "
			"event : [Freeswitch-重新验证当前所有会议室用户], "
			"bCreateChannel : %d "
			")",
			bCreateChannel
			);

	if( mFreeswitch.connected != 1 ) {
		// 未连接
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"FreeswitchClient::AuthorizationAllConference( "
				"event : [Freeswitch-重新验证当前所有会议室用户-未连接成功-跳过], "
				"bCreateChannel : %d "
				")",
				bCreateChannel
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
				"event : [Freeswitch-重新验证当前所有会议室用户-获取会议室列表], "
				"bCreateChannel : %d "
				")",
				bCreateChannel
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
				TiXmlHandle conferenceHandle = handle.FirstChild( "conferences" ).Child( "conference", i++ );
				if ( conferenceHandle.ToNode() == NULL ) {
					// 没有更多会议室
					LogManager::GetLogManager()->Log(
							LOG_MSG,
							"FreeswitchClient::AuthorizationAllConference( "
							"event : [Freeswitch-重新验证当前所有会议室用户-成功], "
							"bCreateChannel : %d "
							")",
							bCreateChannel
							);
					break;
				}

				TiXmlElement* conferenceElement = conferenceHandle.ToElement();
				if( conferenceElement ) {
					conferenceName = conferenceElement->Attribute("name");
					LogManager::GetLogManager()->Log(
							LOG_STAT,
							"FreeswitchClient::AuthorizationAllConference( "
							"event : [Freeswitch-重新验证当前所有会议室用户-获取到会议室], "
							"bCreateChannel : %d, "
							"conferenceName : %s "
							")",
							bCreateChannel,
							conferenceName.c_str()
							);

					string sessionId = "";
					string uuId = "";
					string user = "";
					string memberId = "";
					MemberType memberType = Member;
					int j = 0;

					while( true ) {
						TiXmlHandle memberHandle = conferenceHandle.FirstChild( "members" ).Child( "member", j++ );
						if ( memberHandle.ToNode() == NULL ) {
							// 没有更多会员
							LogManager::GetLogManager()->Log(
									LOG_STAT,
									"FreeswitchClient::AuthorizationAllConference( "
									"event : [Freeswitch-重新验证当前所有会议室用户-没有更多会员], "
									"bCreateChannel : %d, "
									"conferenceName : %s "
									")",
									bCreateChannel,
									conferenceName.c_str()
									);
							break;
						}

						const char* type = memberHandle.ToElement()->Attribute("type");
						LogManager::GetLogManager()->Log(
								LOG_STAT,
								"FreeswitchClient::AuthorizationAllConference( "
								"event : [Freeswitch-重新验证当前所有会议室用户-获取到会员], "
								"bCreateChannel : %d, "
								"conferenceName : %s, "
								"type : %s "
								")",
								bCreateChannel,
								conferenceName.c_str(),
								type
								);

						if( strcmp(type, "caller") == 0 ) {
							TiXmlElement* uuidElement = memberHandle.FirstChild("uuid").ToElement();
							if( uuidElement ) {
								uuId = uuidElement->GetText();
								user = GetUserByUUID(uuId);
							}

							TiXmlElement* memberIdElement = memberHandle.FirstChild("id").ToElement();
							if( memberIdElement ) {
								memberId = memberIdElement->GetText();
							}

							TiXmlElement* moderatorElement = memberHandle.FirstChild("flags").FirstChild("is_moderator").ToElement();
							if( moderatorElement ) {
								if( strcmp(moderatorElement->GetText(), "true") == 0 ) {
									memberType = Moderator;
								}
							}

							LogManager::GetLogManager()->Log(
									LOG_STAT,
									"FreeswitchClient::AuthorizationAllConference( "
									"event : [Freeswitch-重新验证当前所有会议室用户-获取到会员属性], "
									"bCreateChannel : %d, "
									"conferenceName : %s, "
									"uuId : %s, "
									"user : %s, "
									"memberId : %s, "
									"memberType : %d "
									")",
									bCreateChannel,
									conferenceName.c_str(),
									uuId.c_str(),
									user.c_str(),
									memberId.c_str(),
									memberType
									);

							if( user.length() > 0 && conferenceName.length() > 0 ) {
								string serverId = GetChannelParam(uuId, "serverId");
								string siteId = GetChannelParam(uuId, "siteId");
								string chatTypeString = GetChannelParam(uuId, "chat_type_string");
								ENTERCONFERENCETYPE chatType = GetChatTypeWithChannelVar(chatTypeString);
								// 获取rtmp_session
								string rtmp_session = GetChannelParam(uuId, "rtmp_session");
								if (bCreateChannel) {
									// 根据会议室更新频道信息
									UpdateChannelWithConference(uuId, user, conferenceName, memberType, memberId, rtmp_session, serverId, siteId, chatType);
								}

								LogManager::GetLogManager()->Log(
										LOG_WARNING,
										"FreeswitchClient::AuthorizationAllConference( "
										"event : [Freeswitch-重新验证当前所有会议室用户-验证用户], "
										"bCreateChannel : %d, "
										"user : %s, "
										"conference : %s, "
										"memberType : %s, "
										"memberId : %s, "
										"channelId : %s, "
										"serverId : %s, "
										"siteId : %s, "
										"chatTypeString : %s, "
										"chatType : %s "
										")",
										bCreateChannel,
										user.c_str(),
										conferenceName.c_str(),
										(memberType == Moderator)?"Moderator":"Member",
										memberId.c_str(),
										uuId.c_str(),
										serverId.c_str(),
										siteId.c_str(),
										chatTypeString.c_str(),
										(chatType == ENTERCONFERENCETYPE_CAMSHARE)?"C":"V"
										);

							    // 回调重新验证会议室用户
								if( mpFreeswitchClientListener ) {
									Channel channel(uuId, user, conferenceName, memberType, memberId, rtmp_session, serverId, siteId, chatType);
									mpFreeswitchClientListener->OnFreeswitchEventConferenceAuthorizationMember(this, &channel);
								}
							}
						}

					}
					// 获取会议室成员结束
				}
			}
			// 获取会议室结束
		} else {
			// XML解析错误
			LogManager::GetLogManager()->Log(
					LOG_ERR_USER,
					"FreeswitchClient::AuthorizationAllConference( "
					"event : [Freeswitch-重新验证当前所有会议室用户-XML解析错误], "
					"errorId : %d, "
					"errorDesc : %s, "
					"errorRow : %d, "
					"errorCol : %d "
					")",
					doc.ErrorId(),
					doc.ErrorDesc(),
					doc.ErrorRow(),
					doc.ErrorCol()
					);
			bFlag = false;
		}
	}

	if( !bFlag ) {
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"FreeswitchClient::AuthorizationAllConference( "
				"event : [Freeswitch-重新验证当前所有会议室用户-失败] "
				")"
				);
	}

	return bFlag;
}

bool FreeswitchClient::UpdateChannelWithConference(
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
	bool bFlag = false;

	mConnectedMutex.lock();
	if( mbIsConnected ) {
		LogManager::GetLogManager()->Log(
				LOG_STAT,
				"FreeswitchClient::UpdateChannelWithConference( "
				"event : [Freeswitch-根据会议室信息更新频道信息], "
				"user : %s, "
				"conference : %s, "
				"type : %s, "
				"memberId : %s, "
				"channelId : %s, "
				"rtmp_session : %s, "
				"serverId : %s, "
				"siteId : %s, "
				"chatType : %d, "
				")",
				user.c_str(),
				conference.c_str(),
				(type == Moderator)?"Moderator":"Member",
				memberId.c_str(),
				channelId.c_str(),
				rtmp_session.c_str(),
				serverId.c_str(),
				siteId.c_str(),
				chatType
				);

		mRtmpChannel2UserMap.Lock();
		// 更新信息
		string uuid = GetChannelParam(channelId, "uuid");
		if ( uuid == channelId ) {
			Channel* channel = CreateChannel(channelId);
			channel->user = user;
			channel->conference = conference;
			channel->serverId = serverId;
			channel->siteId = siteId;
			channel->chatType = chatType;
			bFlag = true;
		}
		mRtmpChannel2UserMap.Unlock();
	} else {
		LogManager::GetLogManager()->Log(
				LOG_WARNING,
				"FreeswitchClient::UpdateChannelWithConference( "
				"event : [Freeswitch-根据会议室信息更新频道信息-还没连接], "
				"channelId : %s "
				")",
				channelId.c_str()
				);
	}
	mConnectedMutex.unlock();

	return bFlag;
}

bool FreeswitchClient::UpdateChannelWithDialplan(
		const string& channelId,
		const string& user,
		const string& conference,
		const string& serverId,
		const string& siteId,
		const string& chat_type_string
		) {
	bool bFlag = false;

	mConnectedMutex.lock();
	if( mbIsConnected ) {
		LogManager::GetLogManager()->Log(
				LOG_STAT,
				"FreeswitchClient::UpdateChannelWithDialplan( "
				"event : [Freeswitch-根据拨号计划更新频道信息], "
				"user : %s, "
				"conference : %s, "
				"channelId : %s, "
				"serverId : %s, "
				"siteId : %s, "
				"chat_type_string : %s "
				")",
				user.c_str(),
				conference.c_str(),
				channelId.c_str(),
				serverId.c_str(),
				siteId.c_str(),
				chat_type_string.c_str()
				);

		mRtmpChannel2UserMap.Lock();
		// 更新信息
		string uuid = GetChannelParam(channelId, "uuid");
		if ( uuid == channelId ) {
			Channel* channel = CreateChannel(channelId);
			channel->user = user;
			channel->conference = conference;
			channel->serverId = serverId;
			channel->siteId = siteId;
			if ( chat_type_string.length() > 0 ) {
				channel->chatType = GetChatTypeWithChannelVar(chat_type_string);
			}
		} else {
			LogManager::GetLogManager()->Log(
					LOG_WARNING,
					"FreeswitchClient::UpdateChannelWithDialplan( "
					"event : [Freeswitch-根据拨号计划更新频道信息-等待频道创建], "
					"user : %s, "
					"conference : %s, "
					"uuid : %s, "
					"channelId : %s, "
					"serverId : %s, "
					"siteId : %s, "
					"chat_type_string : %s "
					")",
					user.c_str(),
					conference.c_str(),
					uuid.c_str(),
					channelId.c_str(),
					serverId.c_str(),
					siteId.c_str(),
					chat_type_string.c_str()
					);
		}
		mRtmpChannel2UserMap.Unlock();

		bFlag = true;

	} else {
		LogManager::GetLogManager()->Log(
				LOG_WARNING,
				"FreeswitchClient::UpdateChannelWithDialplan( "
				"event : [Freeswitch-根据拨号计划更新频道信息-还没连接], "
				"channelId : %s "
				")",
				channelId.c_str()
				);
	}
	mConnectedMutex.unlock();

	return bFlag;
}

unsigned int FreeswitchClient::GetChannelCount() {
	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"FreeswitchClient::GetChannelCount( "
			"event : [Freeswitch-获取通话数目] "
			")"
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
			"event : [Freeswitch-获取在线用户数目] "
			")"
			);
	unsigned int count = 0;
	mRtmpSessionMap.Lock();
	count = mRtmpSessionMap.Size();
	mRtmpSessionMap.Unlock();
	return count;
}

unsigned int FreeswitchClient::GetRTMPSessionCount() {
	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"FreeswitchClient::GetRTMPSessionCount( "
			"event : [Freeswitch-获取在线RTMP_Session数目] "
			")"
			);
	unsigned int count = 0;
	mRtmpSessionMap.Lock();
	count = mRtmpUserMap.Size();
	mRtmpSessionMap.Unlock();
	return count;
}

unsigned int FreeswitchClient::GetWebSocketUserCount() {
	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"FreeswitchClient::GetWebSocketUserCount( "
			"event : [Freeswitch-获取在线WebSocket用户数目] "
			")"
			);
	return mWebSocketUserCount;
}

bool FreeswitchClient::GetConferenceUserList(
		const string& conference,
		list<string>& userList
		) {
	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"FreeswitchClient::GetConferenceUserList( "
			"event : [Freeswitch-获取指定会议室用户列表], "
			"conference : %s "
			")",
			conference.c_str()
			);
	bool bFlag = false;

	string result = "";
	if( conference.length() > 0 ) {
		char temp[1024] = {'\0'};
		snprintf(temp, sizeof(temp), "api conference %s xml_list", conference.c_str());
		if( SendCommandGetResult(temp, result) ) {
			bFlag = true;
		}
	}

	if( bFlag ) {
		TiXmlDocument doc;
		doc.Parse(result.c_str());
		if ( !doc.Error() ) {
			int i = 0;
			string conferenceName = "";

			TiXmlHandle handle( &doc );
			while ( true ) {
				TiXmlHandle conferenceHandle = handle.FirstChild( "conferences" ).Child( "conference", i++ );
				if ( conferenceHandle.ToNode() == NULL ) {
					// 没有更多会议室
					LogManager::GetLogManager()->Log(
							LOG_MSG,
							"FreeswitchClient::GetConferenceUserList( "
							"event : [Freeswitch-获取指定会议室用户列表-成功] "
							")"
							);
					break;
				}

				TiXmlElement* conferenceElement = conferenceHandle.ToElement();
				if( conferenceElement ) {
					conferenceName = conferenceElement->Attribute("name");
					LogManager::GetLogManager()->Log(
							LOG_STAT,
							"FreeswitchClient::GetConferenceUserList( "
							"event : [Freeswitch-获取指定会议室用户列表-获取到会议室], "
							"conference : %s "
							")",
							conferenceName.c_str()
							);

					string uuId = "";
					string user = "";
					int j = 0;

					while( true ) {
						TiXmlHandle memberHandle = conferenceHandle.FirstChild( "members" ).Child( "member", j++ );
						if ( memberHandle.ToNode() == NULL ) {
							// 没有更多会员
							LogManager::GetLogManager()->Log(
									LOG_STAT,
									"FreeswitchClient::GetConferenceUserList( "
									"event : [Freeswitch-获取指定会议室用户列表-没有更多会员], "
									"conference : %s "
									")",
									conferenceName.c_str()
									);
							break;
						}

						const char* type = memberHandle.ToElement()->Attribute("type");
						LogManager::GetLogManager()->Log(
								LOG_STAT,
								"FreeswitchClient::AuthorizationAllConference( "
								"event : [Freeswitch-获取指定会议室用户列表-获取到会员], "
								"conference : %s, "
								"type : %s "
								")",
								conferenceName.c_str(),
								type
								);

						if( strcmp(type, "caller") == 0 ) {
							TiXmlElement* uuidElement = memberHandle.FirstChild("uuid").ToElement();
							if( uuidElement ) {
								uuId = uuidElement->GetText();
								user = GetUserByUUID(uuId);
							}

							LogManager::GetLogManager()->Log(
									LOG_STAT,
									"FreeswitchClient::GetConferenceUserList( "
									"event : [Freeswitch-获取指定会议室用户列表-获取到用户], "
									"conferenceName : %s, "
									"user : %s, "
									"uuId : %s "
									")",
									conferenceName.c_str(),
									user.c_str(),
									uuId.c_str()
									);

							if( user.length() > 0 ) {
								// 加入到用户列表
								userList.push_back(user);
							}
						}

					}
					// 获取会议室成员结束
				}
			}
			// 获取会议室结束
		} else {
			// XML解析错误
			LogManager::GetLogManager()->Log(
					LOG_MSG,
					"FreeswitchClient::GetConferenceUserList( "
					"event : [Freeswitch-获取指定会议室用户列表-XML解析错误], "
					"conference : %s, "
					"errorId : %d, "
					"errorDesc : %s, "
					"errorRow : %d, "
					"errorCol : %d "
					")",
					conference.c_str(),
					doc.ErrorId(),
					doc.ErrorDesc(),
					doc.ErrorRow(),
					doc.ErrorCol()
					);
			bFlag = false;
		}
	}

	if( !bFlag ) {
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"FreeswitchClient::GetConferenceUserList( "
				"event : [Freeswitch-获取指定会议室用户列表-失败], "
				"conference : %s "
				")",
				conference.c_str()
				);
	}

	return bFlag;
}

Channel* FreeswitchClient::CreateChannel(
		const string& channelId
		) {
	Channel* channel = NULL;

	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"FreeswitchClient::CreateChannel( "
			"event : [Freeswitch-创建频道], "
			"channelId : %s "
			")",
			channelId.c_str()
			);

	// 插入channel
	RtmpChannel2UserMap::iterator itr = mRtmpChannel2UserMap.Find(channelId);
	if( itr != mRtmpChannel2UserMap.End() ) {
		channel = itr->second;
	} else {
		channel = new Channel();
		mRtmpChannel2UserMap.Insert(channelId, channel);
	}

	if ( channel ) {
		channel->channelId = channelId;
	}

	return channel;
}

void FreeswitchClient::DestroyChannel(
		const string& channelId
		) {

	// 删除channel
	mRtmpChannel2UserMap.Lock();
	RtmpChannel2UserMap::iterator itr = mRtmpChannel2UserMap.Find(channelId);
	if( itr != mRtmpChannel2UserMap.End() ) {
		LogManager::GetLogManager()->Log(
				LOG_STAT,
				"FreeswitchClient::DestroyChannel( "
				"event : [Freeswitch-销毁频道-找到对应频道], "
				"channelId : %s "
				")",
				channelId.c_str()
				);

		// 清除channel -> <user,conference> 关系
		mRtmpChannel2UserMap.Erase(itr);

		Channel* channel = itr->second;
		if( channel ) {
			delete channel;
		}

	} else {
		LogManager::GetLogManager()->Log(
				LOG_ERR_USER,
				"FreeswitchClient::DestroyChannel( "
				"event : [Freeswitch-销毁频道-找不到对应频道], "
				"channelId : %s "
				")",
				channelId.c_str()
				);
	}
	mRtmpChannel2UserMap.Unlock();
}

string FreeswitchClient::GetChannelParam(
		const string& uuid,
		const string& key
		) {
	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"FreeswitchClient::GetChannelParam( "
			"event : [Freeswitch-获取会话变量], "
			"uuid : %s, "
			"key : %s "
			")",
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
				"event : [Freeswitch-获取会话变量-成功], "
				"uuid : %s, "
				"key : %s, "
				"value : %s "
				")",
				uuid.c_str(),
				key.c_str(),
				result.c_str()
				);
	} else {
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"FreeswitchClient::GetChannelParam( "
				"event : [Freeswitch-获取会话变量-失败], "
				"uuid : %s, "
				"key : %s, "
				"value : %s "
				")",
				uuid.c_str(),
				key.c_str(),
				result.c_str()
				);
	}

	return result;
}

ENTERCONFERENCETYPE FreeswitchClient::GetEnterConferenceTypeWithString(const string& value) {
    ENTERCONFERENCETYPE type = ENTERCONFERENCETYPE_CAMSHARE;
    if (value.compare("c") == 0) {
        type = ENTERCONFERENCETYPE_CAMSHARE;
    } else if (value.compare("v") == 0) {
        type = ENTERCONFERENCETYPE_VIDEO;
    }
    return type;
}

ENTERCONFERENCETYPE FreeswitchClient::GetChatTypeWithChannelVar(const string& value) {
	ENTERCONFERENCETYPE type = ENTERCONFERENCETYPE_CAMSHARE;

    // Alex, variable_key_type 是websocket传过来的BASE64字符串（v+时间戳 或 c+时间戳），解BASE64后获取第一个字符
    const int bufferSize = 1024;
    char buffer[bufferSize] = {0};
    Arithmetic ari;
    ari.Base64Decode(value.c_str(), value.length(), buffer);
    string decodeString = buffer;
    string chatTypeString = decodeString.substr(0,1);
    // Alex, chatType全转小写
    transform(chatTypeString.begin(), chatTypeString.end(), chatTypeString.begin(), ::tolower);

    type = GetEnterConferenceTypeWithString(chatTypeString);

    return type;
}

string FreeswitchClient::GetUserByUUID(const string& uuId) {
	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"FreeswitchClient::GetUserByUUID( "
			"event : [Freeswitch-根据ChannelId获取用户名], "
			"uuId : %s "
			")",
			uuId.c_str()
			);

	// 获取rtmp_session
	char temp[1024] = {'\0'};
	string result = "";
	snprintf(temp, sizeof(temp), "api uuid_getvar %s caller", uuId.c_str());
	if( SendCommandGetResult(temp, result) ) {
		LogManager::GetLogManager()->Log(
				LOG_STAT,
				"FreeswitchClient::GetUserByUUID( "
				"event : [Freeswitch-根据ChannelId获取用户名-成功], "
				"uuId : %s, "
				"result : %s "
				")",
				uuId.c_str(),
				result.c_str()
				);
	} else {
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"FreeswitchClient::GetUserByUUID( "
				"event : [Freeswitch-根据ChannelId获取用户名-失败], "
				"uuId : %s, "
				"result : %s "
				")",
				uuId.c_str(),
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

	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"FreeswitchClient::GetMemberIdByUserFromConference( "
			"event : [Freeswitch-在会议室获取用户member_id], "
			"user : %s, "
			"conference : %s "
			")",
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
				"event : [Freeswitch-在会议室获取用户member_id-获取会议会员列表-成功] "
				")"
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

					// 获取用户名
					mRtmpChannel2UserMap.Lock();
					RtmpChannel2UserMap::iterator itr = mRtmpChannel2UserMap.Find(uuId);
					Channel* channel = NULL;
					if( itr != mRtmpChannel2UserMap.End() ) {
						channel = itr->second;
						if( channel->user == user && channel->conference == conference ) {
							LogManager::GetLogManager()->Log(
									LOG_STAT,
									"FreeswitchClient::GetMemberIdByUserFromConference( "
									"event : [Freeswitch-在会议室获取用户member_id-找到对应channel], "
									"user : %s, "
									"conference : %s, "
									"channelId : %s "
									")",
									user.c_str(),
									conference.c_str(),
									uuId.c_str()
									);

							TiXmlElement* xmlMemberId = memberHandle.FirstChild("id").ToElement();
							if( xmlMemberId ) {
								memberId = xmlMemberId->GetText();
								memberIds.push_back(memberId);
								bFlag = true;
							}
						}
					}
					mRtmpChannel2UserMap.Unlock();
				}

				i++;
			}
		}
	}

	if( !bFlag ) {
		LogManager::GetLogManager()->Log(
				LOG_STAT,
				"FreeswitchClient::GetMemberIdByUserFromConference( "
				"event : [Freeswitch-在会议室获取用户member_id-失败], "
				"user : %s, "
				"conference : %s "
				")",
				user.c_str(),
				conference.c_str()
				);
	}

	return memberIds;
}

bool FreeswitchClient::StartRecordConference(
		const string& conference,
		const string& siteId,
		string& outFilePath
		) {
	bool bFlag = false;

	string result = "";

	// get current time
	time_t stm = time(NULL);
	struct tm tTime;
	localtime_r(&stm, &tTime);

	// get directory
	char dirPath[255] = {'\0'};
	snprintf(dirPath, sizeof(dirPath), "%s/%02d/%02d/%02d",
			mRecordingPath.c_str(),
			tTime.tm_year + 1900,
			tTime.tm_mon + 1,
			tTime.tm_mday
			);
	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"FreeswitchClient::StartRecordConference( "
			"event : [Freeswitch-开始录制会议视频], "
			"conference : %s, "
			"siteId : %s, "
			"dirPath : %s "
			")",
			conference.c_str(),
			siteId.c_str(),
			dirPath
			);
	if( MakeDir(dirPath) ) {
		// get file path
		char timeBuffer[64] = {'\0'};
		snprintf(timeBuffer, 64, "%d%02d%02d%02d%02d%02d", tTime.tm_year + 1900, tTime.tm_mon + 1, tTime.tm_mday, tTime.tm_hour, tTime.tm_min, tTime.tm_sec);

		char filePath[256] = {'\0'};
		snprintf(filePath, sizeof(filePath), "%s/%s_%s_%s.flv",
				dirPath,
				conference.c_str(),
				siteId.c_str(),
				timeBuffer
				);
		outFilePath = filePath;

		LogManager::GetLogManager()->Log(
				LOG_WARNING,
				"FreeswitchClient::StartRecordConference( "
				"event : [Freeswitch-开始录制会议视频], "
				"conference : %s, "
				"siteId : %s, "
				"outFilePath : %s "
				")",
				conference.c_str(),
				siteId.c_str(),
				outFilePath.c_str()
				);

		char temp[2048] = {'\0'};
		snprintf(temp, sizeof(temp), "api conference %s record %s",
				conference.c_str(),
				outFilePath.c_str()
				);

		if( SendCommandGetResult(temp, result) ) {
			LogManager::GetLogManager()->Log(
					LOG_MSG,
					"FreeswitchClient::StartRecordConference( "
					"event : [Freeswitch-开始录制会议视频-成功], "
					"conference : %s, "
					"siteId : %s, "
					"outFilePath : %s "
					")",
					conference.c_str(),
					siteId.c_str(),
					outFilePath.c_str()
					);
			bFlag = true;
		}
	}

	if( !bFlag ) {
		LogManager::GetLogManager()->Log(
				LOG_ERR_USER,
				"FreeswitchClient::StartRecordConference( "
				"event : [Freeswitch-开始录制会议视频-失败], "
				"conference : %s, "
				"siteId : %s "
				"outFilePath : %s "
				")",
				conference.c_str(),
				siteId.c_str(),
				outFilePath.c_str()
				);
	}

	return bFlag;
}

bool FreeswitchClient::StopRecordConference(const string& conference, string& filePath) {
	bool bFlag = false;

	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"FreeswitchClient::StopRecordConference( "
			"event : [Freeswitch-停止录制会议视频], "
			"conference : %s, "
			"filePath : %s "
			")",
			conference.c_str(),
			filePath.c_str()
			);

	char temp[1024] = {'\0'};
	string result = "";
	snprintf(temp, sizeof(temp), "api conference %s recording stop %s",
			conference.c_str(),
			filePath.c_str()
			);

	if( SendCommandGetResult(temp, result) ) {
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"FreeswitchClient::StopRecordConference( "
				"event : [Freeswitch-停止录制会议视频-成功], "
				"conference : %s, "
				"filePath : %s "
				")",
				conference.c_str(),
				filePath.c_str()
				);
		bFlag = true;
	}

	if( !bFlag ) {
		LogManager::GetLogManager()->Log(
				LOG_ERR_USER,
				"FreeswitchClient::StopRecordConference( "
				"event : [Freeswitch-停止录制会议视频-失败], "
				"conference : %s, "
				"filePath : %s "
				")",
				conference.c_str(),
				filePath.c_str()
				);
	}

	return bFlag;
}

bool FreeswitchClient::StartRecordChannel(const string& uuid) {
	bool bFlag = false;
	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"FreeswitchClient::StartRecordChannel( "
			"event : [Freeswitch-开始录制频道视频], "
			"uuid : %s "
			")",
			uuid.c_str()
			);

	char temp[1024] = {'\0'};
	string result = "";
	snprintf(temp, sizeof(temp), "api uuid_setvar %s enable_file_write_buffering false", uuid.c_str());
	if( SendCommandGetResult(temp, result) ) {
		LogManager::GetLogManager()->Log(
				LOG_STAT,
				"FreeswitchClient::StartRecordChannel( "
				"event : [Freeswitch-开始录制频道视频-设置不使用buffer], "
				"uuid : %s "
				")",
				uuid.c_str()
				);
		bFlag = true;
	}

	if( bFlag ) {
		bFlag = false;

		// 获取用户名
		string user = "";
		mRtmpChannel2UserMap.Lock();
		RtmpChannel2UserMap::iterator itr = mRtmpChannel2UserMap.Find(uuid);
		Channel* channel = NULL;
		if( itr != mRtmpChannel2UserMap.End() ) {
			channel = itr->second;
			user = channel->user;
		}
		mRtmpChannel2UserMap.Unlock();

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
					"event : [Freeswitch-开始录制频道视频-成功], "
					"uuid : %s "
					")",
					uuid.c_str()
					);
			bFlag = true;
		}
	}

	if( !bFlag ) {
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"FreeswitchClient::StartRecordChannel( "
				"event : [Freeswitch-开始录制频道视频-失败], "
				"uuid : %s "
				")",
				uuid.c_str()
				);
	}

	return bFlag;
}

void FreeswitchClient::FreeswitchEventHandle(esl_event_t *event) {
//	LogManager::GetLogManager()->Log(
//			LOG_STAT,
//			"FreeswitchClient::FreeswitchEventHandle( "
//			"event : [Freeswitch-事件处理], "
//			"body : \n%s\n"
//			")",
//			event->body
//			);

	Json::Value root;
	Json::Reader reader;
	if( event->body != NULL && reader.parse(event->body, root, false) ) {
		if( root.isObject() && root["Event-Name"].isString() ) {
			string eventName = root["Event-Name"].asString();
			LogManager::GetLogManager()->Log(
					LOG_STAT,
					"FreeswitchClient::FreeswitchEventHandle( "
					"event : [Freeswitch-事件处理], "
					"event : %s, "
					"body : \n%s\n"
					")",
					eventName.c_str(),
					event->body
					);

			if( eventName == "CUSTOM" ) {
				// 自定义模块事件
				if( root["Event-Calling-Function"].isString() ) {
					string function = root["Event-Calling-Function"].asString();
					LogManager::GetLogManager()->Log(
							LOG_STAT,
							"FreeswitchClient::FreeswitchEventHandle( "
							"event : [Freeswitch-事件处理-自定义模块事件], "
							"function : %s "
							")",
							function.c_str()
							);

					if( function == RTMP_EVENT_LOGIN_FUNCTION ) {
						// rtmp登录成功
						FreeswitchEventRtmpLogin(root);

					} else if( function == RTMP_EVENT_DISCONNECT_FUNCTION ) {
						// rtmp销毁成功
						FreeswitchEventRtmpDestory(root);

					} else if( function == WS_EVENT_CONNECT_FUNCTION ) {
						// websocket连接成功
						FreeswitchEventWebsocketConnect(root);

					} else if( function == WS_EVENT_LOGIN_FUNCTION ) {
						// websocket登录成功
						FreeswitchEventWebsocketLogin(root);

					} else if( function == WS_EVENT_DISCONNECT_FUNCTION ) {
						// websocket销毁成功
						FreeswitchEventWebsocketDestory(root);

					} else if( function == CONF_EVENT_MEMBER_ADD_FUCTION ) {
						// 增加会议成员
						FreeswitchEventConferenceAddMember(root);

					} else if( function == CONF_EVENT_MEMBER_DEL_FUCTION ) {
						// 删除会议成员
						FreeswitchEventConferenceDelMember(root);
					}
	 			}
			} else {
				// 标准事件
				LogManager::GetLogManager()->Log(
						LOG_STAT,
						"FreeswitchClient::FreeswitchEventHandle( "
						"event : [Freeswitch-事件处理-标准事件], "
						"event : %s "
						")",
						eventName.c_str()
						);

				if( eventName == CHANNEL_CREATE ) {
					// 创建频道
					FreeswitchEventChannelCreate(root);

				} else if( eventName == CHANNEL_DESTROY ) {
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
	string siteId;
    if( root["Site"].isString() ) {
    	siteId = root["Site"].asString();
    }
    string rtmp_session = "";
    if( root["RTMP-Session-ID"].isString() ) {
    	rtmp_session = root["RTMP-Session-ID"].asString();
    }

	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"FreeswitchClient::FreeswitchEventRtmpLogin( "
			"event : [Freeswitch-事件处理-rtmp终端登录], "
			"user : %s, "
			"siteId : %s, "
			"rtmp_session : %s "
			")",
			user.c_str(),
			siteId.c_str(),
			rtmp_session.c_str()
			);

	// 用户是否改变状态
	bool bUserStatusChange = false;
	// 创建连接信息
	RtmpObject rtmpObject;
	// 插入在线用户列表
    if( user.length() > 0 && rtmp_session.length() > 0 ) {
    	mRtmpSessionMap.Lock();

    	// 更新连接信息
    	rtmpObject.user = user;
    	rtmpObject.siteId = siteId;
    	rtmpObject.rtmpSession = rtmp_session;

    	// 插入 user -> sessiond
    	RtmpSessionMap::iterator itr = mRtmpSessionMap.Find(user);
    	if( itr == mRtmpSessionMap.End() ) {
    		// 插入新的用户
    		RtmpObjectList* pRtmpObjectList = new RtmpObjectList();
    		pRtmpObjectList->PushBack(rtmpObject);
    		mRtmpSessionMap.Insert(user, pRtmpObjectList);

    		bUserStatusChange = true;

    	} else {
    		// 加入用户的rtmp session列表
    		RtmpObjectList* pRtmpObjectList = itr->second;
    		pRtmpObjectList->PushBack(rtmpObject);
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

    // 通知用户上线
	if(mpFreeswitchClientListener) {
		mpFreeswitchClientListener->OnFreeswitchEventOnlineStatus(this, rtmpObject, true);
	}

}

void FreeswitchClient::FreeswitchEventRtmpDestory(const Json::Value& root) {
    string rtmp_session = "";
    if( root["RTMP-Session-ID"].isString() ) {
    	rtmp_session = root["RTMP-Session-ID"].asString();
    }

    // 用户是否改变状态
    bool bUserStatusChange = false;
	// 创建连接信息
	RtmpObject rtmpObject;
	// 移出在线用户列表
    if( rtmp_session.length() > 0 ) {
    	mRtmpSessionMap.Lock();
    	RtmpUserMap::iterator itr = mRtmpUserMap.Find(rtmp_session);
    	if( itr != mRtmpUserMap.End() ) {
    		LogManager::GetLogManager()->Log(
    				LOG_WARNING,
    				"FreeswitchClient::FreeswitchEventRtmpDestory( "
    				"event : [Freeswitch-事件处理-rtmp终端断开-有身份], "
					"user : %s, "
    				"rtmp_session : %s "
    				")",
					itr->second.c_str(),
    				rtmp_session.c_str()
    				);

    		// 查找用户的对应的rtmp_session list
        	RtmpSessionMap::iterator itr2 = mRtmpSessionMap.Find(itr->second);
        	if( itr2 != mRtmpSessionMap.End() ) {
        		RtmpObjectList* pRtmpObjectList = itr2->second;
    			// 移除用户的对应的一个rtmp_session
    			pRtmpObjectList->Lock();
    			for(RtmpObjectList::iterator itr = pRtmpObjectList->Begin(); itr != pRtmpObjectList->End();) {
    				rtmpObject = *itr;
    				if( rtmp_session == rtmpObject.rtmpSession ) {
    					pRtmpObjectList->PopValueUnSafe(itr++);
    					break;
    				} else {
    					itr++;
    				}
    			}
    			pRtmpObjectList->Unlock();

    			// 清除user -> rtmp_session关系
    			if( pRtmpObjectList->Empty() ) {
					delete pRtmpObjectList;
					pRtmpObjectList = NULL;
					mRtmpSessionMap.Erase(itr2);

					bUserStatusChange = true;
				}
        	}

			// 清除rtmp_session -> user关系
			mRtmpUserMap.Erase(itr);

    	} else {
    		LogManager::GetLogManager()->Log(
    				LOG_WARNING,
    				"FreeswitchClient::FreeswitchEventRtmpDestory( "
    				"event : [Freeswitch-事件处理-rtmp终端断开-无身份], "
    				"rtmp_session : %s "
    				")",
    				rtmp_session.c_str()
    				);
    	}
    	mRtmpSessionMap.Unlock();
    }

    // 通知用户下线
	if(mpFreeswitchClientListener) {
		mpFreeswitchClientListener->OnFreeswitchEventOnlineStatus(this, rtmpObject, false);
	}
}

void FreeswitchClient::FreeswitchEventWebsocketConnect(const Json::Value& root) {
	mWebSocketUserCount++;

	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"FreeswitchClient::FreeswitchEventWebsocketConnect( "
			"event : [Freeswitch-事件处理-websocket终端连接] "
			")"
			);
}

void FreeswitchClient::FreeswitchEventWebsocketLogin(const Json::Value& root) {
	string user;
    if( root["User"].isString() ) {
    	user = root["User"].asString();
    }

	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"FreeswitchClient::FreeswitchEventWebsocketLogin( "
			"event : [Freeswitch-事件处理-websocket终端登录], "
			"user : %s "
			")",
			user.c_str()
			);
}

void FreeswitchClient::FreeswitchEventWebsocketDestory(const Json::Value& root) {
	string user;
    if( root["User"].isString() ) {
    	user = root["User"].asString();
    }

	mWebSocketUserCount--;

	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"FreeswitchClient::FreeswitchEventWebsocketDestory( "
			"event : [Freeswitch-事件处理-websocket终端断开], "
			"user : %s "
			")",
			user.c_str()
			);

}

void FreeswitchClient::FreeswitchEventConferenceAddMember(const Json::Value& root) {
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
    string memberType = "";
    MemberType type = Member;
    if( root["Member-Type"].isString() ) {
    	memberType = root["Member-Type"].asString();
    	if( memberType == "moderator" ) {
    		type = Moderator;
    	}
    }

	// 收到事件时候, 连接还没断开
	// 插入channel
	Channel* channel = NULL;
	if( channelId.length() > 0 ) {
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
	}

	// 已经登录
	if( channel && channel->user.length() > 0 ) {
		LogManager::GetLogManager()->Log(
				LOG_WARNING,
				"FreeswitchClient::FreeswitchEventConferenceAddMember( "
				"event : [Freeswitch-事件处理-增加会议室成员], "
				"user : %s, "
				"conference : %s, "
				"type : %s, "
				"memberId : %s, "
				"channelId : %s, "
				"rtmp_session : %s "
				")",
				channel->user.c_str(),
				conference.c_str(),
				(type == Moderator)?"Moderator":"Member",
				channel->memberId.c_str(),
				channelId.c_str(),
				channel->rtmpSession.c_str()
				);

		if( type == Moderator ) {
			if( mbIsRecording && conference.length() > 0 && channel->siteId.length() > 0 ) {
				// 开始录制视频
				StartRecordConference(conference, channel->siteId, channel->recordFilePath);
			}
		}

		// 回调用户进入会议室
		if( mpFreeswitchClientListener ) {
			mpFreeswitchClientListener->OnFreeswitchEventConferenceAddMember(this, channel);
		}
	} else {
		// 踢出用户
		char temp[1024] = {'\0'};
		string result = "";
		snprintf(temp, sizeof(temp), "api conference %s kick %s", conference.c_str(), memberId.c_str());
		if( SendCommandGetResult(temp, result) ) {
			LogManager::GetLogManager()->Log(
					LOG_WARNING,
					"FreeswitchClient::FreeswitchEventConferenceAddMember( "
					"event : [Freeswitch-事件处理-增加会议室成员-踢出未登录用户], "
					"conference : %s, "
					"memberId : %s, "
					"channelId: %s, "
					"rtmp_session : %s "
					")",
					conference.c_str(),
					memberId.c_str(),
					channelId.c_str(),
					channel->rtmpSession.c_str()
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

		if( channel && channel->user.length() > 0 ) {
			// 停止录制视频
			if( channel->type == Moderator ) {
				StopRecordConference(conference, channel->recordFilePath);
			}

			LogManager::GetLogManager()->Log(
					LOG_WARNING,
					"FreeswitchClient::FreeswitchEventConferenceDelMember( "
					"event : [Freeswitch-事件处理-删除会议室成员], "
					"user : %s, "
					"conference : %s, "
					"memberId : %s, "
					"channelId: %s, "
					"rtmp_session : %s "
					")",
					channel->user.c_str(),
					conference.c_str(),
					memberId.c_str(),
					channelId.c_str(),
					channel->rtmpSession.c_str()
					);

			// 回调用户退出会议室
			if( mpFreeswitchClientListener ) {
				mpFreeswitchClientListener->OnFreeswitchEventConferenceDelMember(this, channel);
			}
		}
	}
}

void FreeswitchClient::FreeswitchEventChannelCreate(const Json::Value& root) {
    string channelId = "";
    if( root["Channel-Call-UUID"].isString() ) {
    	channelId = root["Channel-Call-UUID"].asString();
    }

    string rtmp_session = "";
    if( root["variable_rtmp_session"].isString() ) {
    	rtmp_session = root["variable_rtmp_session"].asString();
    }
    
    string conference = "";
    if( root["variable_conference"].isString() ) {
    	conference = root["variable_conference"].asString();
    }

    string serverId = "";
    if( root["variable_serverId"].isString() ) {
    	serverId = root["variable_serverId"].asString();
    }

    string siteId = "";
    if( root["variable_siteId"].isString() ) {
    	siteId = root["variable_siteId"].asString();
    }

    // Alex, 增加的登录验证类型
    string chat_type_string = "";
    ENTERCONFERENCETYPE chatType = ENTERCONFERENCETYPE_CAMSHARE;
    if ( root["variable_chat_type_string"].isString() ) {
    	chat_type_string = root["variable_chat_type_string"].asString();
    	if ( chat_type_string.length() > 0 ) {
    		chatType = GetChatTypeWithChannelVar(chat_type_string);
    	}
    }

	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"FreeswitchClient::FreeswitchEventChannelCreate( "
			"event : [Freeswitch-事件处理-创建频道], "
			"channelId : %s, "
			"rtmp_session : %s, "
			"conference : %s, "
			"serverId : %s, "
			"siteId : %s, "
            "chat_type_string : %s, "
            "chatType : %d "
			")",
			channelId.c_str(),
			rtmp_session.c_str(),
			conference.c_str(),
			serverId.c_str(),
			siteId.c_str(),
			chat_type_string.c_str(),
            chatType
			);

	mRtmpChannel2UserMap.Lock();
	// 更新信息
	Channel* channel = CreateChannel(channelId);
	channel->rtmpSession = (channel->rtmpSession.length() > 0)?channel->rtmpSession:rtmp_session;
	channel->conference = (channel->conference.length() > 0)?channel->conference:conference;
	channel->serverId = (channel->serverId.length() > 0)?channel->serverId:serverId;
	channel->siteId = (channel->siteId.length() > 0)?channel->siteId:siteId;

    // Alex, 增加的l登录验证类型
	if ( chatType != ENTERCONFERENCETYPE_CAMSHARE ) {
		channel->chatType = chatType;
	}
	mRtmpChannel2UserMap.Unlock();
}

void FreeswitchClient::FreeswitchEventChannelDestroy(const Json::Value& root) {
    string channelId = "";
    if( root["Channel-Call-UUID"].isString() ) {
    	channelId = root["Channel-Call-UUID"].asString();
    }

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"FreeswitchClient::FreeswitchEventChannelDestroy( "
			"event : [Freeswitch-事件处理-销毁频道], "
			"channelId : %s "
			")",
			channelId.c_str()
			);

    // 销毁频道
	if( channelId.length() > 0 ) {
		DestroyChannel(channelId);
	}

}
