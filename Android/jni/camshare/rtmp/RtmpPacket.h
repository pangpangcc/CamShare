/*
 * author: Samson.Fan
 *   date: 2015-03-24
 *   file: ITransportPacketHandler.h
 *   desc: 传输包处理接口类
 */

#pragma once

#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>

#pragma pack(push, 1)

typedef enum RTMP_CHANNEL {
	RTMP_CHANNEL_CONTROL = 0x02,		// example:Set Chunk Size 512
	RTMP_CHANNEL_MESSAGE = 0x03,		// example:login()
	RTMP_CHANNEL_VIDEO_AUDIO = 0x06,	// example:Audio Data|Video Data
	RTMP_CHANNEL_PLAY = 0x08,			// example:play('play')
	RTMP_CHANNEL_PUBLISH = 0x14,		// example:publish('publish')
} RTMP_CHANNEL;

typedef enum RTMP_HEADER_CHUNK_TYPE {
	RTMP_HEADER_CHUNK_TYPE_LARGE = 0x00,
	RTMP_HEADER_CHUNK_TYPE_MEDIUM = 0x01,
	RTMP_HEADER_TYPE_SMALL = 0x02,
	RTMP_HEADER_TYPE_MINIMUM = 0x03,
} RTMP_HEADER_CHUNK_TYPE;

typedef enum RTMP_HEADER_MESSAGE_TYPE {
	RTMP_HEADER_MESSAGE_TYPE_SET_CHUNK = 0x01,
	RTMP_HEADER_MESSAGE_TYPE_ACK = 0x03,
	RTMP_HEADER_MESSAGE_TYPE_WINDOW_ACK_SIZE = 0x5,
	RTMP_HEADER_MESSAGE_TYPE_AUDIO = 0x08,
	RTMP_HEADER_MESSAGE_TYPE_VIDEO = 0x09,
	RTMP_HEADER_MESSAGE_TYPE_INVOKE = 0x14,
} RTMP_HEADER_MESSAGE_TYPE;

/**
 Chunk Format
 +--------------+----------------+--------------------+--------------+
 | Basic Header | Message Header | Extended Timestamp | Chunk Data |
 +--------------+----------------+--------------------+--------------+
 | |
 |<------------------- Chunk Header ----------------->|
 */

/**
 * ##################### Chunk Basic Header #####################
 */
/**
 * (1 byte)
 0 1 2 3 4 5 6 7
 +-+-+-+-+-+-+-+-+
 |fmt| cs id |
 +-+-+-+-+-+-+-+-+
 */
typedef struct _tagRtmpBaseHeader {
	unsigned char buffer;

	_tagRtmpBaseHeader() {
		Reset();
	}

	_tagRtmpBaseHeader& operator=(const _tagRtmpBaseHeader& item) {
		buffer = item.buffer;
		return *this;
	}

	void Reset() {
		buffer = 0;
	}

	RTMP_HEADER_CHUNK_TYPE GetChunkType() {
		// 获取高2位
		return (RTMP_HEADER_CHUNK_TYPE)((buffer & 0xC0) >> 6);
	}

	void SetChunkType(RTMP_HEADER_CHUNK_TYPE type) {
		// 去除高2位
		buffer &= 0x3F;
		// 赋值高2位
		char t = ((char)type << 6) & 0xC0;
		buffer |= t;
	}

	unsigned short GetChunkId() {
		// 获取低6位
		return (buffer & 0x3F);
	}

	void SetChunkId(unsigned short chunkId) {
		// 去除低6位
		buffer &= 0xC0;
		// 赋值低6位
		buffer |= (char)chunkId;
	}

} RtmpBaseHeader;
/**
 * ##################### Chunk Basic Header End #####################
 */

/**
 * ##################### Chunk Message Header #####################
 */
/**
 Type 0
 (11 byte)
 type == 0 [RTMP_HEADER_CHUNK_TYPE_LARGE]
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 | timestamp |message length |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 | message length (cont) |message type id| msg stream id |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 | message stream id (cont) |

 Type 1
 (7 byte)
 type == 1 [RTMP_HEADER_CHUNK_TYPE_MEDIUM]
 0 1 2 3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 | timestamp delta |message length |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 | message length (cont) |message type id|
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

 Type 2
 (3 byte)
 type == 2 [RTMP_HEADER_TYPE_SMALL]
 0 1 2
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 | timestamp delta |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

 */
typedef struct _tagRtmpMessageHeader {
	unsigned char buffer[11];

	_tagRtmpMessageHeader() {
		Reset();
	}

	_tagRtmpMessageHeader& operator=(const _tagRtmpMessageHeader& item) {
		memcpy(buffer, item.buffer, sizeof(buffer));
		return *this;
	}

	void Reset() {
		memset(buffer, 0, sizeof(buffer));
	}

	// 3 byte
	unsigned int GetTimestamp() {
		unsigned int* timeStamp = (unsigned int*)buffer;
		return ntohl(*timeStamp) >> 8;
	}

	void SetTimestamp(unsigned int time) {
		char c = buffer[3];
		unsigned int* timeStamp = (unsigned int*)buffer;
		*timeStamp = htonl(time << 8);
		*timeStamp |= htonl((unsigned int)c);
	}

	// 3 byte
	unsigned int GetBodySize() {
		unsigned int* bodySize = (unsigned int*)(buffer + 3);
		return ntohl(*bodySize) >> 8;
	}

	void SetBodySize(unsigned int length) {
		unsigned int* bodySize = (unsigned int*)(buffer + 3);
		RTMP_HEADER_MESSAGE_TYPE type = GetType();
		*bodySize = htonl(length << 8);
		*bodySize |= htonl((unsigned int)type);
	}

	// 1 byte
	RTMP_HEADER_MESSAGE_TYPE GetType() {
		return (RTMP_HEADER_MESSAGE_TYPE)buffer[6];
	}

	void SetType(RTMP_HEADER_MESSAGE_TYPE type) {
		buffer[6] = (char)type;
	}

	// 4 byte
	unsigned int GetStreamId() {
		unsigned int* streamId = (unsigned int*)(buffer + 7);
		return *streamId;
	}

	void SetStreamId(unsigned int stream) {
		unsigned int* streamId = (unsigned int*)(buffer + 7);
		*streamId = stream;
	}

} RtmpMessageHeader;
/**
 * ##################### Chunk Message Header End #####################
 */

typedef struct _tagRtmpPacket {
	RtmpBaseHeader baseHeader;
	RtmpMessageHeader messageHeader;
	char* body;

    unsigned int nBytesRead;

	_tagRtmpPacket() {
		body = NULL;
		nBytesRead = 0;
	}

	_tagRtmpPacket& operator=(const _tagRtmpPacket& item) {
		baseHeader = item.baseHeader;
		messageHeader = item.messageHeader;
		nBytesRead = item.nBytesRead;
		return *this;
	}

	~_tagRtmpPacket() {
	}

	unsigned int GetMessageHeaderLength() {
		unsigned int length = 0;
		switch( baseHeader.GetChunkType() ) {
		case RTMP_HEADER_CHUNK_TYPE_LARGE:{
			length += 11;
		}break;
		case RTMP_HEADER_CHUNK_TYPE_MEDIUM:{
			length += 7;
		}break;
		case RTMP_HEADER_TYPE_SMALL:{
			length += 3;
		}break;
		case RTMP_HEADER_TYPE_MINIMUM:{
			length += 0;
		}break;
		default:{

		}break;
		}

		return length;
	}

	unsigned int GetHeaderLength() {
		unsigned int length = sizeof(baseHeader) + GetMessageHeaderLength();
		return length;
	}

	void AllocBody() {
		if( messageHeader.GetBodySize() > 0 && body == NULL ) {
			body = new char[messageHeader.GetBodySize() + 1];
			nBytesRead = 0;
		}
	}

	void FreeBody() {
		if( body ) {
			delete[] body;
			body = NULL;
		}
		nBytesRead = 0;
	}

} RtmpPacket;

#pragma pack(pop)
