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

class WSClientParser;
typedef struct WSChannel {
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

	int ParseData(char* buffer, int len);
	bool GetHandShakeRespond(char** sendBuffer, int& len);
	bool GetPacket(WSPacket* packet, unsigned long long dataLen);

	void SetWSClientParserCallback(WSClientParserCallback* callback);

	/**
	 * 创建和销毁频道
	 */
	WSChannel* CreateCall(
			switch_core_session_t *session,
			const char *profileName,
//			const char *destNumber,
			const char *profileContext,
			const char *profileDialplan,
			const char* ip
			);
	switch_core_session_t* DestroyChannel(bool hangup);
	switch_core_session_t* DestroyChannel(WSChannel* wsChannel, bool hangup);

	bool IsConnected();
	void Disconnected();

	void SetClient(Client *client);
	Client *GetClient();

	void Lock();
	void Unlock();

	bool GetFrame(switch_frame_t *frame, const char** data, switch_size_t *len);
	void DestroyVideoBuffer();

private:
	WSChannel* CreateChannel(switch_core_session_t *session);
	bool InitChannel(WSChannel* wsChannel);
	bool ParseFirstLine(char* line);
	void ParseHeader(char* line);
	bool CheckHandShake();

	switch_memory_pool_t *mpPool;

	WSClientState mState;
	WSClientParserCallback* mpCallback;
	Client *mpClient;

	char* mpUser;
	char* mpDomain;
	char* mpDestNumber;
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
};

#endif /* SRC_MOD_ENDPOINTS_MOD_WS_WSCLIENTPARSER_H_ */
