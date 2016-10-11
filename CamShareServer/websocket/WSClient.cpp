/*
 * WSClient.cpp
 *
 *  Created on: 2016年4月28日
 *      Author: max
 */

#include <string.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ISocketHandler.h"
#include <common/md5.h>

#include "../LogManager.h"
#include "WSClient.h"

#define WS_HANDSHAKE_SIZE 2048
#define WS_DEFAULT_CHUNKSIZE	128

WSClient::WSClient() {
	// TODO Auto-generated constructor stub
	mIndex = -1;
	mUser = "";

	port = 8080;
	hostname = "";

    mbConnected = false;
    mbShutdown = false;

	m_socketHandler = ISocketHandler::CreateSocketHandler(ISocketHandler::TCP_SOCKET);

	mpWSClientListener = NULL;
}

WSClient::~WSClient() {
	// TODO Auto-generated destructor stub
	if( m_socketHandler ) {
		ISocketHandler::ReleaseSocketHandler(m_socketHandler);
	}
}

 int WSClient::GetIndex() {
	return mIndex;
}

void WSClient::SetIndex(int index) {
	mIndex = index;
}

const string& WSClient::GetUser() {
	return mUser;
}

bool WSClient::IsConnected() {
	return mbConnected;
}

void WSClient::SetWSClientListener(WSClientListener *listener) {
	mpWSClientListener = listener;
}

bool WSClient::Connect(const string& hostName, const string& user, const string& dest) {
	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"WSClient::Connect( "
			"tid : %d, "
			"index : '%d', "
			"hostName : %s, "
			"user : %s, "
			"dest : %s "
			")",
			(int)syscall(SYS_gettid),
			mIndex,
			hostName.c_str(),
			user.c_str(),
			dest.c_str()
			);

	hostname = hostName;
	mUser = user;
	mDest = dest;

	bool bFlag = false;

	if( mbConnected || mbShutdown ) {
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"WSClient::Connect( "
				"tid : %d, "
				"[Already Connected or Shutdown], "
				"index : '%d', "
				"hostName : '%s' "
				")",
				(int)syscall(SYS_gettid),
				mIndex,
				hostName.c_str()
				);
		return false;
	}

	if( hostname.length() > 0 ) {
		bFlag = m_socketHandler->Create();
		bFlag = bFlag & m_socketHandler->Connect(hostname, port, 0) == SOCKET_RESULT_SUCCESS;
		if( bFlag ) {
			m_socketHandler->SetBlock(true);
			bFlag = HandShake();
			if( bFlag ) {
				mbConnected = true;
				if( mpWSClientListener != NULL ) {
					mpWSClientListener->OnConnect(this);
				}
			}
		} else {
			LogManager::GetLogManager()->Log(
					LOG_MSG,
					"WSClient::Connect( "
					"tid : %d, "
					"[Tcp connect fail], "
					"index : '%d', "
					"hostName : '%s' "
					")",
					(int)syscall(SYS_gettid),
					mIndex,
					hostName.c_str()
					);
		}
	}

	if( !bFlag ) {
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"WSClient::Connect( "
				"tid : %d, "
				"[Fail], "
				"index : '%d', "
				"hostName : '%s' "
				")",
				(int)syscall(SYS_gettid),
				mIndex,
				hostName.c_str()
				);

	} else {
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"WSClient::Connect( "
				"tid : %d, "
				"[Success], "
				"index : '%d', "
				"hostName : '%s' "
				")",
				(int)syscall(SYS_gettid),
				mIndex,
				hostName.c_str()
				);
	}

	return bFlag;
}

void WSClient::Shutdown() {
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"WSClient::Close( "
			"tid : %d, "
			"index : '%d', "
			"port : %u, "
			"user : '%s, "
			"dest : '%s' "
			")",
			(int)syscall(SYS_gettid),
			mIndex,
			m_socketHandler->GetPort(),
			mUser.c_str(),
			mDest.c_str()
			);
	if( m_socketHandler ) {
		m_socketHandler->Shutdown();
	}

	mbShutdown = true;
}

void WSClient::Close() {
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"WSClient::Close( "
			"tid : %d, "
			"index : '%d', "
			"port : %u, "
			"user : '%s, "
			"dest : '%s' "
			")",
			(int)syscall(SYS_gettid),
			mIndex,
			m_socketHandler->GetPort(),
			mUser.c_str(),
			mDest.c_str()
			);

	if( m_socketHandler ) {
		m_socketHandler->Shutdown();
		m_socketHandler->Close();
	}

	mbShutdown = false;
	mbConnected = false;
}

bool WSClient::RecvWSPacket() {
	if( !mbConnected || mbShutdown ) {
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"RtmpClient::RecvWSPacket( "
				"tid : %d, "
				"[Not connected], "
				"index : '%d' "
				")",
				(int)syscall(SYS_gettid),
				mIndex
				);
		return false;
	}

	bool bFlag = true;

	ISocketHandler::HANDLE_RESULT result = ISocketHandler::HANDLE_FAIL;
	unsigned int len = 0;

	char buffer[1024];
	result = m_socketHandler->Recv((void *)buffer, sizeof(buffer), len);
	if( ISocketHandler::HANDLE_FAIL == result ) {
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"RtmpClient::RecvWSPacket( "
				"tid : %d, "
				"[Recv fail], "
				"index : '%d' "
				")",
				(int)syscall(SYS_gettid),
				mIndex
				);
		Shutdown();
		if( mpWSClientListener != NULL ) {
			mpWSClientListener->OnDisconnect(this);
		}
		bFlag = false;
	}

	return bFlag;
}

bool WSClient::HandShake() {
//	printf("# WSClient::HandShake() \n");
	int i;
	unsigned int iLen;
	uint32_t uptime, suptime;
	int bMatch;

	char type;
	char buffer[WS_HANDSHAKE_SIZE + 1];

	memset(buffer, '0', sizeof(buffer));
	sprintf(buffer,
			"GET /%s/%s/%s HTTP/1.1\r\n"
			"Host: %s:%d"
			"Connection: Upgrade\r\n"
			"Pragma: no-cache\r\n"
			"Cache-Control: no-cache\r\n"
			"Upgrade: websocket\r\n"
			"Origin: http://%s\r\n"
			"Sec-WebSocket-Version: 13\r\n"
			"User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_11_6) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/53.0.2785.116 Safari/537.36\r\n"
			"Accept-Encoding: gzip, deflate, sdch\r\n"
			"Accept-Language: zh-CN,zh;q=0.8,zh-TW;q=0.6,en;q=0.4\r\n"
			"Sec-WebSocket-Key: JrcQQt/f+5WZxRiMmiFq+g==\r\n"
			"Sec-WebSocket-Extensions: permessage-deflate; client_max_window_bits\r\n"
			"\r\n",
			mUser.c_str(),
			hostname.c_str(),
			mDest.c_str(),
			hostname.c_str(),
			port,
			hostname.c_str()
			);

	ISocketHandler::HANDLE_RESULT result = ISocketHandler::HANDLE_FAIL;
	result = m_socketHandler->Send(buffer, strlen(buffer));
	if (result == ISocketHandler::HANDLE_FAIL) {
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"WSClient::HandShake( "
				"tid : %d, "
				"[Send fail], "
				"index : '%d', "
				")",
				(int)syscall(SYS_gettid),
				mIndex
				);
		return false;
	}

//	result = m_socketHandler->Recv((void *)buffer, WS_HANDSHAKE_SIZE, iLen);
//	if( result == ISocketHandler::HANDLE_FAIL ) {
//		LogManager::GetLogManager()->Log(
//				LOG_MSG,
//				"WSClient::HandShake( "
//				"tid : %d, "
//				"[Recv fail], "
//				"index : '%d' "
//				")",
//				(int)syscall(SYS_gettid),
//				mIndex
//				);
//		return false;
//	} else {
//
//	}

	return true;
}
