/*
 * WSClientParser.cpp
 *
 *  Created on: 2016年9月14日
 *      Author: max
 */

#include "WSClientParser.h"
#include "Arithmetic.hpp"

#include <openssl/ssl.h>

//static const char hex[16] = {
//	'0','1','2','3','4','5','6','7',
//	'8','9','A','B','C','D','E','F'
//};

static const char c64[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

#define HTTP_HEADER_SEC_WEBSOCKET_KEY "Sec-WebSocket-Key"
#define WEBSOCKET_GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

#define HTTP_URL_MAX_PATH 2048
#define HTTP_URL_FIRSTLINE_PARAM_COUNT 3
//#define HTTP_URL_PATH_PARAM_COUNT 5
// 临时解决, 不做PHP用户验证
#define HTTP_URL_PATH_PARAM_COUNT 3

#define HTTP_PARAM_SEP ":"
#define HTTP_LINE_SEP "\r\n"
#define HTTP_HEADER_SEP "\r\n\r\n"

#define SHA1_HASH_SIZE 20

#ifdef NO_OPENSSL
static void sha1_digest(char *digest, unsigned char *in)
{
	SHA1Context sha;
	char *p;
	int x;


	SHA1Init(&sha);
	SHA1Update(&sha, in, strlen(in));
	SHA1Final(&sha, digest);
}
#else
static void sha1_digest(unsigned char *digest, char *in)
{
	SHA_CTX sha;

	SHA1_Init(&sha);
	SHA1_Update(&sha, in, strlen(in));
	SHA1_Final(digest, &sha);

}
#endif

static int b64encode(unsigned char *in, size_t ilen, unsigned char *out, size_t olen)
{
	int y=0,bytes=0;
	size_t x=0;
	unsigned int b=0,l=0;

	if(olen) {
	}

	for(x=0;x<ilen;x++) {
		b = (b<<8) + in[x];
		l += 8;
		while (l >= 6) {
			out[bytes++] = c64[(b>>(l-=6))%64];
			if(++y!=72) {
				continue;
			}
			//out[bytes++] = '\n';
			y=0;
		}
	}

	if (l > 0) {
		out[bytes++] = c64[((b%16)<<(6-l))%64];
	}
	if (l != 0) while (l < 6) {
		out[bytes++] = '=', l += 2;
	}

	return 0;
}

WSClientParser::WSClientParser() {
	// TODO Auto-generated constructor stub
	mState = WSClientState_UnKnow;
	mpCallback = NULL;
	mpClient = NULL;

	mpUser = NULL;
	mpDomain = NULL;
	mpDestNumber = NULL;
	mpSite = NULL;
	mpCustom = NULL;
	mpChannel = NULL;
	memset(mWebSocketKey, '\0', sizeof(mWebSocketKey));

	// 创建内存池
	switch_core_new_memory_pool(&mpPool);
	switch_mutex_init(&clientMutex, SWITCH_MUTEX_NESTED, mpPool);

	// 生成唯一标识
	switch_uuid_t uuid;
	switch_uuid_get(&uuid);
	switch_uuid_format(this->uuid, &uuid);
}

WSClientParser::~WSClientParser() {
	// TODO Auto-generated destructor stub
	switch_mutex_destroy(clientMutex);

	switch_core_destroy_memory_pool(&mpPool);
	mpPool = NULL;

}

int WSClientParser::ParseData(char* buffer, int len) {
	switch_log_printf(
			SWITCH_CHANNEL_UUID_LOG(this->uuid),
			SWITCH_LOG_DEBUG,
			"WSClientParser::ParseData( "
			"parser : %p, "
			"len : %d "
//			"buffer : \n%s\n"
			") \n",
			this,
			len
//			buffer
			);

	int ret = 0;
//	DataParser::ParseData(NULL, len);

	Lock();
	switch( mState ) {
	case WSClientState_UnKnow:{
		int lineNumber = 0;

		char line[HTTP_URL_MAX_PATH];
		int lineLen = 0;

		const char* header = buffer;
		const char* sepHeader = strstr(buffer, HTTP_HEADER_SEP);
		const char* sep = NULL;
		if( sepHeader ) {
			switch_log_printf(
					SWITCH_CHANNEL_UUID_LOG(this->uuid),
					SWITCH_LOG_INFO,
					"WSClientParser::ParseData( "
					"[HandShake], "
					"parser : %p, "
					"len : %d, "
					"buffer : \n%s\n"
					") \n",
					this,
					len,
					buffer
					);

			// Parse HTTP header separator
			ret = sepHeader - buffer + strlen(HTTP_HEADER_SEP);

			// Parse HTTP header line separator
			while( true ) {
				if( (sep = strstr(header, HTTP_LINE_SEP)) && sep != sepHeader ) {
					lineLen = sep - header;
					if( lineLen < sizeof(line) - 1 ) {
						memcpy(line, header, lineLen);
						line[lineLen] = '\0';

						if( lineNumber == 0 ) {
							// Get First Line
							if( ParseFirstLine(line) ) {

							}
						} else {
							ParseHeader(line);
							if( CheckHandShake() ) {
								break;
							}
						}
					}

					header += lineLen + strlen(HTTP_LINE_SEP);

					lineNumber++;

				} else {
					break;
				}
			}
		}
		Unlock();

		if( mState == WSClientState_Handshake ) {
			if( mpCallback ) {
				mpCallback->OnWSClientParserHandshake(this);
			}
		} else {
			if( mpCallback ) {
				mpCallback->OnWSClientDisconected(this);
			}
		}
	}break;
	case WSClientState_Handshake:{

	}
	case WSClientState_Data:{
//		char temp[4096] = {0};
//		char *p = temp;
//		unsigned char c;
//		for(int i = 0; i< len; i++) {
//			c = buffer[i];
//			sprintf(p, "%c%c,", hex[c >> 4], hex[c & (16 - 1)]);
//			p += 3;
//		}
//		switch_log_printf(
//				SWITCH_CHANNEL_UUID_LOG(this->uuid),
//				SWITCH_LOG_INFO,
//				"WSClientParser::ParseData( "
//				"parser : %p, "
//				"hex : \n%s\n"
//				") \n",
//				this,
//				temp
//				);

		WSPacket* packet = (WSPacket *)buffer;

//		switch_log_printf(
//				SWITCH_CHANNEL_UUID_LOG(this->uuid),
//				SWITCH_LOG_INFO,
//				"WSClientParser::ParseData( "
//				"parser : %p, "
//				"packet->Fin : %s, "
//				"packet->RSV : %u, "
//				"packet->Opcode : %u, "
//				"packet->Mask : %s, "
//				"packet->Playload Length : %u, "
//				"packet->Extend Playload Length : %llu, "
//				"packet->Mask Key : %s "
//				") \n",
//				this,
//				packet->baseHeader.IsFin()?"true":"false",
//				packet->baseHeader.GetRSVType(),
//				packet->baseHeader.GetOpcode(),
//				packet->baseHeader.IsMask()?"true":"false",
//				packet->baseHeader.GetPlayloadLength(),
//				packet->GetPlayloadLength(),
//				packet->GetMaskKey()
//				);

		Unlock();

		const char* data = (const char*)packet->GetData();//buffer;//packet->GetData();
		int playloadLength = packet->GetPlayloadLength();//len;//packet->GetPlayloadLength();
		ret = packet->GetHeaderLength() + playloadLength;
		if( mpCallback) {
			mpCallback->OnWSClientParserData(this, data, playloadLength);
		}

	}break;
	case WSClientState_Disconnected:{
		ret = len;
		Unlock();
	}break;
	default:{
		Unlock();
		break;
	}
	}

	return ret;
}

bool WSClientParser::GetHandShakeRespond(char** buffer, int& len) {
	if( mState == WSClientState_Handshake ) {
		mState = WSClientState_Data;

		char input[256] = "";
		unsigned char output[SHA1_HASH_SIZE] = "";
		char b64[256] = "";

		snprintf(input, sizeof(input), "%s%s", mWebSocketKey, WEBSOCKET_GUID);
		sha1_digest(output, input);
		b64encode((unsigned char *)output, SHA1_HASH_SIZE, (unsigned char *)b64, sizeof(b64));

		char* temp = switch_core_sprintf(
				mpPool,
				"HTTP/1.1 101 Switching Protocols\r\n"
				"Upgrade: websocket\r\n"
				"Connection: Upgrade\r\n"
				"Sec-WebSocket-Accept: %s\r\n"
				"\r\n",
				b64
				);

		*buffer = temp;
		len = strlen(temp);

		switch_log_printf(
				SWITCH_CHANNEL_UUID_LOG(this->uuid),
				SWITCH_LOG_INFO,
				"WSClientParser::GetHandShakeRespond( "
				"parser : %p, "
				"len : %d, "
				"buffer : \n%s\n"
				") \n",
				this,
				len,
				*buffer
				);

		return true;
	}
	return false;
}

bool WSClientParser::GetPacket(WSPacket* packet, unsigned long long dataLen) {
	if( mState == WSClientState_Data ) {
		packet->Reset();

		packet->baseHeader.SetFin(true);
		packet->baseHeader.SetRSVType(WS_RSV_TYPE_DEFAULT);
		packet->baseHeader.SetOpcode(WS_OPCODE_BINARY);
		packet->SetPlayloadLength(dataLen);
		packet->SetMaskKey(NULL);

		switch_log_printf(
				SWITCH_CHANNEL_UUID_LOG(this->uuid),
				SWITCH_LOG_DEBUG,
				"WSClientParser::GetPacket( "
				"parser : %p, "
				"dataLen : %lld, "
				"packet->Fin : %s, "
				"packet->RSV : %u, "
				"packet->Opcode : %u, "
				"packet->Mask : %s, "
				"packet->Playload Length : %u, "
				"packet->Extend Playload Length : %llu, "
				"packet->Mask Key : %s "
				") \n",
				this,
				dataLen,
				packet->baseHeader.IsFin()?"true":"false",
				packet->baseHeader.GetRSVType(),
				packet->baseHeader.GetOpcode(),
				packet->baseHeader.IsMask()?"true":"false",
				packet->baseHeader.GetPlayloadLength(),
				packet->GetPlayloadLength(),
				packet->GetMaskKey()
				);

		return true;
	}
	return false;
}

void WSClientParser::SetWSClientParserCallback(WSClientParserCallback* callback) {
	mpCallback = callback;
}

bool WSClientParser::Login() {
	switch_log_printf(
			SWITCH_CHANNEL_UUID_LOG(this->uuid),
			SWITCH_LOG_NOTICE,
			"WSClientParser::Login( "
			"this : %p, "
			"user : '%s', "
			"domain : '%s', "
			"site : '%s', "
			"custom : '%s' "
			") \n",
			this,
			mpUser,
			mpDomain,
			mpSite,
			mpCustom
			);

// 临时解决, 不做PHP用户验证
//	switch_event_t *locate_params;
//	switch_xml_t xml = NULL;
//
//	switch_event_create(&locate_params, SWITCH_EVENT_GENERAL);
//	switch_assert(locate_params);
//	switch_event_add_header_string(locate_params, SWITCH_STACK_BOTTOM, "source", "mod_ws");
//	switch_event_add_header_string(locate_params, SWITCH_STACK_BOTTOM, "site", mpSite);
//	switch_event_add_header_string(locate_params, SWITCH_STACK_BOTTOM, "custom", mpCustom);
//
//	/* Locate user */
//	if (switch_xml_locate_user_merged("id", mpUser, mpDomain, NULL, &xml, locate_params) != SWITCH_STATUS_SUCCESS) {
//		switch_log_printf(
//				SWITCH_CHANNEL_UUID_LOG(this->uuid),
//				SWITCH_LOG_WARNING,
//				"WSClientParser::Login( "
//				"this : %p, "
//				"user : '%s', "
//				"domain : '%s' ",
//				this,
//				mpUser,
//				mpDomain
//				);
//		return false;
//	}

	// 发送登录成功事件
	ws_login();

	return true;
}

WSChannel* WSClientParser::CreateCall(
		switch_core_session_t *session,
		const char *profileName,
//		const char *destNumber,
		const char *profileContext,
		const char *profileDialplan,
		const char* ip
		) {
	switch_memory_pool_t *pool = NULL;
	switch_channel_t* channel = NULL;
	switch_caller_profile_t *caller_profile = NULL;

	char *user = mpUser;
	char *domain = mpDomain;
	const char *destNumber = mpDestNumber;
	const char *context = NULL;
	const char *dialplan = NULL;
	switch_status_t status = SWITCH_STATUS_FALSE;

	if ( !user || !domain || !destNumber ) {
		return NULL;
	}

	pool = switch_core_session_get_pool(session);
	channel = switch_core_session_get_channel(session);
	switch_channel_set_name(
			channel,
			switch_core_session_sprintf(session, "ws/%s/%s/%s", profileName, user, destNumber)
			);

	if (!zstr(user) && !zstr(domain)) {
		const char *ivrUser = switch_core_session_sprintf(session, "%s@%s", user, domain);
//		status = switch_ivr_set_user(session, ivrUser);
		// 临时解决, 不做PHP用户验证
		switch_ivr_set_user(session, ivrUser);
		status = SWITCH_STATUS_SUCCESS;
	}

	if( status == SWITCH_STATUS_SUCCESS ) {
		if (!(context = switch_channel_get_variable(channel, "user_context"))) {
			if (!(context = profileContext)) {
				context = "public";
			}
		}

		if (!(dialplan = switch_channel_get_variable(channel, "inbound_dialplan"))) {
			if (!(dialplan = profileDialplan)) {
				dialplan = "LUA";
			}
		}

		//	switch_log_printf(
		//			SWITCH_CHANNEL_SESSION_LOG(session),
		//			SWITCH_LOG_NOTICE,
		//			"WSClientParser::CreateCall( "
		//			"context : %s, "
		//			"dialplan : %s "
		//			") \n",
		//			context,
		//			dialplan
		//			);

		// 设置主叫
		caller_profile = switch_caller_profile_new(
				pool,
				switch_str_nil(user),
				dialplan,
				SWITCH_DEFAULT_CLID_NAME,
				!zstr(user) ? user : SWITCH_DEFAULT_CLID_NUMBER,
				ip /* net addr */,
				NULL /* ani   */,
				NULL /* anii  */,
				NULL /* rdnis */,
				"mod_ws",
				context,
				destNumber
				);
		switch_channel_set_caller_profile(channel, caller_profile);
		switch_core_session_add_stream(session, NULL);
		switch_channel_set_variable(channel, "caller", user);
		switch_channel_set_state(channel, CS_INIT);

		return CreateChannel(session);
	}

	return NULL;
}

WSChannel* WSClientParser::CreateChannel(switch_core_session_t *session) {
	WSChannel *wsChannel = NULL;

	if( mState != WSClientState_Disconnected ) {
		// 连接还没被销毁
		DestroyCall();
	}

	// 创建channel
//	switch_memory_pool_t *pool = switch_core_session_get_pool(session);
	wsChannel = (WSChannel *)switch_core_alloc(mpPool, sizeof(WSChannel));
	wsChannel->session = session;
	wsChannel->parser = this;
	wsChannel->uuid_str = switch_core_strdup(mpPool, switch_core_session_get_uuid(session));

	// 创建计时器
//	switch_core_timer_init(&wsChannel->timer, "soft", 20, (16000 / (1000 / 20)), mpPool);

	if( InitChannel(wsChannel) ) {
		mpChannel = wsChannel;
	}

	return wsChannel;
}

WSChannel* WSClientParser::DestroyCall() {
	WSChannel* wsChannel = NULL;
	wsChannel = mpChannel;
	mpChannel = NULL;
	return wsChannel;
}

bool WSClientParser::InitChannel(WSChannel* wsChannel) {
	switch_core_session_t *session = wsChannel->session;
	switch_channel_t* channel = switch_core_session_get_channel(wsChannel->session);

	// 初始化语音
	wsChannel->read_frame.data = wsChannel->databuf;
	wsChannel->read_frame.buflen = sizeof(wsChannel->databuf);

	/* Initialize read & write codecs */
	if (switch_core_codec_init(
			&wsChannel->read_codec,
			/* name */ "SPEEX",
			/* modname */ NULL,
			/* fmtp */ NULL,
			/* rate */ 16000,
			/* ms */ 20,
			/* channels */ 1,
			/* flags */ SWITCH_CODEC_FLAG_ENCODE | SWITCH_CODEC_FLAG_DECODE,
			/* codec settings */ NULL, switch_core_session_get_pool(session)) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(
				SWITCH_CHANNEL_SESSION_LOG(session),
				SWITCH_LOG_ERROR,
				"WSClientParser::InitChannel( "
				"Can't initialize read codec "
				")\n"
				);

		return false;
	}

	if (switch_core_codec_init(
			&wsChannel->write_codec,
			/* name */ "SPEEX",
			/* modname */ NULL,
			/* fmtp */ NULL,
			/* rate */ 16000,
			/* ms */ 20,
			/* channels */ 1,
			/* flags */ SWITCH_CODEC_FLAG_ENCODE | SWITCH_CODEC_FLAG_DECODE,
			/* codec settings */ NULL, switch_core_session_get_pool(session)) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(
				SWITCH_CHANNEL_SESSION_LOG(session),
				SWITCH_LOG_ERROR, ""
				"WSClientParser::InitChannel( "
				"Can't initialize write codec "
				")\n"
				);

		return false;
	}

	switch_core_session_set_read_codec(session, &wsChannel->read_codec);
	switch_core_session_set_write_codec(session, &wsChannel->write_codec);

	//static inline uint8_t rtmp_audio_codec(int channels, int bits, int rate, rtmp_audio_format_t format) {
	wsChannel->audio_codec = 0xB2; //rtmp_audio_codec(1, 16, 0 /* speex is always 8000  */, RTMP_AUDIO_SPEEX);

	// 初始化视频
	switch_codec_settings_t codec_settings = {{ 0 }};
	wsChannel->video_read_frame.data = wsChannel->video_databuf;
	wsChannel->video_read_frame.buflen = sizeof(wsChannel->video_databuf);

	/* Initialize video read & write codecs */
	if (switch_core_codec_init(
			&wsChannel->video_read_codec,
			/* name */ "H264",
			/* modname */ NULL,
			/* fmtp */ NULL,
			/* rate */ 90000,
			/* ms */ 0,
			/* channels */ 1,
			/* flags */ SWITCH_CODEC_FLAG_ENCODE | SWITCH_CODEC_FLAG_DECODE,
			/* codec settings */ NULL,
			switch_core_session_get_pool(session)) != SWITCH_STATUS_SUCCESS
			) {
		switch_log_printf(
				SWITCH_CHANNEL_SESSION_LOG(session),
				SWITCH_LOG_ERROR,
				"WSClientParser::InitChannel( "
				"Can't initialize video read codec "
				") \n"
				);

		return false;
	}

	codec_settings.video.bandwidth = switch_parse_bandwidth_string("1mb");

	if (switch_core_codec_init(
			&wsChannel->video_write_codec,
			/* name */ "H264",
			/* modname */ NULL,
			/* fmtp */ NULL,
			/* rate */ 90000,
			/* ms */ 0,
			/* channels */ 1,
			/* flags */ SWITCH_CODEC_FLAG_ENCODE | SWITCH_CODEC_FLAG_DECODE,
			/* codec settings */ &codec_settings,
			switch_core_session_get_pool(session)) != SWITCH_STATUS_SUCCESS
			) {
		switch_log_printf(
				SWITCH_CHANNEL_SESSION_LOG(session),
				SWITCH_LOG_ERROR,
				"WSClientParser::InitChannel( "
				"Can't initialize write codec "
				") \n"
				);
		return false;
	}

	switch_core_session_set_video_read_codec(session, &wsChannel->video_read_codec);
	switch_core_session_set_video_write_codec(session, &wsChannel->video_write_codec);
	switch_channel_set_flag(channel, CF_VIDEO);

	wsChannel->mparams.external_video_source = SWITCH_TRUE;
	switch_media_handle_create(&wsChannel->media_handle, session, &wsChannel->mparams);

	wsChannel->video_read_frame.packet = wsChannel->video_databuf;
	wsChannel->video_read_frame.data = wsChannel->video_databuf + 12;
	wsChannel->video_read_frame.buflen = SWITCH_RECOMMENDED_BUFFER_SIZE - 12;

	switch_mutex_init(&wsChannel->video_readbuf_mutex, SWITCH_MUTEX_NESTED, switch_core_session_get_pool(session));

	switch_buffer_create_dynamic(&wsChannel->video_readbuf, 1024, 1024, 2048000);

	wsChannel->video_codec = 0xB2;

	return true;
}

bool WSClientParser::IsConnected() {
	return mState != WSClientState_Disconnected;
}

void WSClientParser::Disconnected() {
	ws_disconnect();
	mState = WSClientState_Disconnected;
}

void WSClientParser::SetClient(Client *client) {
	mpClient = client;
}

Client *WSClientParser::GetClient() {
	return mpClient;
}

void WSClientParser::Lock() {
	switch_mutex_lock(clientMutex);
}

void WSClientParser::Unlock() {
	switch_mutex_unlock(clientMutex);
}

bool WSClientParser::GetFrame(switch_frame_t *frame, const char** data, switch_size_t *len) {
	return mVideoTransfer.GetFrame(frame, data, len);
}

void WSClientParser::DestroyVideoBuffer() {
	return mVideoTransfer.DestroyVideoBuffer();
}

const char* WSClientParser::GetUUID() {
	return uuid;
}

const char* WSClientParser::GetUser() {
	return mpUser;
}

const char* WSClientParser::GetDomain() {
	return mpDomain;
}

const char* WSClientParser::GetDestNumber() {
	return mpDestNumber;
}

bool WSClientParser::ParseFirstLine(char* line) {
	bool bFlag = false;

	// Get Param
	char* p = line;
	char* delim = " ";

	char *array[10];
	if( switch_separate_string_string(p, delim, array, HTTP_URL_FIRSTLINE_PARAM_COUNT) == HTTP_URL_FIRSTLINE_PARAM_COUNT ) {
		char decodeUrl[HTTP_URL_MAX_PATH] = {0};
		Arithmetic ac;
		ac.decode_url(array[1], strlen(array[1]), decodeUrl);

		switch_log_printf(
				SWITCH_CHANNEL_UUID_LOG(this->uuid),
				SWITCH_LOG_INFO,
				"WSClientParser::ParseFirstLine( "
				"parser : %p, "
				"decodeUrl : '%s' "
				") \n",
				this,
				decodeUrl
				);

		if( strlen(decodeUrl) > 0 ) {
			p = decodeUrl;
			delim = "/";
			if( *p == *delim ) {
				p++;
			}
			if( switch_separate_string_string(p, delim, array, HTTP_URL_PATH_PARAM_COUNT) >= HTTP_URL_PATH_PARAM_COUNT ) {
				mpUser = switch_core_strdup(mpPool, array[0]);
				mpDomain = switch_core_strdup(mpPool, array[1]);
				mpDestNumber = switch_core_strdup(mpPool, array[2]);
//				mpSite = switch_core_strdup(mpPool, array[3]);
//				mpCustom = switch_core_strdup(mpPool, array[4]);
				// 临时解决, 不做PHP用户验证
				mpSite = switch_core_strdup(mpPool, "");
				mpCustom = switch_core_strdup(mpPool, "");

				switch_log_printf(
						SWITCH_CHANNEL_UUID_LOG(this->uuid),
						SWITCH_LOG_INFO,
						"WSClientParser::ParseFirstLine( "
						"parser : %p, "
						"user : '%s', "
						"domain : '%s', "
						"destnumber : '%s', "
						"site : '%s', "
						"custom : '%s' "
						") \n",
						this,
						mpUser,
						mpDomain,
						mpDestNumber,
						mpSite,
						mpCustom
						);
			}
		}

		bFlag = true;
	}

	return bFlag;
}

void WSClientParser::ParseHeader(char* line) {
	char* paramSep = NULL;
	char* key = NULL;
	char* value = NULL;

	// Parse HTTP header line parameter separator
	if( (paramSep = strstr(line, HTTP_PARAM_SEP)) ) {
		*paramSep = '\0';
		key = line;

		value = paramSep + strlen(HTTP_PARAM_SEP);
		value = switch_strip_spaces(value, SWITCH_FALSE);

		// Sec-WebSocket-Key
		if( strcmp(key, HTTP_HEADER_SEC_WEBSOCKET_KEY) == 0 ) {
			strncpy(mWebSocketKey, value, sizeof(mWebSocketKey) - 1);
		}
	}
}

bool WSClientParser::CheckHandShake() {
	bool bFlag = false;
	if( mpUser && strlen(mpUser) > 0
		&& mpDomain && strlen(mpDomain) > 0
		&& mpDestNumber && strlen(mpDestNumber) > 0
		&& strlen(mWebSocketKey) > 0
	) {
		mState = WSClientState_Handshake;
		bFlag = true;
	}
	return bFlag;
}

bool WSClientParser::ws_login() {
	switch_status_t status = SWITCH_STATUS_FALSE;
	switch_event_t *event;
	if ( (status = switch_event_create_subclass(&event, SWITCH_EVENT_CUSTOM, WS_EVENT_MAINT)) == SWITCH_STATUS_SUCCESS) {
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "User", mpUser);
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "Domain", mpDomain);
		status = switch_event_fire(&event);
	}

	if( status != SWITCH_STATUS_SUCCESS ) {
		switch_log_printf(
				SWITCH_CHANNEL_UUID_LOG(this->uuid),
				SWITCH_LOG_ERROR,
				"WSClientParser::ws_login( "
				"[Send event Fail], "
				"this : %p, "
				"user : '%s', "
				"domain : '%s' "
				") \n",
				this,
				mpUser,
				mpDomain
				);
	}

	return status == SWITCH_STATUS_SUCCESS;
}

bool WSClientParser::ws_disconnect() {
	switch_status_t status = SWITCH_STATUS_FALSE;
	switch_event_t *event;
	if ( (status = switch_event_create_subclass(&event, SWITCH_EVENT_CUSTOM, WS_EVENT_MAINT)) == SWITCH_STATUS_SUCCESS) {
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "User", mpUser);
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "Domain", mpDomain);
		status = switch_event_fire(&event);
	}

	if( status != SWITCH_STATUS_SUCCESS ) {
		switch_log_printf(
				SWITCH_CHANNEL_UUID_LOG(this->uuid),
				SWITCH_LOG_ERROR,
				"WSClientParser::ws_disconnect( "
				"[Send event Fail], "
				"this : %p, "
				"user : '%s', "
				"domain : '%s' "
				") \n",
				this,
				mpUser,
				mpDomain
				);
	}

	return status == SWITCH_STATUS_SUCCESS;
}
