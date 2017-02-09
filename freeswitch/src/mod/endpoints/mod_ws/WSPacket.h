/*
 * WSPacket.h
 *
 *  Created on: 2016年9月14日
 *      Author: max
 */

#pragma once

#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#pragma pack(push, 1)

#define __SwapInt64(x) \
    ((__uint64_t)((((__uint64_t)(x) & 0xff00000000000000ULL) >> 56) | \
                (((__uint64_t)(x) & 0x00ff000000000000ULL) >> 40) | \
                (((__uint64_t)(x) & 0x0000ff0000000000ULL) >> 24) | \
                (((__uint64_t)(x) & 0x000000ff00000000ULL) >>  8) | \
                (((__uint64_t)(x) & 0x00000000ff000000ULL) <<  8) | \
                (((__uint64_t)(x) & 0x0000000000ff0000ULL) << 24) | \
                (((__uint64_t)(x) & 0x000000000000ff00ULL) << 40) | \
                (((__uint64_t)(x) & 0x00000000000000ffULL) << 56)))

#ifndef ntohll
#define ntohll(x) __SwapInt64(x)
#endif
#ifndef htonll
#define htonll(x) __SwapInt64(x)
#endif

#define MASK_KEY_LEN 16
#define WS_HEADER_MAX_LEN 12
#define WS_HANDSHAKE_MAX_LEN WS_HEADER_MAX_LEN + 1024

typedef enum _tagWS_RSV_TYPE {
	WS_RSV_TYPE1 = 0x40,
	WS_RSV_TYPE2 = 0x20,
	WS_RSV_TYP3 = 0x10,
	WS_RSV_TYPE_DEFAULT = 0x00,
} WS_RSV_TYPE;

typedef enum _tagWS_OPCODE {
	WS_OPCODE_CONTINUATION = 0x00,
	WS_OPCODE_TEXT = 0x01,
	WS_OPCODE_BINARY = 0x02,
	WS_OPCODE_CONNECTION_CLOSE = 0x08,
	WS_OPCODE_PING = 0x09,
	WS_OPCODE_PONG = 0x0A,
	WS_OPCODE_UNKNOW,
} WS_OPCODE;

/**
 *      0                   1                   2                   3
      0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
     +-+-+-+-+-------+-+-------------+-------------------------------+
     |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
     |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
     |N|V|V|V|       |S|             |   (if payload len==126/127)   |
     | |1|2|3|       |K|             |                               |
     +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
     |     Extended payload length continued, if payload len == 127  |
     + - - - - - - - - - - - - - - - +-------------------------------+
     |                               |Masking-key, if MASK set to 1  |
     +-------------------------------+-------------------------------+
     | Masking-key (continued)       |          Payload Data         |
     +-------------------------------- - - - - - - - - - - - - - - - +
     :                     Payload Data continued ...                :
     + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
     |                     Payload Data continued ...                |
     +---------------------------------------------------------------+
 */

/**
 * ##################### Basic Header #####################
 */
/**
 * (2 byte)
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
 +-+-+-+-+-------+-+-------------+
 |F|R|R|R| opcode|M| Payload len |
 |I|S|S|S|  (4)  |A|     (7)     |
 |N|V|V|V|       |S|             |
 | |1|2|3|       |K|             |
 +-+-+-+-+-------+-+-------------+
 */
typedef struct _tagWSBaseHeader {
	char buffer[2];

	_tagWSBaseHeader() {
		Reset();
	}

	_tagWSBaseHeader& operator=(const _tagWSBaseHeader& item) {
		memcpy(buffer, item.buffer, sizeof(buffer));
		return *this;
	}

	void Reset() {
		memset(buffer, 0, sizeof(buffer));
	}

	bool IsFin() {
		// 获取 1 bit
		return (buffer[0] >> 7);
	}

	void SetFin(bool fin) {
		if( fin ) {
			// 赋值 1 bit
			buffer[0] |= 0x80;
		} else {
			// 去除 1 bit
			buffer[0] &= 0x7F;
		}
	}

	WS_RSV_TYPE GetRSVType() {
		// 获取 2-4 bit
		return (WS_RSV_TYPE)(buffer[0] & 0x70);
	}

	void SetRSVType(WS_RSV_TYPE type) {
		// 赋值 2-4 bit
		buffer[0] |= type;
	}

	WS_OPCODE GetOpcode() {
		// 获取 2-4 bit
		return (WS_OPCODE)(buffer[0] & 0x0F);
	}

	void SetOpcode(WS_OPCODE code) {
		// 去除 5-8 bit
		buffer[0] &= 0xF0;
		// 赋值 5-8 bit
		buffer[0] |= code;
	}

	bool IsMask() {
		// 获取 1 bit
		return (buffer[1] >> 7);
	}

	void SetMask(bool mask) {
		if( mask ) {
			// 赋值 1 bit
			buffer[1] |= 0x80;
		} else {
			// 去除 1 bit
			buffer[1] &= 0x7F;
		}
	}

	unsigned short GetPlayloadLength() {
		// 获取 2-8 bit
		return buffer[1] & 0x7F;
	}

	bool SetPlayloadLength(unsigned short playloadLength) {
		if( playloadLength > 127 || playloadLength < 0 ) {
			return false;
		} else {
			// 赋值 2-8 bit
			buffer[1] &= 0x80;
			buffer[1] |= playloadLength;
		}
		return true;
	}

} WSBaseHeader;

/**
 * ##################### Basic Header End #####################
 */

/**
 * ##################### WebSocket Packet #####################
 */
typedef struct _tagWSPacket {
	WSBaseHeader baseHeader;
	char buffer[0];

	_tagWSPacket() {
		Reset();
	}

	_tagWSPacket& operator=(const _tagWSPacket& item) {
		memcpy(&baseHeader, &item.baseHeader, sizeof(baseHeader));
		return *this;
	}

	void Reset() {
		memset(&baseHeader, 0, sizeof(baseHeader));
	}

	unsigned short GetHeaderLength() {
		unsigned short headerLength = sizeof(baseHeader);
		unsigned short playloadLen = baseHeader.GetPlayloadLength();
		if( playloadLen < 126 ) {
			// 7 bit
		} else if( playloadLen == 126 ) {
			// 16 bit
			headerLength += sizeof(unsigned short);
		} else if( playloadLen == 127 ) {
			// 64 bit
			headerLength += sizeof(unsigned long long);
		}

		if( baseHeader.IsMask() ) {
			headerLength += MASK_KEY_LEN;
		}

		return headerLength;
	}

	unsigned long long GetPlayloadLength() {
		unsigned long long playloadLen = (unsigned long long)baseHeader.GetPlayloadLength();
		if( playloadLen < 126 ) {
			// 7 bit
		} else if( playloadLen == 126 ) {
			// 16 bit
			unsigned short *len = (unsigned short*)buffer;
			playloadLen = ntohs(*len);
		} else if( playloadLen == 127 ) {
			// 64 bit
			unsigned long long *len = (unsigned long long*)buffer;
			playloadLen = ntohll(*len);
		}
		return playloadLen;
	}

	void SetPlayloadLength(unsigned long long playloadLength) {
		if( playloadLength < 126 ) {
			// 7 bit
			baseHeader.SetPlayloadLength(playloadLength);
		} else if( playloadLength < USHRT_MAX ) {
			// 16 bit
			baseHeader.SetPlayloadLength(126);
			unsigned short *len = (unsigned short *)buffer;
			*len = htons((unsigned short)playloadLength);
		} else if( playloadLength < ULLONG_MAX ) {
			// 64 bit
			baseHeader.SetPlayloadLength(127);
			unsigned long long *len = (unsigned long long *)buffer;
			*len = htonll(playloadLength);
		}
	}

	char* GetMaskKey() {
		if( baseHeader.IsMask() ) {
			unsigned long long playloadLen = (unsigned long long)baseHeader.GetPlayloadLength();
			if( playloadLen < 126 ) {
				// 7 bit
				return buffer;
			} else if( playloadLen == 126 ) {
				// 16 bit
				return buffer + sizeof(unsigned short);
			} else if( playloadLen == 127 ) {
				// 64 bit
				return buffer + sizeof(unsigned long long);
			}
		}
		return NULL;
	}

	void SetMaskKey(char* key) {
		if( key != NULL ) {
			baseHeader.SetMask(true);
			unsigned long long playloadLen = (unsigned long long)baseHeader.GetPlayloadLength();
			if( playloadLen < 126 ) {
				// 7 bit
			} else if( playloadLen == 126 ) {
				// 16 bit
				memcpy(buffer + sizeof(unsigned short), key, MASK_KEY_LEN);
			} else if( playloadLen == 127 ) {
				// 64 bit
				memcpy(buffer + sizeof(unsigned long long), key, MASK_KEY_LEN);
			}
		} else {
			baseHeader.SetMask(false);
		}
	}

	char* GetData() {
		char* data = buffer;
		unsigned long long playloadLen = (unsigned long long)baseHeader.GetPlayloadLength();
		if( playloadLen < 126 ) {
			// 7 bit
		} else if( playloadLen == 126 ) {
			// 16 bit
			data += sizeof(unsigned short);
		} else if( playloadLen == 127 ) {
			// 64 bit
			data += sizeof(unsigned long long);
		}

		if( baseHeader.IsMask() ) {
			data += MASK_KEY_LEN;
		}

		return data;
	}

} WSPacket;
/**
 * ##################### WebSocket Packet End #####################
 */
#pragma pack(pop)
