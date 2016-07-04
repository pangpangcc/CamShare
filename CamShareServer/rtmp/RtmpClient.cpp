/*
 * RtmpClient.cpp
 *
 *  Created on: 2016年4月28日
 *      Author: max
 */

#include "RtmpClient.h"

#include <string.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ISocketHandler.h"
#include "amf.h"
#include <common/md5.h>

#include "../LogManager.h"

#define RTMP_SIG_SIZE 1536
#define RTMP_DEFAULT_CHUNKSIZE	128

RtmpClient::RtmpClient() {
	// TODO Auto-generated constructor stub
	mIndex = -1;

	mSession = "";
	port = 1935;
	hostname = "192.168.88.143";
    app = "phone";

    mbConnected = false;
    miNumberInvokes = 0;
    m_outChunkSize = RTMP_DEFAULT_CHUNKSIZE;

	m_socketHandler = ISocketHandler::CreateSocketHandler(ISocketHandler::TCP_SOCKET);

	mpLastSendPacket = NULL;
	mpLastRecvPacket = NULL;
	mpRtmpClientListener = NULL;

	mPublishStreamId = 0;
	mPlayStreamId = 0;
	mTimestamp = 0;
}

RtmpClient::~RtmpClient() {
	// TODO Auto-generated destructor stub
	if( m_socketHandler ) {
		ISocketHandler::ReleaseSocketHandler(m_socketHandler);
	}
}

 int RtmpClient::GetIndex() {
	return mIndex;
}

void RtmpClient::SetIndex(int index) {
	mIndex = index;
}

void RtmpClient::SetRtmpClientListener(RtmpClientListener *listener) {
	mpRtmpClientListener = listener;
}

bool RtmpClient::Connect() {
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"RtmpClient::Connect( "
			"index : %d "
			")",
			mIndex
			);

	bool bFlag = false;

	if( hostname.length() > 0 ) {
		bFlag = m_socketHandler->Create();
		bFlag = bFlag & m_socketHandler->Connect(hostname, port, 0) == SOCKET_RESULT_SUCCESS;
		if( bFlag ) {
			m_socketHandler->SetBlock(true);
			bFlag = HandShake();
			if( bFlag ) {
				mbConnected = true;
				bFlag = SendConnectPacket();
//				if( mpRtmpClientListener != NULL ) {
//					mpRtmpClientListener->OnConnect(this, "");
//				}
			}
		} else {
			LogManager::GetLogManager()->Log(
					LOG_WARNING,
					"RtmpClient::Connect( "
					"index : %d, "
					"TCP connect fail "
					")",
					mIndex
					);
		}
	}

	if( !bFlag ) {
		Close();
	}

	return bFlag;
}

void RtmpClient::Close() {
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"RtmpClient::Close( "
			"index : %d, "
			"port : %u "
			")",
			mIndex,
			m_socketHandler->GetPort()
			);
	mbConnected = false;
	m_socketHandler->Shutdown();
	m_socketHandler->Close();
}

bool RtmpClient::Login(const string& user, const string& password, const string& site, const string& custom) {
//	printf("# RtmpClient::Login( start, user : %s, password : %s ) \n", user.c_str(), password.c_str());
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"RtmpClient::Login( "
			"index : %d, "
			"user : %s, "
			"password : %s, "
			"site : %s, "
			"custom : %s "
			")",
			mIndex,
			user.c_str(),
			password.c_str(),
			site.c_str(),
			custom.c_str()
			);
	bool bFlag = false;

	string temp = "";
	temp += mSession + ":";
	temp += user + ":";
	temp += password;

	char auth[128];
	GetMD5String(temp.c_str(), auth);
	bFlag = SendLoginPacket(user, auth, site, custom);
	if( bFlag ) {
	} else {
		LogManager::GetLogManager()->Log(
				LOG_WARNING,
				"RtmpClient::Login( "
				"index : %d, "
				"fail "
				")",
				mIndex
				);
//		printf("# RtmpClient::Login( fail ) \n");
	}

	return bFlag;
}

bool RtmpClient::MakeCall(const string& dest) {
//	printf("# RtmpClient::MakeCall() \n");
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"RtmpClient::MakeCall( "
			"index : %d, "
			"dest : %s "
			")",
			mIndex,
			dest.c_str()
			);

	bool bFlag = false;

	bFlag = SendMakeCallPacket(dest);
	if( bFlag ) {
	} else {
//		printf("# RtmpClient::MakeCall( fail ) \n");
		LogManager::GetLogManager()->Log(
				LOG_WARNING,
				"RtmpClient::MakeCall( "
				"index : %d, "
				"fail "
				")",
				mIndex
				);
	}
	return bFlag;
}

bool RtmpClient::CreatePublishStream() {
//	printf("# RtmpClient::CreatePublishStream() \n");
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"RtmpClient::CreatePublishStream( "
			"index : %d, "
			")",
			mIndex
			);

	bool bFlag = false;

	bFlag = SendCreateStream(STAND_INVOKE_TYPE_PUBLISH);

	if( bFlag ) {
	} else {
		LogManager::GetLogManager()->Log(
				LOG_WARNING,
				"RtmpClient::CreatePublishStream( "
				"index : %d, "
				"fail "
				")",
				mIndex
				);
//		printf("# RtmpClient::CreatePublishStream( fail ) \n");
	}

	return bFlag;
}

bool RtmpClient::SendVideoData(const char* data, unsigned int len, unsigned int timestamp) {
//	printf("# RtmpClient::SendVideoData( len : %u ) \n", len);
	RtmpPacket packet;
	packet.baseHeader.SetChunkType(RTMP_HEADER_CHUNK_TYPE_LARGE);
	packet.baseHeader.SetChunkId(RTMP_CHANNEL_VIDEO_AUDIO);

	packet.body = (char *)data;

	packet.messageHeader.SetTimestamp(timestamp);
	packet.messageHeader.SetBodySize(len);
	packet.messageHeader.SetType(RTMP_HEADER_MESSAGE_TYPE_VIDEO);
	packet.messageHeader.SetStreamId(mPublishStreamId);

	// 发送包
	if( !SendRtmpPacket(&packet) ) {
		printf("# RtmpClient::SendVideoData( fail ) \n");
		return false;
	}

	return true;

}

unsigned int RtmpClient::GetTime() {
	struct timeval tv;
	gettimeofday(&tv, 0);
	unsigned int t = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
	return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

bool RtmpClient::HandShake() {
//	printf("# RtmpClient::HandShake() \n");
	int i;
	unsigned int iLen;
	uint32_t uptime, suptime;
	int bMatch;

	char type;
	char clientbuf[RTMP_SIG_SIZE + 1], *clientsig = clientbuf + 1;
	char serversig[RTMP_SIG_SIZE];

	memset(clientbuf, '0', sizeof(clientbuf));
	/* not encrypted */
	clientbuf[0] = 0x03;
	uptime = htonl(GetTime());
	memcpy(clientsig, &uptime, 4);
	memset(&clientsig[4], 0, 4);

	for (i = 8; i < RTMP_SIG_SIZE; i++) {
//		clientsig[i] = (char)(rand() % 256);
	}

	ISocketHandler::HANDLE_RESULT result = ISocketHandler::HANDLE_FAIL;
	result = m_socketHandler->Send(clientbuf, RTMP_SIG_SIZE + 1);
	if (result == ISocketHandler::HANDLE_FAIL) {
		LogManager::GetLogManager()->Log(
				LOG_ERR_USER,
				"RtmpClient::HandShake( "
				"index : %d, "
				"first step send fail "
				")",
				mIndex
				);
		return false;
	}

//	printf("# RtmpClient::HandShake( first step send ok ) \n");

	result = m_socketHandler->Recv((void *)(&type), 1, iLen);
	if( result == ISocketHandler::HANDLE_FAIL || (1 != iLen) ) {
		LogManager::GetLogManager()->Log(
				LOG_ERR_USER,
				"RtmpClient::HandShake( "
				"index : %d, "
				"first step recv type fail "
				")",
				mIndex
				);
		return false;
	}

	result = m_socketHandler->Recv((void *)serversig, RTMP_SIG_SIZE, iLen);
	if( result == ISocketHandler::HANDLE_FAIL || (RTMP_SIG_SIZE != iLen) ) {
		LogManager::GetLogManager()->Log(
				LOG_ERR_USER,
				"RtmpClient::HandShake( "
				"index : %d, "
				"first step recv body fail "
				")",
				mIndex
				);
		return false;
	}

//	printf("# RtmpClient::HandShake( first step recv ok ) \n");

	/* decode server response */
	memcpy(&suptime, serversig, 4);
	suptime = ntohl(suptime);

	/* 2nd part of handshake */
	result = m_socketHandler->Send((void *)serversig, RTMP_SIG_SIZE);
	if( result == ISocketHandler::HANDLE_FAIL ) {
		LogManager::GetLogManager()->Log(
				LOG_ERR_USER,
				"RtmpClient::HandShake( "
				"index : %d, "
				"second step send fail "
				")",
				mIndex
				);
		return false;
	}

//	printf("# RtmpClient::HandShake( second step send ok ) \n");

	result = m_socketHandler->Recv((void *)serversig, RTMP_SIG_SIZE, iLen);
	if( result == ISocketHandler::HANDLE_FAIL || (RTMP_SIG_SIZE != iLen) ) {
		LogManager::GetLogManager()->Log(
				LOG_ERR_USER,
				"RtmpClient::HandShake( "
				"index : %d, "
				"second step recv fail "
				")",
				mIndex
				);
		return false;
	}

//	printf("# RtmpClient::HandShake( second step recv ok ) \n");

	bMatch = (memcmp(serversig, clientsig, RTMP_SIG_SIZE) == 0);
	if (!bMatch) {
		LogManager::GetLogManager()->Log(
				LOG_ERR_USER,
				"RtmpClient::HandShake( "
				"index : %d, "
				"handshake check fail "
				")",
				mIndex
				);
		return false;
	}

	return true;
}

bool RtmpClient::SendConnectPacket() {
//	printf("# RtmpClient::SendConnectPacket() \n");
	RtmpPacket packet;
	packet.baseHeader.SetChunkType(RTMP_HEADER_CHUNK_TYPE_LARGE);
	packet.baseHeader.SetChunkId(RTMP_CHANNEL_MESSAGE);
	packet.messageHeader.SetTimestamp(0);

	char buffer[4096], *pend = buffer + sizeof(buffer);
	char *enc;

	packet.body = (char *)buffer;
	enc = packet.body;

	AVal avconnect = {"connect", strlen("connect")};
	enc = AMF_EncodeString(enc, pend, &avconnect);
	enc = AMF_EncodeNumber(enc, pend, ++miNumberInvokes);
	*enc++ = AMF_OBJECT;

	AVal avappName = {"app", strlen("app")};
	AVal avapp = {(char *)app.c_str(), app.length()};
	enc = AMF_EncodeNamedString(enc, pend, &avappName, &avapp);

	AVal avfpad = {"fpad", strlen("fpad")};
	enc = AMF_EncodeNamedBoolean(enc, pend, &avfpad, 0);

	AVal avcapabilities = {"capabilities", strlen("capabilities")};
	enc = AMF_EncodeNamedNumber(enc, pend, &avcapabilities, 239);

	AVal avaudioCodecs = {"audioCodecs", strlen("audioCodecs")};
	enc = AMF_EncodeNamedNumber(enc, pend, &avaudioCodecs, 3575);

	AVal avvideoCodecs = {"videoCodecs", strlen("videoCodecs")};
	enc = AMF_EncodeNamedNumber(enc, pend, &avvideoCodecs, 252);

	AVal avvideoFunction = {"videoFunction", strlen("videoFunction")};
	enc = AMF_EncodeNamedNumber(enc, pend, &avvideoFunction, 1);

	AVal avobjectEncoding = {"objectEncoding", strlen("objectEncoding")};
	enc = AMF_EncodeNamedNumber(enc, pend, &avobjectEncoding, 0);

	/* end of object - 0x00 0x00 0x09 */
	*enc++ = 0;
	*enc++ = 0;
	*enc++ = AMF_OBJECT_END;

	size_t size = enc - packet.body;
	packet.messageHeader.SetBodySize(size);
	packet.messageHeader.SetType(RTMP_HEADER_MESSAGE_TYPE_INVOKE);
	packet.messageHeader.SetStreamId(0);

	// 发送包
	if( !SendRtmpPacket(&packet) ) {
//		printf("# RtmpClient::SendConnectPacket( fail ) \n");
		LogManager::GetLogManager()->Log(
				LOG_ERR_USER,
				"RtmpClient::SendConnectPacket( "
				"index : %d, "
				"fail "
				")",
				mIndex
				);
		miNumberInvokes--;
		return false;
	}

	mpStandInvokeMap.Lock();
	mpStandInvokeMap.Insert(miNumberInvokes, STAND_INVOKE_TYPE_CONNECT);
	mpStandInvokeMap.Unlock();

	return true;
}

bool RtmpClient::SendCreateStream(STAND_INVOKE_TYPE type) {
//	printf("# RtmpClient::SendCreateStream() \n");
	RtmpPacket packet;
	packet.baseHeader.SetChunkType(RTMP_HEADER_CHUNK_TYPE_LARGE);
	packet.baseHeader.SetChunkId(RTMP_CHANNEL_MESSAGE);
	packet.messageHeader.SetTimestamp(0);

	char buffer[4096], *pend = buffer + sizeof(buffer);
	char *enc;

	packet.body = (char *)buffer;
	enc = packet.body;

	AVal avcreateStream = {"createStream", strlen("createStream")};
	enc = AMF_EncodeString(enc, pend, &avcreateStream);
	enc = AMF_EncodeNumber(enc, pend, ++miNumberInvokes);
	*enc++ = AMF_NULL;

	size_t size = enc - packet.body;
	packet.messageHeader.SetBodySize(size);
	packet.messageHeader.SetType(RTMP_HEADER_MESSAGE_TYPE_INVOKE);
	packet.messageHeader.SetStreamId(0);

	// 发送包
	if( !SendRtmpPacket(&packet) ) {
		printf("# RtmpClient::SendCreateStream( fail ) \n");
		miNumberInvokes--;
		return false;
	}

	mpStandInvokeMap.Lock();
	mpStandInvokeMap.Insert(miNumberInvokes, type);
	mpStandInvokeMap.Unlock();

	return true;
}

bool RtmpClient::SendPublishPacket() {
	RtmpPacket packet;
	packet.baseHeader.SetChunkType(RTMP_HEADER_CHUNK_TYPE_LARGE);
	packet.baseHeader.SetChunkId(0x3);
	packet.messageHeader.SetTimestamp(0);

	char buffer[4096], *pend = buffer + sizeof(buffer);
	char *enc;

	packet.body = (char *)buffer;
	enc = packet.body;

	AVal avpublish = {"publish", strlen("publish")};
	enc = AMF_EncodeString(enc, pend, &avpublish);
	enc = AMF_EncodeNumber(enc, pend, 0);
	*enc++ = AMF_NULL;
	AVal avpublishValue = {"publish", strlen("publish")};
	enc = AMF_EncodeString(enc, pend, &avpublishValue);
	AVal avlive = {"live", strlen("live")};
	enc = AMF_EncodeString(enc, pend, &avlive);

	size_t size = enc - packet.body;
	packet.messageHeader.SetBodySize(size);
	packet.messageHeader.SetType(RTMP_HEADER_MESSAGE_TYPE_INVOKE);
	packet.messageHeader.SetStreamId(mPublishStreamId);

	// 发送包
	if( !SendRtmpPacket(&packet) ) {
		printf("# RtmpClient::SendPublishPacket( fail ) \n");
		return false;
	}

	return true;
}

bool RtmpClient::SendLoginPacket(const string& user, const string& auth, const string& site, const string& custom) {
//	printf("# RtmpClient::SendLoginPacket() \n");
	RtmpPacket packet;
	packet.baseHeader.SetChunkType(RTMP_HEADER_CHUNK_TYPE_LARGE);
	packet.baseHeader.SetChunkId(RTMP_CHANNEL_MESSAGE);
	packet.messageHeader.SetTimestamp(0);

	char buffer[4096], *pend = buffer + sizeof(buffer);
	char *enc;

	packet.body = (char *)buffer;
	enc = packet.body;

	AVal avlogin = {"login", strlen("login")};
	enc = AMF_EncodeString(enc, pend, &avlogin);
	enc = AMF_EncodeNumber(enc, pend, 0);
	*enc++ = AMF_NULL;

	AVal avuser = {(char *)user.c_str(), user.length()};
	enc = AMF_EncodeString(enc, pend, &avuser);

	AVal avauth = {(char *)auth.c_str(), auth.length()};
	enc = AMF_EncodeString(enc, pend, &avauth);

	AVal avsite = {(char *)site.c_str(), site.length()};
	enc = AMF_EncodeString(enc, pend, &avsite);

	AVal avcustom = {(char *)custom.c_str(), custom.length()};
	enc = AMF_EncodeString(enc, pend, &avcustom);

	size_t size = enc - packet.body;
	packet.messageHeader.SetBodySize(size);
	packet.messageHeader.SetType(RTMP_HEADER_MESSAGE_TYPE_INVOKE);
	packet.messageHeader.SetStreamId(0);

	// 发送包
	if( !SendRtmpPacket(&packet) ) {
		printf("# RtmpClient::SendLoginPacket( fail ) \n");
		return false;
	}

	return true;

}

bool RtmpClient::SendMakeCallPacket(const string& dest) {
//	printf("# RtmpClient::SendMakeCallPacket() \n");
	RtmpPacket packet;
	packet.baseHeader.SetChunkType(RTMP_HEADER_CHUNK_TYPE_LARGE);
	packet.baseHeader.SetChunkId(RTMP_CHANNEL_MESSAGE);
	packet.messageHeader.SetTimestamp(0);

	char buffer[4096], *pend = buffer + sizeof(buffer);
	char *enc;

	packet.body = (char *)buffer;
	enc = packet.body;

	AVal avmakeCall = {"makeCall", strlen("makeCall")};
	enc = AMF_EncodeString(enc, pend, &avmakeCall);
	enc = AMF_EncodeNumber(enc, pend, 0);
	*enc++ = AMF_NULL;

	AVal avdest = {(char *)dest.c_str(), dest.length()};
	enc = AMF_EncodeString(enc, pend, &avdest);

	AVal avunKnow = {"", 0};
	enc = AMF_EncodeString(enc, pend, &avunKnow);

	*enc++ = AMF_OBJECT;

	AVal avincomingBandwidthName = {"incomingBandwidth", strlen("incomingBandwidth")};
	AVal avincomingBandwidth = {"1mb", strlen("1mb")};
	enc = AMF_EncodeNamedString(enc, pend, &avincomingBandwidthName, &avincomingBandwidth);

	AVal avwantVideoName = {"wantVideo", strlen("wantVideo")};
	AVal avwantVideo = {"true", strlen("true")};
	enc = AMF_EncodeNamedString(enc, pend, &avwantVideoName, &avwantVideo);

	/* end of object - 0x00 0x00 0x09 */
	*enc++ = 0;
	*enc++ = 0;
	*enc++ = AMF_OBJECT_END;

	size_t size = enc - packet.body;
	packet.messageHeader.SetBodySize(size);
	packet.messageHeader.SetType(RTMP_HEADER_MESSAGE_TYPE_INVOKE);
	packet.messageHeader.SetStreamId(0);

	// Send Packet
	if( !SendRtmpPacket(&packet) ) {
		printf("# RtmpClient::SendMakeCallPacket( fail ) \n");
		return false;
	}

	return true;
}

bool RtmpClient::SendRtmpPacket(RtmpPacket* packet) {
//	printf("# RtmpClient::SendRtmpPacket() \n");

	if( !mbConnected ) {
		return false;
	}

	// Change to Absolute Timestamp
	unsigned int timestamp = packet->messageHeader.GetTimestamp() + (GetTime() - mTimestamp);
	packet->messageHeader.SetTimestamp(timestamp);

	unsigned int streamId = -1;
	unsigned int lastTimestamp = 0;
	if( mpLastSendPacket != NULL ) {
		streamId = mpLastSendPacket->messageHeader.GetStreamId();
		lastTimestamp = mpLastSendPacket->messageHeader.GetTimestamp();

	} else {
		// Record Last Send Packet
		mpLastSendPacket = new RtmpPacket();
	}

	// Dump packet
	*mpLastSendPacket = *packet;

	// Same Stream
	if( streamId == packet->messageHeader.GetStreamId() ) {
		// Modify ChunkType to Medium
		packet->baseHeader.SetChunkType(RTMP_HEADER_CHUNK_TYPE_MEDIUM);

		// Calculate Timestamp delta
		timestamp = packet->messageHeader.GetTimestamp() - lastTimestamp;

		// Modify to delta Timestamp
		packet->messageHeader.SetTimestamp(timestamp);
	}

	// Dump Packet
//	DumpRtmpPacket(packet);

	// Send Header
	ISocketHandler::HANDLE_RESULT result = ISocketHandler::HANDLE_FAIL;
	result = m_socketHandler->Send((void *)packet, packet->GetHeaderLength());
	if (result == ISocketHandler::HANDLE_FAIL) {
//		printf("# RtmpClient::SendRtmpPacket( Send Header Fail ) \n");
		LogManager::GetLogManager()->Log(
				LOG_ERR_USER,
				"RtmpClient::SendRtmpPacket( "
				"index : %d, "
				"Send Header Fail "
				")",
				mIndex
				);
		Close();
		if( mpRtmpClientListener != NULL ) {
			mpRtmpClientListener->OnDisconnect(this);
		}
		return false;
	}

//	unsigned char *p = (unsigned char *)packet;
//	if( p != NULL && packet->GetHeaderLength() > 0 ) {
//		printf("############################## RTMP RAW #############################\n");
//		for(int i = 0; i < packet->GetHeaderLength(); i++) {
//			printf("%02x ", p[i]);
//		}
//	}
//
//	if( packet->body != NULL && packet->messageHeader.GetBodySize() > 0 ) {
//		for(int i = 0; i < packet->messageHeader.GetBodySize(); i++) {
//			printf("%02x ", ((unsigned char*)packet->body)[i]);
//		}
//		printf("\n############################## RTMP RAW #############################\n");
//	}

	// Send Body
	unsigned int last = packet->messageHeader.GetBodySize();
	unsigned int send = (m_outChunkSize < last)?m_outChunkSize:last;
	unsigned int index = 0;
	while( last > 0 ) {
//		printf("# RtmpClient::SendRtmpPacket( "
//				"m_outChunkSize : %u, "
//				"last : %u, "
//				"send : %u, "
//				"index : %u "
//				")\n",
//				m_outChunkSize,
//				last,
//				send,
//				index
//				);

		// Send Raw Body
		if( packet->body != NULL && index < packet->messageHeader.GetBodySize() ) {
			result = m_socketHandler->Send((void *)(packet->body + index), send);
			if (result == ISocketHandler::HANDLE_FAIL) {
//				printf("# RtmpClient::SendRtmpPacket( Send Body Fail ) \n");
				LogManager::GetLogManager()->Log(
						LOG_ERR_USER,
						"RtmpClient::SendRtmpPacket( "
						"index : %d, "
						"Send Body Fail "
						")",
						mIndex
						);
				Close();
				if( mpRtmpClientListener != NULL ) {
					mpRtmpClientListener->OnDisconnect(this);
				}
				return false;
			}
		}

		index += send;
		last -= send;
		send = (m_outChunkSize < last)?m_outChunkSize:last;

		// Send Chunk Separate
		if( last > 0 ) {
			unsigned char c = (0xc0 | packet->baseHeader.buffer);
//			printf("# RtmpClient::SendRtmpPacket( Send Chunk Separate : %x )\n", c);
			result = m_socketHandler->Send((void *)&c, 1);
			if (result == ISocketHandler::HANDLE_FAIL) {
//				printf("# RtmpClient::SendRtmpPacket( Send Chunk Separate Fail ) \n");
				LogManager::GetLogManager()->Log(
						LOG_ERR_USER,
						"RtmpClient::SendRtmpPacket( "
						"index : %d, "
						"Send Chunk Separate Fail "
						")",
						mIndex
						);
				Close();
				if( mpRtmpClientListener != NULL ) {
					mpRtmpClientListener->OnDisconnect(this);
				}
				return false;
			}
		}
	}

	return true;
}

bool RtmpClient::RecvRtmpPacket(RtmpPacket* packet) {
//	printf("# RtmpClient::RecvRtmpPacket() \n");

	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"RtmpClient::RecvRtmpPacket( "
			"index : %d "
			")",
			mIndex
			);

	if( !mbConnected ) {
		LogManager::GetLogManager()->Log(
				LOG_STAT,
				"RtmpClient::RecvRtmpPacket( "
				"index : %d, "
				"Disconnected "
				")",
				mIndex
				);
		m_socketHandler->Close();
		return false;
	}

	ISocketHandler::HANDLE_RESULT result = ISocketHandler::HANDLE_FAIL;
	unsigned int len = 0;

	// Read Base Header
	result = m_socketHandler->Recv((void *)&(packet->baseHeader.buffer), 1, len);
	if( ISocketHandler::HANDLE_FAIL == result || (1 != len) ) {
//		printf("# RtmpClient::RecvRtmpPacket( Read Base Header Header fail ) \n");
		LogManager::GetLogManager()->Log(
				LOG_ERR_USER,
				"RtmpClient::RecvRtmpPacket( "
				"index : %d, "
				"Read Base Header Header fail "
				")",
				mIndex
				);
		Close();
		if( mpRtmpClientListener != NULL ) {
			mpRtmpClientListener->OnDisconnect(this);
		}
		return false;
	}

	// Read Message Header
	if( packet->GetMessageHeaderLength() > 0 ) {
		result = m_socketHandler->Recv((void *)&(packet->messageHeader.buffer), packet->GetMessageHeaderLength(), len);
		if( ISocketHandler::HANDLE_FAIL == result || (packet->GetMessageHeaderLength() != len) ) {
//			printf("# RtmpClient::RecvRtmpPacket( Read Message Header fail ) \n");
			LogManager::GetLogManager()->Log(
					LOG_ERR_USER,
					"RtmpClient::RecvRtmpPacket( "
					"index : %d, "
					"Read Message Header Header fail "
					")",
					mIndex
					);
			Close();
			if( mpRtmpClientListener != NULL ) {
				mpRtmpClientListener->OnDisconnect(this);
			}
			return false;
		}
	}

	// Read Body
	switch(packet->baseHeader.GetChunkType()) {
	case RTMP_HEADER_CHUNK_TYPE_LARGE:{
		// Record Last Recv Large Packet
		if( mpLastRecvPacket == NULL ) {
			mpLastRecvPacket = new RtmpPacket();
		}
		*mpLastRecvPacket = *packet;

	}break;
	case RTMP_HEADER_CHUNK_TYPE_MEDIUM:{
		if( mpLastRecvPacket != NULL ) {
			unsigned int delta = packet->messageHeader.GetTimestamp();
			packet->messageHeader.SetTimestamp(delta + mpLastRecvPacket->messageHeader.GetBodySize());
		}

	}break;
	case RTMP_HEADER_TYPE_SMALL:{
		if( mpLastRecvPacket != NULL ) {
			packet->messageHeader.SetBodySize(mpLastRecvPacket->messageHeader.GetBodySize());
			unsigned int delta = packet->messageHeader.GetTimestamp();
			packet->messageHeader.SetTimestamp(delta + mpLastRecvPacket->messageHeader.GetBodySize());
		}
	}break;
	case RTMP_HEADER_TYPE_MINIMUM:{

	}break;
	default:break;
	}

	if( packet->messageHeader.GetBodySize() > 0 ) {
		packet->AllocBody();
		result = m_socketHandler->Recv((void *)(packet->body), packet->messageHeader.GetBodySize(), len);
		if( ISocketHandler::HANDLE_FAIL == result || (packet->messageHeader.GetBodySize() != len) ) {
//			printf("# RtmpClient::RecvRtmpPacket( Read Body fail ) \n");
			LogManager::GetLogManager()->Log(
					LOG_ERR_USER,
					"RtmpClient::RecvRtmpPacket( "
					"index : %d, "
					"Read Body fail "
					")",
					mIndex
					);
			Close();
			if( mpRtmpClientListener != NULL ) {
				mpRtmpClientListener->OnDisconnect(this);
			}
			return false;
		}
	}

	return true;
}

RTMP_PACKET_TYPE RtmpClient::ParseRtmpPacket(RtmpPacket* packet) {
	RTMP_PACKET_TYPE type = RTMP_PACKET_TYPE_UNKNOW;
//	printf("# RtmpClient::ParseRtmpPacket() \n");
//	DumpRtmpPacket(packet);

	switch(packet->messageHeader.GetType()) {
	case RTMP_HEADER_MESSAGE_TYPE_INVOKE:{
//		printf("# RtmpClient::ParseRtmpPacket( INVOKE ) \n");
		AMFObject obj;
		AMF_Decode(&obj, packet->body, packet->messageHeader.GetBodySize(), FALSE);

		AVal method;
		AMFProp_GetString(AMF_GetProp(&obj, NULL, 0), &method);

		char temp[2048];
		if( memcmp(method.av_val, "_result", strlen("_result")) == 0 ) {
			// Parse _result
			int seq = (int)AMFProp_GetNumber(AMF_GetProp(&obj, NULL, 1));
//			printf("# RtmpClient::ParseRtmpPacket( INVOKE : _result, seq : %u ) \n", seq);

			STAND_INVOKE_TYPE invoke_type;
			mpStandInvokeMap.Lock();
			StandInvokeMap::iterator itr = mpStandInvokeMap.Find(seq);
			if( itr != mpStandInvokeMap.End() ) {
				invoke_type = itr->second;
				mpStandInvokeMap.Erase(itr);
			}
			mpStandInvokeMap.Unlock();

			switch(invoke_type) {
			case STAND_INVOKE_TYPE_CONNECT:{
//				printf("# RtmpClient::ParseRtmpPacket( INVOKE : _result, connect ) \n");

			}break;
			case STAND_INVOKE_TYPE_PUBLISH:{
				int streamId = (int)AMFProp_GetNumber(AMF_GetProp(&obj, NULL, 3));
//				printf("# RtmpClient::ParseRtmpPacket( INVOKE : _result, publish, streamId : %u ) \n", streamId);
				mPublishStreamId = streamId;
				mTimestamp = GetTime();
				SendPublishPacket();
				if( mpRtmpClientListener != NULL ) {
					mpRtmpClientListener->OnCreatePublishStream(this);
				}

			}break;
			case STAND_INVOKE_TYPE_PLAY:{
				int streamId = (int)AMFProp_GetNumber(AMF_GetProp(&obj, NULL, 3));
//				printf("# RtmpClient::ParseRtmpPacket( INVOKE : _result, play, streamId : %u ) \n", streamId);

			}break;
			}

		} else if( memcmp(method.av_val, "connected", strlen("connected")) == 0 ) {
			// Parse connected
//			printf("# RtmpClient::ParseRtmpPacket( INVOKE : connected ) \n");
			type = RTMP_PACKET_TYPE_CONNECTED;

			AVal session;
			AMFProp_GetString(AMF_GetProp(&obj, NULL, 3), &session);

			memcpy(temp, session.av_val, session.av_len);
			temp[session.av_len] = '\0';
			mSession = temp;

			LogManager::GetLogManager()->Log(
					LOG_MSG,
					"RtmpClient::OnConnect( "
					"index : %d, "
					"sessionId : %s "
					")",
					mIndex,
					mSession.c_str()
					);

			if( mpRtmpClientListener != NULL ) {
				mpRtmpClientListener->OnConnect(this, mSession);
			}

		} else if( memcmp(method.av_val, "onLogin", strlen("onLogin")) == 0 ) {
			// Parse onLogin
//			printf("# RtmpClient::ParseRtmpPacket( INVOKE : onLogin ) \n");
			AVal loginResult;
			AMFProp_GetString(AMF_GetProp(&obj, NULL, 3), &loginResult);

			bool bFlag = false;
			if( memcmp(loginResult.av_val, "success", strlen("success")) == 0 ) {
				bFlag = true;
			}

			LogManager::GetLogManager()->Log(
					LOG_MSG,
					"RtmpClient::onLogin( "
					"index : %d, "
					"bFlag : %s "
					")",
					mIndex,
					bFlag?"true":"false"
					);

			if( mpRtmpClientListener != NULL ) {
				mpRtmpClientListener->OnLogin(this, bFlag);
			}
		}

		// Parse onMakeCall
		if( memcmp(method.av_val, "onMakeCall", strlen("onMakeCall")) == 0 ) {
			LogManager::GetLogManager()->Log(
					LOG_MSG,
					"RtmpClient::onMakeCall( "
					"index : %d "
					")",
					mIndex
					);
//			printf("# RtmpClient::ParseRtmpPacket( INVOKE : onMakeCall ) \n");
			if( mpRtmpClientListener != NULL ) {
				mpRtmpClientListener->OnMakeCall(this, true);
			}
		}

	}break;
	default:{
//		printf("# RtmpClient::ParseRtmpPacket( UNKNOW ) \n");
		break;
	}
	}

	return type;
}

void RtmpClient::DumpRtmpPacket(RtmpPacket* packet) {
	printf("# RtmpClient::DumpRtmpPacket( \n"
			"Format : %u \n"
			"Chunk Stream ID : %u \n"
			"Timestamp : %u \n"
			"Body Size : %u \n"
			"Type ID : 0x%02x \n"
			"Stream ID : %u \n"
			") \n",
			packet->baseHeader.GetChunkType(),
			packet->baseHeader.GetChunkId(),
			packet->messageHeader.GetTimestamp(),
			packet->messageHeader.GetBodySize(),
			packet->messageHeader.GetType(),
			packet->messageHeader.GetStreamId()
			);
}
