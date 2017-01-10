/*
 * WSClientParser.h
 *
 *  Created on: 2016年9月14日
 *      Author: max
 */

#ifndef SRC_MOD_ENDPOINTS_MOD_WS_WSCLIENTPARSER_H_
#define SRC_MOD_ENDPOINTS_MOD_WS_WSCLIENTPARSER_H_

#include <switch.h>

#include "DataParser.h"
#include "Client.h"
#include "WSPacket.h"
#include "Rtp2H264VideoTransfer.h"

#define WS_EVENT_MAINT "ws::maintenance"

class WSClientParser;
typedef struct WSChannel {
	WSChannel() {
		parser = NULL;
		session = NULL;

		video_max_bandwidth_out = NULL;
		video_readbuf = NULL;
		video_readbuf_mutex = NULL;
		media_handle = NULL;

		uuid_str = NULL;
	}

	WSClientParser* parser;
	switch_core_session_t *session;

	// audio
	switch_codec_t read_codec;
	switch_codec_t write_codec;
	uint8_t audio_codec;
	switch_frame_t read_frame;
	unsigned char databuf[SWITCH_RECOMMENDED_BUFFER_SIZE];	/* < Buffer for read_frame */
	switch_timer_t timer;

	// video
	int has_video;
	char *video_max_bandwidth_out;
	switch_codec_t video_read_codec;
	switch_codec_t video_write_codec;
	switch_frame_t video_read_frame;
	uint32_t video_read_ts;
	uint16_t seq;
	unsigned char video_databuf[SWITCH_RTP_MAX_BUF_LEN];	/* < Buffer for read_frame */
	switch_buffer_t *video_readbuf;
	switch_mutex_t *video_readbuf_mutex;
	uint16_t video_maxlen;
	int video_over_size;
	uint8_t video_codec;

	switch_core_media_params_t mparams;
	switch_media_handle_t *media_handle;

	char* uuid_str;

} WSChannel;

class WSClientParser;
class WSClientParserCallback {
public:
	virtual ~WSClientParserCallback(){};
	virtual void OnWSClientParserHandshake(WSClientParser* parser) = 0;
	virtual void OnWSClientParserData(WSClientParser* parser, const char* buffer, int len) = 0;
	virtual void OnWSClientDisconected(WSClientParser* parser) = 0;
};

typedef enum WSClientState {
	WSClientState_UnKnow = 0,
	WSClientState_Handshake,
	WSClientState_Data,
	WSClientState_Disconnected
};

class WSClientParser : public DataParser {
public:
	WSClientParser();
	virtual ~WSClientParser();

	// 解析WebSocket协议
	int ParseData(char* buffer, int len);
	bool GetHandShakeRespond(char** sendBuffer, int& len);
	bool GetPacket(WSPacket* packet, unsigned long long dataLen);

	void SetWSClientParserCallback(WSClientParserCallback* callback);

	/**
	 * 登录
	 */
	bool Login();

	/**
	 * 创建和销毁频道
	 */
	WSChannel* CreateCall(
			switch_core_session_t *session,
			const char *profileName,
			const char *profileContext,
			const char *profileDialplan,
			const char* ip
			);
	/**
	 * 销毁会话
	 */
	void DestroyCall();

	/**
	 * 是否存在会话
	 */
	bool IsAlreadyCall();

	/**
	 * 挂断会话
	 */
	bool HangupCall();

	/**
	 * 状态管理函数
	 */
	bool IsConnected();
	void Connected();
	void Disconnected();

	void SetClient(Client *client);
	Client *GetClient();

	void Lock();
	void Unlock();

	bool GetFrame(switch_frame_t *frame, const char** data, switch_size_t *len);
	void DestroyVideoBuffer();

	const char* GetUUID();
	const char* GetUser();
	const char* GetDomain();
	const char* GetDestNumber();

private:
	// 频道管理
	WSChannel* CreateChannel(switch_core_session_t *session);
	void DestroyChannel(WSChannel* wsChannel);
	bool InitChannel(WSChannel* wsChannel);

	// 解析WebSocket协议
	bool ParseFirstLine(char* line);
	void ParseHeader(char* line);
	bool CheckHandShake();

	/**
	 * 用于发送事件的函数
	 */
	bool ws_connect();
	bool ws_login();
	bool ws_disconnect();

	switch_memory_pool_t *mpPool;

	WSClientState mState;
	WSClientParserCallback* mpCallback;
	Client *mpClient;

	char* mpUser;
	char* mpDomain;
	char* mpDestNumber;
	char* mpSite;
	char* mpCustom;
	char mWebSocketKey[256];

	/**
	 * 当前会话
	 */
	WSChannel *mpChannel;

	/**
	 * RTP frame to H264 frame
	 */
	Rtp2H264VideoTransfer mVideoTransfer;

	/**
	 * 同步锁
	 */
	switch_mutex_t *clientMutex;

	/**
	 * 唯一标识
	 */
	char uuid[SWITCH_UUID_FORMATTED_LENGTH+1];
};

#endif /* SRC_MOD_ENDPOINTS_MOD_WS_WSCLIENTPARSER_H_ */
