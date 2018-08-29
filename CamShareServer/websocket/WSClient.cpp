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

#include <common/md5.h>
#include <common/KLog.h>

#include "ISocketHandler.h"
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

	Init();
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

bool WSClient::IsRunning() {
	return mbRunning;
}

void WSClient::SetWSClientListener(WSClientListener *listener) {
	mpWSClientListener = listener;
}

void WSClient::Init() {
    mbRunning = false;
	mbConnected = false;
	mbShutdown = false;
}

bool WSClient::Connect(const string& hostName, const string& user, const string& dest) {
	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"WSClient::Connect( "
			"index : '%d', "
			"hostName : '%s', "
			"user : '%s', "
			"dest : '%s', "
			"mbRunning : %s "
			")",
			mIndex,
			hostName.c_str(),
			user.c_str(),
			dest.c_str(),
			mbRunning?"true":"false"
			);

	bool bFlag = false;

	if( mbRunning ) {
		FileLog("WSClient",
				"WSClient::Connect( "
				"this : %p, "
				"[Client Already Running], "
				"hostName : '%s' "
				"user : '%s', "
				"dest : '%s' "
				")",
				this,
				hostName.c_str(),
				user.c_str(),
				dest.c_str()
				);
		return false;
	}

	Init();

	hostname = hostName;
	mUser = user;
	mDest = dest;

	mbRunning = true;
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
					"[Tcp connect fail], "
					"index : '%d', "
					"hostName : '%s' "
					")",
					mIndex,
					hostName.c_str()
					);
		}
	}

	if( !bFlag ) {
		FileLog("WSClient",
				"WSClient::Connect( "
				"[Fail], "
				"index : '%d', "
				"hostName : '%s' "
				")",
				mIndex,
				hostName.c_str()
				);
		mbRunning = false;
	} else {
		FileLog("WSClient",
				"WSClient::Connect( "
				"[Success], "
				"index : '%d', "
				"hostName : '%s' "
				")",
				mIndex,
				hostName.c_str()
				);
	}

	return bFlag;
}

void WSClient::Shutdown() {
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"WSClient::Shutdown( "
			"index : '%d', "
			"port : %u, "
			"user : '%s, "
			"dest : '%s' "
			")",
			mIndex,
			m_socketHandler->GetPort(),
			mUser.c_str(),
			mDest.c_str()
			);

	if( !mbShutdown ) {
		mbShutdown = true;
		if( m_socketHandler ) {
			if (ISocketHandler::CONNECTION_STATUS_CONNECTING == m_socketHandler->GetConnectionStatus()) {
				m_socketHandler->Close();
			}
			else {
				m_socketHandler->Shutdown();
			}
		} else {
			if( m_socketHandler ) {
				m_socketHandler->Shutdown();
			}
		}
	}

}

void WSClient::Close() {
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"WSClient::Close( "
			"index : '%d', "
			"port : %u, "
			"user : '%s, "
			"dest : '%s' "
			")",
			mIndex,
			m_socketHandler->GetPort(),
			mUser.c_str(),
			mDest.c_str()
			);

	if( mbRunning ) {
		if( m_socketHandler ) {
			m_socketHandler->Close();
		}
	}

	mbRunning = false;
}

bool WSClient::RecvWSPacket() {
	FileLog("WSClient",
			"WSClient::RecvWSPacket( "
			"this : %p, "
			"####################### Recv ####################### "
			")",
			this
			);

	bool bFlag = true;

	ISocketHandler::HANDLE_RESULT result = ISocketHandler::HANDLE_FAIL;
	unsigned int len = 0;

	char buffer[1024];
	result = m_socketHandler->Recv((void *)buffer, sizeof(buffer), len);
	if( ISocketHandler::HANDLE_FAIL == result ) {
		FileLog("WSClient",
				"WSClient::RecvWSPacket( "
				"[Recv fail], "
				"index : '%d' "
				")",
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
			"GET /%s/%s/%s/1/SID=12346&USERTYPE=1 HTTP/1.1\r\n"
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
		FileLog("WSClient",
				"WSClient::HandShake( "
				"[Send fail], "
				"index : '%d', "
				")",
				mIndex
				);
		return false;
	}

//	result = m_socketHandler->Recv((void *)buffer, WS_HANDSHAKE_SIZE, iLen);
//	if( result == ISocketHandler::HANDLE_FAIL ) {
//		LogManager::GetLogManager()->Log(
//				LOG_MSG,
//				"WSClient::HandShake( "
//				
//				"[Recv fail], "
//				"index : '%d' "
//				")",
//				,
//				mIndex
//				);
//		return false;
//	} else {
//
//	}

	return true;
}
