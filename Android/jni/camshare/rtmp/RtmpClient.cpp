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

#include <common/md5.h>
#include <common/Arithmetic.h>

#include "ISocketHandler.h"
#include "amf.h"

#define RTMP_SIG_SIZE 1536
#define RTMP_DEFAULT_CHUNKSIZE	128
#define RTMP_DEFAULT_ACK_WINDOW 0x200000

#define MAX_RTP_PAYLOAD_SIZE 1400

#define MAX_ENCODE_SIZE 20000

#define AMF_MAX_SIZE      2048 * 16 * 2

typedef struct RtmpBuffer {
	RtmpBuffer() {
		data = NULL;
		len = 0;
	}
	RtmpBuffer(const RtmpBuffer* item) {
		this->data = NULL;
		this->len = 0;

		if( item && item->data != NULL ) {
			this->data = new unsigned char[item->len + 1];
			this->len = item->len;

			memcpy(this->data, item->data, item->len);
			this->data[len] = '\0';
		}
	}
	~RtmpBuffer() {
		if( data ) {
			delete[] data;
			data = NULL;
		}
	}
	RtmpBuffer(unsigned char* data, unsigned int len) {
		this->data = new unsigned char[len + 1];
		this->len = len;

		memcpy(this->data, data, len);
		this->data[len] = '\0';
	}

	unsigned char* data;
	unsigned int len;

}RtmpBuffer;

typedef struct RTMP2RTP_HELPER_S {
	RTMP2RTP_HELPER_S() {
		sps = NULL;
		pps = NULL;
		lenSize = 0;
	}

	RTMP2RTP_HELPER_S(const RTMP2RTP_HELPER_S& item) {
		sps = new RtmpBuffer(item.sps);
		pps = new RtmpBuffer(item.pps);
		std::copy(item.nal_list.begin(), item.nal_list.end(), std::back_inserter(nal_list));
		this->lenSize = item.lenSize;
	}

	~RTMP2RTP_HELPER_S() {
		if( sps ) {
			delete sps;
			sps = NULL;
		}

		if( pps ) {
			delete pps;
			pps = NULL;
		}

		for(list<RtmpBuffer*>::const_iterator itr = nal_list.begin(); itr != nal_list.end(); itr++) {
			RtmpBuffer* buffer = *itr;
			delete buffer;
		}
		nal_list.clear();
	}

	RtmpBuffer	*sps;
	RtmpBuffer	*pps;
	list<RtmpBuffer*> nal_list;
	uint32_t	lenSize;

} RTMP2RTP_HELPER_T;

typedef struct RTP2RTMP_HELPER_S {
	RTP2RTMP_HELPER_S() {
	    rtmp_buf = NULL; //  视频帧的数据
		fua_buf = NULL;  // ??
		sps = NULL;
		pps = NULL;
		avc_conf = NULL;
		send     = false;
		send_avc = false;
		last_recv_ts = 0;
		last_seq     = 0;
		last_mark    = 0;
		sps_changed  = false;
		timestamp    = 0;
	}

	RTP2RTMP_HELPER_S(const RTP2RTMP_HELPER_S& item) {
		rtmp_buf = new RtmpBuffer(item.rtmp_buf);
		fua_buf = new RtmpBuffer(item.fua_buf);
		sps = new RtmpBuffer(item.sps);
		pps = new RtmpBuffer(item.pps);
		avc_conf = new RtmpBuffer(item.avc_conf);
		send     = item.send;
		send_avc = item.send_avc;
		last_recv_ts = item.last_recv_ts;
		last_seq     = item.last_seq;
		last_mark    = item.last_mark;
		sps_changed  = item.sps_changed;
		timestamp    = item.timestamp;
	}

	~RTP2RTMP_HELPER_S() {
		if( sps ) {
			delete sps;
			sps = NULL;
		}

		if( pps ) {
			delete pps;
			pps = NULL;
		}

		if(avc_conf){
			delete avc_conf;
			avc_conf = NULL;
		}


	}


	RtmpBuffer *rtmp_buf; //  视频帧的数据
	RtmpBuffer *fua_buf;  // ??
	RtmpBuffer	*sps;     // sps的数据
	RtmpBuffer	*pps;     // pps的数据
	RtmpBuffer	*avc_conf;  // sps和pps的数据
	bool        send;       // 是否发送视频帧数据
	bool        send_avc;   // 是否发送sps和pps的数据
	uint32_t last_recv_ts; // 时间
	uint16_t last_seq;     // 序列好
	uint8_t  last_mark; //
	uint32_t    sps_changed; // 是否sps改变了

	//switch_buffer_t *rtmp_buf;
	//switch_buffer_t *fua_buf; //fu_a buf
	int    timestamp;
} RTP2RTMP_HELPER_T;

RtmpClient::RtmpClient() {
	// TODO Auto-generated constructor stub
	FileLog("RtmpClient",
			"RtmpClient::RtmpClient( "
			"this : %p "
			")",
			this
			);

	m_socketHandler = ISocketHandler::CreateSocketHandler(ISocketHandler::TCP_SOCKET);

	mpRtmpClientListener = NULL;

	Init();
}

RtmpClient::~RtmpClient() {
	// TODO Auto-generated destructor stub
	FileLog("RtmpClient",
			"RtmpClient::~RtmpClient( "
			"this : %p "
			")",
			this
			);
	Close();

	if( m_socketHandler ) {
		ISocketHandler::ReleaseSocketHandler(m_socketHandler);
	}
}

const string& RtmpClient::GetUser() {
	return mUser;
}

const string& RtmpClient::GetDest() {
	return mDest;
}

bool RtmpClient::IsRunning() {
	return mbRunning;
}

void RtmpClient::SetRtmpClientListener(RtmpClientListener *listener) {
	mpRtmpClientListener = listener;
}

bool RtmpClient::Connect(const string& hostName) {
	FileLog("RtmpClient",
			"RtmpClient::Connect( "
			"hostName : %s, "
			"mbRunning : %s "
			")",
			hostName.c_str(),
			mbRunning?"true":"false"
			);
	bool bFlag = false;

	if( mbRunning ) {
		FileLog("RtmpClient",
				"RtmpClient::Connect( "
				"[Client Already Running], "
				"hostName : '%s' "
				")",
				hostName.c_str()
				);
		return false;
	}

	Init();

	mbRunning = true;
	this->hostName = hostName;
	if( hostName.length() > 0 ) {
		bFlag = m_socketHandler->Create();
		bFlag = bFlag & m_socketHandler->Connect(hostName, port, 10 * 1000) == SOCKET_RESULT_SUCCESS;
		if( bFlag ) {
			m_socketHandler->SetBlock(true);
			bFlag = HandShake();
			if( bFlag ) {
				mbConnected = true;
				bFlag = SendConnectPacket();
			}
		} else {
			FileLog("RtmpClient",
					"RtmpClient::Connect( "
					"[Tcp connect fail], "
					"hostName : '%s' "
					")",
					hostName.c_str()
					);
		}
	}

	if( !bFlag ) {
		FileLog("RtmpClient",
				"RtmpClient::Connect( "
				"[Fail], "
				"hostName : '%s' "
				")",
				hostName.c_str()
				);
		mbRunning = false;
	} else {
		FileLog("RtmpClient",
				"RtmpClient::Connect( "
				"[Success], "
				"hostName : '%s' "
				")",
				hostName.c_str()
				);
	}

	return bFlag;
}

void RtmpClient::Shutdown() {
	FileLog("RtmpClient",
			"RtmpClient::Shutdown( "
			"mbConnected : %s, "
			"port : %u, "
			"user : '%s, "
			"dest : '%s' "
			")",
			mbConnected?"true":"false",
			port,
			mUser.c_str(),
			mDest.c_str()
			);

	if( !mbConnected ) {
		if( m_socketHandler ) {
			m_socketHandler->Shutdown();
			m_socketHandler->Close();
		}
	} else {
		if( m_socketHandler ) {
			m_socketHandler->Shutdown();
		}
	}

}

void RtmpClient::Close() {
	FileLog("RtmpClient",
			"RtmpClient::Close( "
			"mbRunning : %s, "
			"port : %u, "
			"user : '%s, "
			"dest : '%s' "
			")",
			mbRunning?"true":"false",
			port,
			mUser.c_str(),
			mDest.c_str()
			);

	if( mbRunning ) {
		if( m_socketHandler ) {
			m_socketHandler->Shutdown();
			m_socketHandler->Close();
		}

		for(int i = 0; i < RTMP_CHANNELS; i++) {
			RtmpPacket* pLastRecvPacket = mChannelsIn[i];
			if( pLastRecvPacket ) {
				delete pLastRecvPacket;
				mChannelsIn[i] = NULL;
			}

			RtmpPacket* pLastSendPacket = mChannelsOut[i];
			if( pLastSendPacket ) {
				delete pLastSendPacket;
				mChannelsOut[i] = NULL;
			}
		}

		if( mReadVideoHelper ) {
			delete mReadVideoHelper;
			mReadVideoHelper = NULL;
		}

		if(mPacketVideoHelper){
			delete mPacketVideoHelper;
			mPacketVideoHelper = NULL;
		}
	}

	mbRunning = false;
}

bool RtmpClient::Login(const string& user, const string& password, const string& site, const string& custom) {
	FileLog("RtmpClient",
			"RtmpClient::Login( "
			"user : '%s, "
			"password : %s, "
			"site : %s, "
			"custom : %s "
			")",
			user.c_str(),
			password.c_str(),
			site.c_str(),
			custom.c_str()
			);
	bool bFlag = false;

	char userWithHost[1024];
	sprintf(userWithHost, "%s@%s", user.c_str(), hostName.c_str());

	char authString[1024];
	sprintf(authString, "%s:%s:%s", mSession.c_str(), userWithHost, password.c_str());

	char auth[128];
	GetMD5String(authString, auth);
	bFlag = SendLoginPacket(userWithHost, auth, site, custom);
	if( bFlag ) {
		FileLog("RtmpClient",
				"RtmpClient::Login( "
				"[Success], "
				"user : '%s' "
				")",
				user.c_str()
				);
		mUser = user;

	} else {
		FileLog("RtmpClient",
				"RtmpClient::Login( "
				"[Fail], "
				"user : '%s' "
				")",
				user.c_str()
				);
	}

	return bFlag;
}

bool RtmpClient::MakeCall(const string& dest) {
	FileLog("RtmpClient",
			"RtmpClient::MakeCall( "
			"user : '%s, "
			"dest : '%s' "
			")",
			mUser.c_str(),
			dest.c_str()
			);

	bool bFlag = false;
	mDest = dest;

	bFlag = SendMakeCallPacket(dest);
	if( bFlag ) {
		FileLog("RtmpClient",
				"RtmpClient::MakeCall( "
				"[Success], "
				"user : '%s, "
				"dest : '%s' "
				")",
				mUser.c_str(),
				dest.c_str()
				);

		// 创建视频转换器
		if( mReadVideoHelper ) {
			delete mReadVideoHelper;
			mReadVideoHelper = NULL;
		}
		mReadVideoHelper = new RTMP2RTP_HELPER_T();


		// 创建视频打包器
		if( mPacketVideoHelper ) {
			delete mPacketVideoHelper;
			mPacketVideoHelper = NULL;
		}
		mPacketVideoHelper = new RTP2RTMP_HELPER_T();

	} else {
		FileLog("RtmpClient",
				"RtmpClient::MakeCall( "
				"[Fail], "
				"user : '%s, "
				"dest : '%s' "
				")",
				mUser.c_str(),
				dest.c_str()
				);
	}
	return bFlag;
}

bool RtmpClient::Hangup() {
	FileLog("RtmpClient",
			"RtmpClient::Hangup( "
			"user : '%s, "
			"dest : '%s' "
			")",
			mUser.c_str(),
			mDest.c_str()
			);

	bool bFlag = false;

	bFlag = SendHangupPacket();
	if( bFlag ) {
		FileLog("RtmpClient",
				"RtmpClient::Hangup( "
				"[Success], "
				"user : '%s, "
				"dest : '%s' "
				")",
				mUser.c_str(),
				mDest.c_str()
				);
	} else {
		FileLog("RtmpClient",
				"RtmpClient::Hangup( "
				"[Fail], "
				"user : '%s, "
				"dest : '%s' "
				")",
				mUser.c_str(),
				mDest.c_str()
				);
	}
	return bFlag;
}

bool RtmpClient::CreatePublishStream() {
	FileLog("RtmpClient",
			"RtmpClient::CreatePublishStream( "
			"user : '%s, "
			"dest : '%s' "
			")",
			mUser.c_str(),
			mDest.c_str()
			);

	bool bFlag = false;

	bFlag = SendCreateStream(STAND_INVOKE_TYPE_PUBLISH);

	if( bFlag ) {
		FileLog("RtmpClient",
				"RtmpClient::CreatePublishStream( "
				"[Success], "
				"user : '%s, "
				"dest : '%s' "
				")",
				mUser.c_str(),
				mDest.c_str()
				);
	} else {
		FileLog("RtmpClient",
				"RtmpClient::CreatePublishStream( "
				"[Fail], "
				"user : '%s, "
				"dest : '%s' "
				")",
				mUser.c_str(),
				mDest.c_str()
				);
	}

	return bFlag;
}

bool RtmpClient::CreateReceiveStream() {
	FileLog("RtmpClient",
			"RtmpClient::CreateReceiveStream( "
			"user : '%s, "
			"dest : '%s' "
			")",
			mUser.c_str(),
			mDest.c_str()
			);

	bool bFlag = false;

	bFlag = SendCreateStream(STAND_INVOKE_TYPE_PLAY);

	if( bFlag ) {
		FileLog("RtmpClient",
				"RtmpClient::CreateReceiveStream( "
				"[Success], "
				"user : '%s, "
				"dest : '%s' "
				")",
				mUser.c_str(),
				mDest.c_str()
				);

	} else {
		FileLog("RtmpClient",
				"RtmpClient::CreateReceiveStream( "
				"[Fail], "
				"user : '%s, "
				"dest : '%s' "
				")",
				mUser.c_str(),
				mDest.c_str()
				);
	}

	return bFlag;
}

bool RtmpClient::SendVideoData(const char* data, unsigned int len, unsigned int timestamp) {
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
		FileLog("RtmpClient",
				"Jni::RtmpClient::SendVideoData( "
				"[Fail], "
				"user : '%s', "
				"dest : '%s' "
				")",
				mUser.c_str(),
				mDest.c_str()
				);
		return false;
	}
	FileLog("RtmpClient",
			"Jni::RtmpClient::SendVideoData( "
			"[success], "
			"user : '%s', "
			"dest : '%s', "
			"len:%d"
			")",
			mUser.c_str(),
			mDest.c_str(),
			len
			);

	return true;
}

bool RtmpClient::SendHeartBeat() {
	bool bFlag = false;

	FileLog("RtmpClient",
			"RtmpClient::SendHeartBeat("
			")"
			);

	bFlag = SendActivePacket();
	if( bFlag ) {
		FileLog("RtmpClient",
				"RtmpClient::SendHeartBeat( "
				"[Success] "
				")"
				);
	} else {
		FileLog("RtmpClient",
				"RtmpClient::SendHeartBeat( "
				"[Fail] "
				")"
				);
	}
	return bFlag;
}

void RtmpClient::Init() {
	mUser = "";

	mSession = "";
	port = 1935;
	hostName = "";
    app = "phone";

    mbRunning = false;

    miNumberInvokes = 0;

	mPublishStreamId = 0;
	mReceiveStreamId = 0;

	mSendTimestamp = 0;
    mSendChunkSize = RTMP_DEFAULT_CHUNKSIZE;
	mRecvChunkSize = RTMP_DEFAULT_CHUNKSIZE;

	mRecv = 0;
	mRecvAckSent = 0;
	mRecvAckWindow = RTMP_DEFAULT_ACK_WINDOW;

	mReadVideoHelper = NULL;

	mPacketVideoHelper = NULL;

	for(int i = 0; i < RTMP_CHANNELS; i++) {
		mChannelsIn[i] = NULL;
		mChannelsOut[i] = NULL;
	}
}

unsigned int RtmpClient::GetTime() {
	struct timeval tv;
	gettimeofday(&tv, 0);
	unsigned int t = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
	return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

bool RtmpClient::HandShake() {
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

	bool bFlag = true;

	mSendMutex.lock();
	ISocketHandler::HANDLE_RESULT result = ISocketHandler::HANDLE_FAIL;
	result = m_socketHandler->Send(clientbuf, RTMP_SIG_SIZE + 1);
	if (result == ISocketHandler::HANDLE_FAIL) {
		FileLog("RtmpClient",
				"RtmpClient::HandShake( "
				"[First step send fail] "
				")"
				);
		bFlag = false;
	}

	if( bFlag ) {
		result = m_socketHandler->Recv((void *)(&type), 1, iLen);
		if( result == ISocketHandler::HANDLE_FAIL || (1 != iLen) ) {
			FileLog("RtmpClient",
					"RtmpClient::HandShake( "
					"[First step receive type fail], "
					"iLen : %d ",
					")",
					iLen
					);
			bFlag = false;
		}
	}

	if( bFlag ) {
		result = m_socketHandler->Recv((void *)serversig, RTMP_SIG_SIZE, iLen);
		if( result == ISocketHandler::HANDLE_FAIL || (RTMP_SIG_SIZE != iLen) ) {
			FileLog("RtmpClient",
					"RtmpClient::HandShake( "
					"[First step receive body fail], "
					"iLen : %d "
					")",
					iLen
					);
			bFlag = false;
		}
	}

	if( bFlag ) {
		/* decode server response */
		memcpy(&suptime, serversig, 4);
		suptime = ntohl(suptime);

		/* 2nd part of handshake */
		result = m_socketHandler->Send((void *)serversig, RTMP_SIG_SIZE);
		if( result == ISocketHandler::HANDLE_FAIL ) {
			FileLog("RtmpClient",
					"RtmpClient::HandShake( "
					"[Second step send fail] "
					")"
					);
			bFlag = false;
		}
	}

	if( bFlag ) {
		result = m_socketHandler->Recv((void *)serversig, RTMP_SIG_SIZE, iLen);
		if( result == ISocketHandler::HANDLE_FAIL || (RTMP_SIG_SIZE != iLen) ) {
			FileLog("RtmpClient",
					"RtmpClient::HandShake( "
					"[Second step receive fail], "
					"iLen : %d "
					")",
					iLen
					);
			bFlag = false;
		}
	}

	if( bFlag ) {
		bMatch = (memcmp(serversig, clientsig, RTMP_SIG_SIZE) == 0);
		if (!bMatch) {
			FileLog("RtmpClient",
					"RtmpClient::HandShake( "
					"[Handshake check fail] "
					")"
					);
			bFlag = false;
		}
	}
	mSendMutex.unlock();

	return bFlag;
}

bool RtmpClient::SendConnectPacket() {
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
	AVal avapp = {(char *)app.c_str(), (int)app.length()};
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
		FileLog("RtmpClient",
				"RtmpClient::SendConnectPacket( "
				"[Fail] "
				")"
				);
		miNumberInvokes--;
		return false;
	}

//	mpStandInvokeMap.Lock();
	mpStandInvokeMap.Insert(miNumberInvokes, STAND_INVOKE_TYPE_CONNECT);
//	mpStandInvokeMap.Unlock();

	return true;
}

bool RtmpClient::SendCreateStream(STAND_INVOKE_TYPE type) {
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
		FileLog("RtmpClient",
				"RtmpClient::SendCreateStream( "
				"[Fail], "
				"user : '%s, "
				"dest : '%s' "
				")",
				mUser.c_str(),
				mDest.c_str()
				);

		miNumberInvokes--;
		return false;
	}

//	mpStandInvokeMap.Lock();
	mpStandInvokeMap.Insert(miNumberInvokes, type);
//	mpStandInvokeMap.Unlock();

	return true;
}

bool RtmpClient::SendPublishPacket(unsigned int streamId) {
	mPublishStreamId = streamId;

	RtmpPacket packet;
	packet.baseHeader.SetChunkType(RTMP_HEADER_CHUNK_TYPE_LARGE);
	packet.baseHeader.SetChunkId(RTMP_CHANNEL_MESSAGE);
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
		FileLog("RtmpClient",
				"RtmpClient::SendPublishPacket( "
				"[Fail], "
				"user : '%s, "
				"dest : '%s' "
				")",
				mUser.c_str(),
				mDest.c_str()
				);
		return false;
	}

	return true;
}

bool RtmpClient::SendReceiveVideoPacket(unsigned int streamId) {
	mReceiveStreamId = streamId;

	RtmpPacket packet;
	packet.baseHeader.SetChunkType(RTMP_HEADER_CHUNK_TYPE_LARGE);
	packet.baseHeader.SetChunkId(0x3);
	packet.messageHeader.SetTimestamp(0);

	char buffer[4096], *pend = buffer + sizeof(buffer);
	char *enc;

	packet.body = (char *)buffer;
	enc = packet.body;

	AVal avpublish = {"receiveVideo", strlen("receiveVideo")};
	enc = AMF_EncodeString(enc, pend, &avpublish);
	enc = AMF_EncodeNumber(enc, pend, 0);
	*enc++ = AMF_NULL;
	enc = AMF_EncodeBoolean(enc, pend, true);

	size_t size = enc - packet.body;
	packet.messageHeader.SetBodySize(size);
	packet.messageHeader.SetType(RTMP_HEADER_MESSAGE_TYPE_INVOKE);
	packet.messageHeader.SetStreamId(mReceiveStreamId);

	// 发送包
	if( !SendRtmpPacket(&packet) ) {
		FileLog("RtmpClient",
				"RtmpClient::SendReceiveVideoPacket( "
				"[Fail], "
				"user : '%s, "
				"dest : '%s' "
				")",
				mUser.c_str(),
				mDest.c_str()
				);
		return false;
	}

	return true;
}

bool RtmpClient::SendPlayPacket() {
	RtmpPacket packet;
	packet.baseHeader.SetChunkType(RTMP_HEADER_CHUNK_TYPE_LARGE);
	packet.baseHeader.SetChunkId(0x3);
	packet.messageHeader.SetTimestamp(0);

	char buffer[4096], *pend = buffer + sizeof(buffer);
	char *enc;

	packet.body = (char *)buffer;
	enc = packet.body;

	AVal avpublish = {"play", strlen("play")};
	enc = AMF_EncodeString(enc, pend, &avpublish);
	enc = AMF_EncodeNumber(enc, pend, 0);
	*enc++ = AMF_NULL;
	AVal avpublishValue = {"play", strlen("play")};
	enc = AMF_EncodeString(enc, pend, &avpublishValue);

	size_t size = enc - packet.body;
	packet.messageHeader.SetBodySize(size);
	packet.messageHeader.SetType(RTMP_HEADER_MESSAGE_TYPE_INVOKE);
	packet.messageHeader.SetStreamId(mReceiveStreamId);

	// 发送包
	if( !SendRtmpPacket(&packet) ) {
		FileLog("RtmpClient",
				"RtmpClient::SendPlayPacket( "
				"[Fail], "
				"user : '%s, "
				"dest : '%s' "
				")",
				mUser.c_str(),
				mDest.c_str()
				);
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

	AVal avuser = {(char *)user.c_str(), (int)user.length()};
	enc = AMF_EncodeString(enc, pend, &avuser);

	AVal avauth = {(char *)auth.c_str(), (int)auth.length()};
	enc = AMF_EncodeString(enc, pend, &avauth);

	AVal avsite = {(char *)site.c_str(), (int)site.length()};
	enc = AMF_EncodeString(enc, pend, &avsite);

	AVal avcustom = {(char *)custom.c_str(), (int)custom.length()};
	enc = AMF_EncodeString(enc, pend, &avcustom);

	size_t size = enc - packet.body;
	packet.messageHeader.SetBodySize(size);
	packet.messageHeader.SetType(RTMP_HEADER_MESSAGE_TYPE_INVOKE);
	packet.messageHeader.SetStreamId(0);

	// 发送包
	if( !SendRtmpPacket(&packet) ) {
		FileLog("RtmpClient",
				"RtmpClient::SendLoginPacket( "
				"[Fail], "
				"user : '%s, "
				")",
				mUser.c_str()
				);
		return false;
	}

	return true;
}

bool RtmpClient::SendMakeCallPacket(const string& dest) {
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

	AVal avdest = {(char *)dest.c_str(), (int)dest.length()};
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
		FileLog("RtmpClient",
				"RtmpClient::SendMakeCallPacket( "
				"[Fail], "
				"user : '%s, "
				"dest : '%s' "
				")",
				mUser.c_str(),
				mDest.c_str()
				);
		return false;
	}

	return true;
}

bool RtmpClient::SendHangupPacket() {
	RtmpPacket packet;
	packet.baseHeader.SetChunkType(RTMP_HEADER_CHUNK_TYPE_LARGE);
	packet.baseHeader.SetChunkId(RTMP_CHANNEL_MESSAGE);
	packet.messageHeader.SetTimestamp(0);

	char buffer[4096], *pend = buffer + sizeof(buffer);
	char *enc;

	packet.body = (char *)buffer;
	enc = packet.body;

	AVal avhangup = {"hangup", strlen("hangup")};
	enc = AMF_EncodeString(enc, pend, &avhangup);
	enc = AMF_EncodeNumber(enc, pend, 0);
	*enc++ = AMF_NULL;

	AVal avuuid = {(char *)"", 0};
	enc = AMF_EncodeString(enc, pend, &avuuid);

	AVal avcause = {(char *)"", 0};
	enc = AMF_EncodeString(enc, pend, &avcause);

	size_t size = enc - packet.body;
	packet.messageHeader.SetBodySize(size);
	packet.messageHeader.SetType(RTMP_HEADER_MESSAGE_TYPE_INVOKE);
	packet.messageHeader.SetStreamId(0);

	// Send Packet
	if( !SendRtmpPacket(&packet) ) {
		FileLog("RtmpClient",
				"RtmpClient::SendHangupPacket( "
				"[Fail], "
				"user : '%s, "
				"dest : '%s' "
				")",
				mUser.c_str(),
				mDest.c_str()
				);
		return false;
	}

	return true;
}

bool RtmpClient::SendActivePacket() {
	RtmpPacket packet;
	packet.baseHeader.SetChunkType(RTMP_HEADER_CHUNK_TYPE_LARGE);
	packet.baseHeader.SetChunkId(RTMP_CHANNEL_MESSAGE);
	packet.messageHeader.SetTimestamp(0);

	char buffer[4096], *pend = buffer + sizeof(buffer);
	char *enc;

	packet.body = (char *)buffer;
	enc = packet.body;

	AVal avSetActive = {"setActive", strlen("setActive")};
	enc = AMF_EncodeString(enc, pend, &avSetActive);
	enc = AMF_EncodeNumber(enc, pend, 0);
	*enc++ = AMF_NULL;

	size_t size = enc - packet.body;
	packet.messageHeader.SetBodySize(size);
	packet.messageHeader.SetType(RTMP_HEADER_MESSAGE_TYPE_INVOKE);
	packet.messageHeader.SetStreamId(0);

	// Send Packet
	if( !SendRtmpPacket(&packet) ) {
		FileLog("RtmpClient",
				"RtmpClient::SendActivePacket( "
				"[Fail] "
				")"
				);
		return false;
	}

	return true;
}

bool RtmpClient::SendAckPacket() {
//	FileLog("RtmpClient",
//			"RtmpClient::SendAckPacket( "
//			"mRecv : %d, "
//			"mRecvAckSent : %d, "
//			"mRecvAckWindow : %d "
//			")",
//			mRecv,
//			mRecvAckSent,
//			mRecvAckWindow
//			);

	if (mRecv - mRecvAckSent >= mRecvAckWindow / 4) {
		RtmpPacket packet;
		packet.baseHeader.SetChunkType(RTMP_HEADER_CHUNK_TYPE_LARGE);
		packet.baseHeader.SetChunkId(RTMP_CHANNEL_CONTROL);
		packet.messageHeader.SetTimestamp(0);

		unsigned int ack = htonl(mRecv);
		packet.body = (char *)&ack;

		size_t size = sizeof(unsigned int);
		packet.messageHeader.SetBodySize(size);
		packet.messageHeader.SetType(RTMP_HEADER_MESSAGE_TYPE_ACK);
		packet.messageHeader.SetStreamId(0);

		// Send Packet
		if( !SendRtmpPacket(&packet) ) {
			FileLog("RtmpClient",
					"RtmpClient::SendAckPacket( "
					"[Fail] "
					")"
					);
			return false;
		}

		mRecvAckSent = mRecv;
	}

	return true;
}

bool RtmpClient::SendRtmpPacket(RtmpPacket* packet) {
	FileLog("RtmpClient",
			"Jni::RtmpClient::SendRtmpPacket( "
			"####################### Send ####################### "
			")"
			);
	// Dump Raw Packet
	DumpRtmpPacket(packet);

	bool bFlag = true;

//	if( !mbConnected || mbShutdown ) {
//		FileLog("RtmpClient",
//				"RtmpClient::SendRtmpPacket( "
//				"[Not connected] "
//				")"
//				);
//		return false;
//	}

	unsigned int timestamp = 0;
//	// Change to Absolute Timestamp
//	unsigned int timestamp = packet->messageHeader.GetTimestamp() + (GetTime() - mTimestamp);
//	packet->messageHeader.SetTimestamp(timestamp);

	unsigned int chunkId = -1;
	unsigned int streamId = -1;
	unsigned int lastTimestamp = 0;

	mSendMutex.lock();

	RtmpPacket* pLastSendPacket = mChannelsOut[packet->baseHeader.GetChunkId()];
	if( pLastSendPacket != NULL ) {
		chunkId = pLastSendPacket->baseHeader.GetChunkId();
		streamId = pLastSendPacket->messageHeader.GetStreamId();
		lastTimestamp = pLastSendPacket->messageHeader.GetTimestamp();

	} else {
		// New Last Send Packet
		pLastSendPacket = new RtmpPacket();
		mChannelsOut[packet->baseHeader.GetChunkId()] = pLastSendPacket;
	}


	// Record Last Send Packet
	*pLastSendPacket = *packet;

	// Same Chunk && Same Stream
	if( chunkId == packet->baseHeader.GetChunkId() && streamId == packet->messageHeader.GetStreamId() ) {
		// Modify ChunkType to Medium
		packet->baseHeader.SetChunkType(RTMP_HEADER_CHUNK_TYPE_MEDIUM);

		// Calculate Timestamp delta
		timestamp = packet->messageHeader.GetTimestamp() - lastTimestamp;

			FileLog("RtmpClient",
					"RtmpClient::SendRtmpPacket( "
					"timestamp ：%d"
					")",
					timestamp
					);
		// Modify to delta Timestamp
		packet->messageHeader.SetTimestamp(timestamp);
	}


//	FileLog("RtmpClient",
//			"RtmpClient::SendRtmpPacket( "
//			"[Dump Send Packet] "
//			")"
//			);
	// Dump Send Packet
	DumpRtmpPacket(packet);

	// Send Header
	ISocketHandler::HANDLE_RESULT result = ISocketHandler::HANDLE_FAIL;
	result = m_socketHandler->Send((void *)packet, packet->GetHeaderLength());
	if (result == ISocketHandler::HANDLE_FAIL) {
		FileLog("RtmpClient",
				"Jni::RtmpClient::SendRtmpPacket( "
				"[Send Header Fail] "
				")"
				);
		Shutdown();
//		if( mpRtmpClientListener != NULL ) {
//			mpRtmpClientListener->OnDisconnect(this);
//		}
		bFlag = false;
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

	if( bFlag ) {
		// Send Body
		unsigned int last = packet->messageHeader.GetBodySize();
		unsigned int send = (mSendChunkSize < last)?mSendChunkSize:last;
		unsigned int index = 0;

		while( last > 0 ) {
			// Send Raw Body
			if( packet->body != NULL && index < packet->messageHeader.GetBodySize() ) {
				result = m_socketHandler->Send((void *)(packet->body + index), send);
				if (result == ISocketHandler::HANDLE_FAIL) {
	//				printf("# RtmpClient::SendRtmpPacket( Send Body Fail ) \n");
					FileLog("RtmpClient",
							"Jni::RtmpClient::SendRtmpPacket( "
							"[Send Body Fail] "
							")"
							);
					Shutdown();
//					if( mpRtmpClientListener != NULL ) {
//						mpRtmpClientListener->OnDisconnect(this);
//					}
					bFlag = false;
					break;
				}
			}

			index += send;
			last -= send;
			send = (mSendChunkSize < last)?mSendChunkSize:last;

			// Send Chunk Separate
			if( last > 0 ) {
				unsigned char c = (0xc0 | packet->baseHeader.buffer);
				result = m_socketHandler->Send((void *)&c, 1);
				if (result == ISocketHandler::HANDLE_FAIL) {
					FileLog("RtmpClient",
							"Jni::RtmpClient::SendRtmpPacket( "
							"[Send Chunk Separate Fail] "
							")"
							);
					Shutdown();
//					if( mpRtmpClientListener != NULL ) {
//						mpRtmpClientListener->OnDisconnect(this);
//					}
					bFlag = false;
					break;
				}


			}
		}
	}

	mSendMutex.unlock();

	return true;
}

bool RtmpClient::RecvRtmpPacket(RtmpPacket* packet) {
	bool bFlag = false;

	FileLog("RtmpClient",
			"RtmpClient::RecvRtmpChunkPacket( "
			"####################### Recv ####################### "
			")"
			);

	while( (bFlag = RecvRtmpChunkPacket(packet)) ) {
		if( IsReadyPacket(packet) ) {
			RtmpPacket* pLastRecvPacket = mChannelsIn[packet->baseHeader.GetChunkId()];
			pLastRecvPacket->FreeBody();
			break;
		}
		packet->Reset();
	}

	return bFlag;
}

bool RtmpClient::RecvRtmpChunkPacket(RtmpPacket* packet) {
	FileLog("RtmpClient",
			"RtmpClient::RecvRtmpChunkPacket( "
			"####################### Recv Chunk ####################### "
			")"
			);

	bool bFlag = true;
	mRecvMutex.lock();

	ISocketHandler::HANDLE_RESULT result = ISocketHandler::HANDLE_FAIL;
	unsigned int len = 0;

	// Read Base Header
	result = m_socketHandler->Recv((void *)&(packet->baseHeader.buffer), 1, len);
	if( ISocketHandler::HANDLE_FAIL == result || (1 != len) ) {
		FileLog("RtmpClient",
				"RtmpClient::RecvRtmpChunkPacket( "
				"[Read Base Header Header fail] "
				")"
				);
		Shutdown();
		if( mpRtmpClientListener != NULL ) {
			mpRtmpClientListener->OnDisconnect(this);
		}
		bFlag = false;
	} else {
		mRecv += 1;
	}

	if( bFlag ) {
		// Read Message Header
		if( packet->GetMessageHeaderLength() > 0 ) {
			result = m_socketHandler->Recv((void *)&(packet->messageHeader.buffer), packet->GetMessageHeaderLength(), len);
			if( ISocketHandler::HANDLE_FAIL == result || (packet->GetMessageHeaderLength() != len) ) {
				FileLog("RtmpClient",
						"RtmpClient::RecvRtmpChunkPacket( "
						"[Read Message Header Header fail] "
						")"
						);
				Shutdown();
				if( mpRtmpClientListener != NULL ) {
					mpRtmpClientListener->OnDisconnect(this);
				}
				bFlag = false;
			} else {
				mRecv += packet->GetMessageHeaderLength();
			}
		}
	}

	if( bFlag ) {
		DumpRtmpPacket(packet);

		RtmpPacket* pLastRecvPacket = mChannelsIn[packet->baseHeader.GetChunkId()];
		if( !pLastRecvPacket ) {
			pLastRecvPacket = new RtmpPacket();
			mChannelsIn[packet->baseHeader.GetChunkId()] = pLastRecvPacket;
		} else {
			packet->messageHeader.SetStreamId(pLastRecvPacket->messageHeader.GetStreamId());
			packet->messageHeader.SetType(pLastRecvPacket->messageHeader.GetType());
			packet->nBytesRead = pLastRecvPacket->nBytesRead;
		}

		// Calculate Timestamp
		switch(packet->baseHeader.GetChunkType()) {
		case RTMP_HEADER_CHUNK_TYPE_LARGE:{
			packet->FreeBody();
		}break;
		case RTMP_HEADER_CHUNK_TYPE_MEDIUM:{
			packet->FreeBody();

			if( pLastRecvPacket != NULL ) {
				unsigned int delta = packet->messageHeader.GetTimestamp();
				unsigned int timestamp = delta + pLastRecvPacket->messageHeader.GetTimestamp();
				packet->messageHeader.SetTimestamp(timestamp);
			}
		}break;
		case RTMP_HEADER_CHUNK_TYPE_SMALL:{
			if( pLastRecvPacket != NULL ) {
				unsigned int delta = packet->messageHeader.GetTimestamp();
				unsigned int timestamp = delta + pLastRecvPacket->messageHeader.GetTimestamp();
				packet->messageHeader.SetTimestamp(timestamp);
				packet->messageHeader.SetBodySize(pLastRecvPacket->messageHeader.GetBodySize());
			}
		}break;
		case RTMP_HEADER_CHUNK_TYPE_MINIMUM:{
			if( pLastRecvPacket != NULL ) {
				packet->messageHeader.SetTimestamp(pLastRecvPacket->messageHeader.GetTimestamp());
				packet->messageHeader.SetBodySize(pLastRecvPacket->messageHeader.GetBodySize());
			}
		}break;
		default:break;
		}

//		FileLog("RtmpClient",
//				"RtmpClient::RecvRtmpChunkPacket( "
//				"####################### Recv Chunk Combine ####################### "
//				")"
//				);
		DumpRtmpPacket(packet);

		// Create buffer if need
		packet->AllocBody();

		// Read Body
		if( packet->messageHeader.GetBodySize() > 0 ) {
			unsigned int nToRead = packet->messageHeader.GetBodySize() - packet->nBytesRead;
			nToRead = min(nToRead, mRecvChunkSize);

			result = m_socketHandler->Recv((void *)(packet->body + packet->nBytesRead), nToRead, len);
			if( ISocketHandler::HANDLE_FAIL == result || (nToRead != len) ) {
				FileLog("RtmpClient",
						"RtmpClient::RecvRtmpChunkPacket( "
						"[Read Body fail], "
						"len : %d, "
						"nToRead : %d "
						")",
						len,
						nToRead
						);
				Shutdown();
				if( mpRtmpClientListener != NULL ) {
					mpRtmpClientListener->OnDisconnect(this);
				}
				bFlag = false;
			} else {
				mRecv += nToRead;

				packet->nBytesRead += nToRead;

				FileLog("RtmpClient",
						"RtmpClient::RecvRtmpChunkPacket( "
						"[Read Chunk Body], "
						"bodySize : %d, "
						"nBytesRead : %d, "
						"nToRead : %d "
						")",
						packet->messageHeader.GetBodySize(),
						packet->nBytesRead,
						nToRead
						);
//				Arithmetic am;
//				string log = am.AsciiToHexWithSep((unsigned char *)packet->body + packet->nBytesRead, nToRead);
//				FileLog("RtmpClient",
//						"RtmpClient::RecvRtmpChunkPacket( "
//						"[Read Chunk Body], "
//						"%s "
//						")",
//						log.c_str()
//						);
			}
		}

		// Update packet
		*pLastRecvPacket = *packet;
	}

	mRecvMutex.unlock();

	/* Send an ACK if we need to */
	SendAckPacket();

	return bFlag;
}

RTMP_PACKET_TYPE RtmpClient::ParseRtmpPacket(RtmpPacket* packet) {
	FileLog("RtmpClient",
			"RtmpClient::ParseRtmpPacket( "
			"####################### Parse ####################### "
			")"
			);
	DumpRtmpPacket(packet);

	RTMP_PACKET_TYPE type = RTMP_PACKET_TYPE_UNKNOW;
	switch(packet->messageHeader.GetType()) {
	case RTMP_HEADER_MESSAGE_TYPE_INVOKE:{
		AMFObject obj;
		if( AMF_Decode(&obj, packet->body, packet->messageHeader.GetBodySize(), FALSE) != -1 ) {
			AVal method;
			AMFProp_GetString(AMF_GetProp(&obj, NULL, 0), &method);

			char temp[2048];
			if( memcmp(method.av_val, "_result", strlen("_result")) == 0 ) {
				// Parse _result
				int seq = (int)AMFProp_GetNumber(AMF_GetProp(&obj, NULL, 1));

				STAND_INVOKE_TYPE invoke_type = STAND_INVOKE_TYPE_UNKNOW;
				mpStandInvokeMap.Lock();
				StandInvokeMap::iterator itr = mpStandInvokeMap.Find(seq);
				if( itr != mpStandInvokeMap.End() ) {
					invoke_type = itr->second;
					mpStandInvokeMap.Erase(itr);
				}
				mpStandInvokeMap.Unlock();

				FileLog("RtmpClient",
						"RtmpClient::ParseRtmpPacket( "
						"[_result], "
						"seq : %d, "
						"invoke_type : %d "
						")",
						seq,
						invoke_type
						);

				switch(invoke_type) {
				case STAND_INVOKE_TYPE_CONNECT:{

				}break;
				case STAND_INVOKE_TYPE_PUBLISH:{
					int streamId = (int)AMFProp_GetNumber(AMF_GetProp(&obj, NULL, 3));
	//				mSendTimestamp = GetTime();
					SendPublishPacket(streamId);

					if( mpRtmpClientListener != NULL ) {
						mpRtmpClientListener->OnCreatePublishStream(this);
					}

				}break;
				case STAND_INVOKE_TYPE_PLAY:{
					int streamId = (int)AMFProp_GetNumber(AMF_GetProp(&obj, NULL, 3));
					SendReceiveVideoPacket(streamId);
	//				mSendTimestamp = GetTime();
					SendPlayPacket();

				}break;
				default:break;
				}

			} else if( memcmp(method.av_val, "connected", strlen("connected")) == 0 ) {
				// Parse connected
				type = RTMP_PACKET_TYPE_CONNECTED;

				AVal session;
				AMFProp_GetString(AMF_GetProp(&obj, NULL, 3), &session);

				memcpy(temp, session.av_val, session.av_len);
				temp[session.av_len] = '\0';
				mSession = temp;

				FileLog("RtmpClient",
						"RtmpClient::ParseRtmpPacket( "
						"[connected], "
						"sessionId : %s "
						")",
						mSession.c_str()
						);

				if( mpRtmpClientListener != NULL ) {
					mpRtmpClientListener->OnConnect(this, mSession);
				}

			} else if( memcmp(method.av_val, "onLogin", strlen("onLogin")) == 0 ) {
				// Parse onLogin
				AVal loginResult;
				AMFProp_GetString(AMF_GetProp(&obj, NULL, 3), &loginResult);

				bool bFlag = false;
				if( memcmp(loginResult.av_val, "success", strlen("success")) == 0 ) {
					bFlag = true;
				}

				FileLog("RtmpClient",
						"RtmpClient::ParseRtmpPacket( "
						"[onLogin], "
						"bFlag : %s "
						")",
						bFlag?"true":"false"
						);

				if( mpRtmpClientListener != NULL ) {
					mpRtmpClientListener->OnLogin(this, bFlag);


//					static bool  ispublish = true;
//					if(ispublish)
//					{
//						unsigned int stream = 3;
//						SendPublishPacket(stream);
//						ispublish = false;
//					}
				}
			} else if( memcmp(method.av_val, "onMakeCall", strlen("onMakeCall")) == 0 ) {
				// Parse onMakeCall
				AVal av_channelId;
				AMFProp_GetString(AMF_GetProp(&obj, NULL, 3), &av_channelId);

				memcpy(temp, av_channelId.av_val, av_channelId.av_len);
				temp[av_channelId.av_len] = '\0';
				string channelId = temp;

				FileLog("RtmpClient",
						"RtmpClient::ParseRtmpPacket( "
						"[onMakeCall], "
						"channelId : '%s' "
						")",
						channelId.c_str()
						);

				if( mpRtmpClientListener != NULL ) {
					mpRtmpClientListener->OnMakeCall(this, true, channelId);
				}

			} else if( memcmp(method.av_val, "onHangup", strlen("onHangup")) == 0 ) {
				// Parse onHangup
				AVal av_channelId;
				AMFProp_GetString(AMF_GetProp(&obj, NULL, 3), &av_channelId);

				memcpy(temp, av_channelId.av_val, av_channelId.av_len);
				temp[av_channelId.av_len] = '\0';
				string channelId = temp;

				AVal av_cause;
				AMFProp_GetString(AMF_GetProp(&obj, NULL, 4), &av_cause);

				memcpy(temp, av_cause.av_val, av_cause.av_len);
				temp[av_cause.av_len] = '\0';
				string cause = temp;

				FileLog("RtmpClient",
						"RtmpClient::ParseRtmpPacket( "
						"[onHangup], "
						"channelId : %s, "
						"cause : %s "
						")",
						channelId.c_str(),
						cause.c_str()
						);

				if( mpRtmpClientListener != NULL ) {
					mpRtmpClientListener->OnHangup(this, channelId, cause);
				}

			} else if( memcmp(method.av_val, "onActive", strlen("onActive")) == 0 ) {
				// Parse onActive
				FileLog("RtmpClient",
						"RtmpClient::ParseRtmpPacket( "
						"[onActive] "
						")"
						);

				if( mpRtmpClientListener != NULL ) {
					mpRtmpClientListener->OnHeartBeat(this);
				}
			}
		}
	}break;
	case RTMP_HEADER_MESSAGE_TYPE_AUDIO:{
		// Parse Audio
		FileLog("RtmpClient",
				"RtmpClient::ParseRtmpPacket( "
				"[Audio] "
				")"
				);
		if( mpRtmpClientListener != NULL ) {
			mpRtmpClientListener->OnRecvAudio(this, packet->body, packet->messageHeader.GetBodySize());
		}
	}break;
	case RTMP_HEADER_MESSAGE_TYPE_VIDEO:{
		// Parse Video
		FileLog("RtmpClient",
				"RtmpClient::ParseRtmpPacket( "
				"[Video] "
				")"
				);

		bool bFlag = Rtmp2H264((unsigned char*)packet->body, packet->messageHeader.GetBodySize());
		while( bFlag ) {
			if( mReadVideoHelper && !mReadVideoHelper->nal_list.empty() ) {
				RtmpBuffer* buffer = mReadVideoHelper->nal_list.front();
				mReadVideoHelper->nal_list.pop_front();
				if( buffer ) {
					if (buffer->len > 1500) {
						FileLog("RtmpClient",
								"RtmpClient::ParseRtmpPacket( "
								"[Video], "
								"data size too large : %d "
								")",
								buffer->len
								);
					} else {
						if( mpRtmpClientListener != NULL ) {
							mpRtmpClientListener->OnRecvVideo(this, (const char*)buffer->data, buffer->len, packet->messageHeader.GetTimestamp());
						}
					}
					delete buffer;
				}
			} else {
				break;
			}
		}
	}break;
	case RTMP_HEADER_MESSAGE_TYPE_SET_CHUNK:{
		// Parse Set Chunk Size
		unsigned int size = mRecvChunkSize;
		if( packet->messageHeader.GetBodySize() == sizeof(unsigned int) ) {
			unsigned int size = ntohl(*(unsigned int *)(packet->body));
			if( size > RTMP_DEFAULT_CHUNKSIZE ) {
				mRecvChunkSize = size;
				FileLog("RtmpClient",
						"RtmpClient::ParseRtmpPacket( "
						"[Set Chunk Size], "
						"size : %u "
						")",
						size
						);
			} else {
				FileLog("RtmpClient",
						"RtmpClient::ParseRtmpPacket( "
						"[Set Chunk Size], "
						"error size : %u "
						")",
						size
						);
			}
		}
	}break;
	case RTMP_HEADER_MESSAGE_TYPE_WINDOW_ACK_SIZE:{
		// Parse Acknowledgment Window Size
		unsigned int size = mRecvAckWindow;
		if( packet->messageHeader.GetBodySize() == sizeof(unsigned int) ) {
			unsigned int size = ntohl(*(unsigned int *)(packet->body));
			if( size > 0 ) {
				mRecvAckWindow = size;
				FileLog("RtmpClient",
						"RtmpClient::ParseRtmpPacket( "
						"[Set Acknowledgment Size], "
						"size : %u "
						")",
						size
						);
			} else {
				FileLog("RtmpClient",
						"RtmpClient::ParseRtmpPacket( "
						"[Set Acknowledgment Size], "
						"error size : %u "
						")",
						size
						);
			}
		}
	}break;
	default:{
		FileLog("RtmpClient",
				"RtmpClient::ParseRtmpPacket( "
				"[Unknow] "
				")"
				);
		break;
	}
	}

	return type;
}

void RtmpClient::DumpRtmpPacket(RtmpPacket* packet) {
	FileLog("RtmpClient",
			"RtmpClient::DumpRtmpPacket( "
			"Format : %u, "
			"Chunk Stream ID : %u, "
			"Timestamp : %u, "
			"Body Size : %u, "
			"Type ID : 0x%02x, "
			"Stream ID : %u, "
			"nBytesRead : %u "
			")",
			packet->baseHeader.GetChunkType(),
			packet->baseHeader.GetChunkId(),
			packet->messageHeader.GetTimestamp(),
			packet->messageHeader.GetBodySize(),
			packet->messageHeader.GetType(),
			packet->messageHeader.GetStreamId(),
			packet->nBytesRead
			);
}

bool RtmpClient::IsReadyPacket(RtmpPacket* packet) {
	bool bFlag = (packet->nBytesRead == packet->messageHeader.GetBodySize());
	return bFlag;
}

/*Rtmp packet to H264 frame*/
bool RtmpClient::Rtmp2H264(unsigned char* data, unsigned int len) {
	bool bFlag = true;

	FileLog("RtmpClient",
			"RtmpClient::Rtmp2H264( "
			"len : %d "
			")",
			len
			);
	if( len <= 1 ) {
		return false;
	}

	FileLog("RtmpClient",
			"RtmpClient::Rtmp2H264( "
			"data[0] : %x, "
			"data[1] : %x "
			")",
			data[0],
			data[1]
			);
	unsigned char *end = data + len;//

	if (data[0] == 0x17 && data[1] == 0) {
		unsigned char *pdata = data + 2;
		int cfgVer = pdata[3];
		if (cfgVer == 1) {
			int i = 0;
			int numSPS = 0;
			int numPPS = 0;
			int lenSize = (pdata[7] & 0x03) + 1;// 4
			int lenSPS;
			int lenPPS;
			//sps (sequence parameter Sets, 序列参数集)
			numSPS = pdata[8] & 0x1f;//1
			pdata += 9;
			for (i = 0; i < numSPS; i++) {
				lenSPS = ntohs(*(unsigned short *)pdata);
				pdata += 2;

				if (lenSPS > end - pdata) {
					return false;
				}

				if (mReadVideoHelper->sps == NULL) {
					RtmpBuffer* buffer = new RtmpBuffer(pdata, lenSPS);
					string str = Arithmetic::AsciiToHexWithSep(buffer->data, buffer->len);
					FileLog("RtmpClient",
							"RtmpClient::Rtmp2H264( "
							"new sps : %s "
							")",
							str.c_str()
							);
					mReadVideoHelper->sps = buffer;
				}
				pdata += lenSPS;
			}
			//pps （Picture Parameter Sets 图片参数集）
			numPPS = pdata[0];
			pdata += 1;
			for (i = 0; i < numPPS; i++) {
				lenPPS = ntohs(*(unsigned short *)pdata);
				pdata += 2;
				if (lenPPS > end - pdata) {
					return false;
				}

				if (mReadVideoHelper->pps == NULL) {
					RtmpBuffer* buffer = new RtmpBuffer(pdata, lenSPS);
					string str = Arithmetic::AsciiToHexWithSep(buffer->data, buffer->len);
					FileLog("RtmpClient",
							"RtmpClient::Rtmp2H264( "
							"new pps : %s "
							")",
							str.c_str()
							);
					mReadVideoHelper->pps = buffer;
				}
				pdata += lenPPS;
			}

			mReadVideoHelper->lenSize = lenSize;

			// add sps to list
			if (mReadVideoHelper->sps != NULL) {
				mReadVideoHelper->nal_list.push_back(new RtmpBuffer(mReadVideoHelper->sps));

			}
			// add pps to list
			if (mReadVideoHelper->pps != NULL) {
				mReadVideoHelper->nal_list.push_back(new RtmpBuffer(mReadVideoHelper->pps));
			}

		} else {
			FileLog("RtmpClient",
					"RtmpClient::Rtmp2H264( "
					"Unsuported cfgVer : %d "
					")",
					cfgVer
					);
		}
	} else if ((data[0] == 0x17 || data[0] == 0x27) && data[1] == 1) {
		if (mReadVideoHelper->sps && mReadVideoHelper->pps) {
			unsigned char * pdata = data + 5;
			unsigned int pdata_len = len - 5;
			unsigned int lenSize = mReadVideoHelper->lenSize;
			unsigned char *nal_buf = NULL;
			unsigned int nal_len = 0;

			while (pdata_len > 0) {
				unsigned int nalSize = 0;
				switch (lenSize) {
				case 1:
					nalSize = pdata[lenSize - 1] & 0xff;
					break;
				case 2:
					nalSize = ((pdata[lenSize - 2] & 0xff) << 8) | (pdata[lenSize - 1] & 0xff);
					break;
				case 4:
					nalSize = (pdata[lenSize - 4] & 0xff) << 24 |
						(pdata[lenSize - 3] & 0xff) << 16 |
						(pdata[lenSize - 2] & 0xff) << 8  |
						(pdata[lenSize - 1] & 0xff);
					break;
				default:
					FileLog("RtmpClient",
							"RtmpClient::Rtmp2H264( "
							"Invalid length size : %d "
							")",
							lenSize
							);
					return false;
				}

				nal_buf = pdata + lenSize;
				nal_len = nalSize;

				//next nal
				pdata = pdata + lenSize + nalSize;
				pdata_len -= (lenSize + nalSize);
			}

			if ((nal_len > 0 && nal_len < len) && nal_buf != NULL) {
				unsigned char * remaining = nal_buf;
				int remaining_len = nal_len;
				int nalType = remaining[0] & 0x1f;
				int nri = remaining[0] & 0x60;

				FileLog("RtmpClient",
						"RtmpClient::Rtmp2H264( "
						"nalType : %d, "
						"remaining_len : %d "
						")",
						nalType,
						remaining_len
						);

				if (nalType == 5 || nalType == 1) {
					if (remaining_len < MAX_RTP_PAYLOAD_SIZE) {
						RtmpBuffer* buffer = new RtmpBuffer(remaining, remaining_len);
						mReadVideoHelper->nal_list.push_back(buffer);
					} else {
						unsigned char start = (unsigned char) 0x80;
						remaining += 1;
						remaining_len -= 1;

						while (remaining_len > 0) {
							int payload_len = (MAX_RTP_PAYLOAD_SIZE - 2) < remaining_len ? (MAX_RTP_PAYLOAD_SIZE - 2) : remaining_len;

							unsigned char payload[MAX_RTP_PAYLOAD_SIZE];
							unsigned char end;

							memcpy(payload + 2, remaining, payload_len);
							remaining_len -= payload_len;
							remaining += payload_len;

							end = (unsigned char) ((remaining_len > 0) ? 0 : 0x40);
							payload[0] = nri | 28; // FU-A
							payload[1] = start | end | nalType;

							FileLog("RtmpClient",
									"RtmpClient::Rtmp2H264( "
									"payload_len : %d "
									")",
									payload_len
									);

							RtmpBuffer* buffer = new RtmpBuffer(payload, payload_len + 2);
							mReadVideoHelper->nal_list.push_back(buffer);

							start = 0;
						}
					}
				}
			}
		}
	} else {
		FileLog("RtmpClient",
				"RtmpClient::Rtmp2H264( "
				"Missing rtmp data "
				")"
				);
	}

	return bFlag;
}

// h264组包
bool RtmpClient::RtmpToPacketH264(unsigned char* data, unsigned int len, unsigned int timesp)
{
	FileLog("RtmpClient",
			"RtmpClient::RtmpToPacketH264(start"
			" )"
			);
	bool result = true;
	// 数据
	unsigned char *payload = data;
	// 数据长度
	unsigned int  payloadLen = len;
	// 根据数据第一位 & 0x 得到类型

	int nalType = payload[0] & 0x1f;

	uint32_t size = 0;
	// rtp序列号
	uint16_t rtp_seq = 0;
	// rtp 时间
	uint32_t rtp_ts = 0;

	uint32_t timestamp = timesp;
	static const uint8_t rtmp_header17[] = {0x17, 1, 0, 0, 0};
	static const uint8_t rtmp_header27[] = {0x27, 1, 0, 0, 0};

	// 应由函数提供，序列号
	rtp_seq = 1;
	// 应由函数提供，时间
	rtp_ts = 1;
	// 应由函数提供，时间

//	// 不是下一个数据？？ 可能丢失了
//	if (mPacketVideoHelper->last_seq && mPacketVideoHelper->last_seq + 1 != rtp_seq)
//	{
//
//		FileLog("CamshareClient",
//				"Jni::RtmpToPacketH264( "
//				"last_seq : %d,"
//				" )",
//				mPacketVideoHelper->last_seq
//				);
//		// 这一帧不是psp帧时
//		if (nalType != 7){
//			// 如果sps有数据就delete
//			if(mPacketVideoHelper->sps){
//				delete mPacketVideoHelper->sps;
//				mPacketVideoHelper->sps = NULL;
//			}
//			// 等到sps
//			mPacketVideoHelper->last_recv_ts = rtp_ts;
//			mPacketVideoHelper->last_seq    = rtp_seq;
//			goto wait_sps;
//		}
//	}
//
//	if (mPacketVideoHelper->last_recv_ts != timestamp){
//		// 上帧接收的时间 不等于这帧编码后的时间
//		if(mPacketVideoHelper->rtmp_buf){
//			delete mPacketVideoHelper->rtmp_buf;
//			mPacketVideoHelper->rtmp_buf = NULL;
//		}
//		if(mPacketVideoHelper->fua_buf){
//			delete mPacketVideoHelper->fua_buf;
//			mPacketVideoHelper->fua_buf = NULL;
//		}
//	}

	mPacketVideoHelper->last_recv_ts = rtp_ts;
	//mPacketVideoHelper->last_mark
	mPacketVideoHelper->last_seq = rtp_seq;

	// 下面才是重点，上面可以删除？？？

	switch (nalType){
	case 7://sps
	{
		// 对比新旧的不同（和时间还是数据比较等等,）
		if (1){
			if (mPacketVideoHelper->sps != NULL){
				delete mPacketVideoHelper->sps;
				mPacketVideoHelper->sps = NULL;
			}
			RtmpBuffer* buffer = new RtmpBuffer(payload, payloadLen);
			mPacketVideoHelper->sps = buffer;
			mPacketVideoHelper->sps_changed = true;
		}
		else
		{
			mPacketVideoHelper->sps_changed = false;
		}
	}
		break;

	case 8: // pps
	{
		if (mPacketVideoHelper->pps != NULL){
			delete mPacketVideoHelper->pps;
			mPacketVideoHelper->pps = NULL;
		}
		RtmpBuffer* buffer = new RtmpBuffer(payload, payloadLen);
		mPacketVideoHelper->pps = buffer;
	}
	break;

	case 1: // Non IDR
	{
		unsigned char loaddata[MAX_ENCODE_SIZE];
		int loaddataLen = 0;
		if (mPacketVideoHelper->rtmp_buf && mPacketVideoHelper->rtmp_buf->len >0){
			memcpy(loaddata + loaddataLen, mPacketVideoHelper->rtmp_buf->data, mPacketVideoHelper->rtmp_buf->len);
			loaddataLen += mPacketVideoHelper->rtmp_buf->len;
		}
		else
		{
			memcpy(loaddata + loaddataLen, rtmp_header27, sizeof(rtmp_header27));
			loaddataLen += sizeof(rtmp_header27);
		}
		size = htonl(payloadLen);
		memcpy(loaddata + loaddataLen, &size, sizeof(uint32_t));
		loaddataLen += sizeof(uint32_t);
		memcpy(loaddata + loaddataLen, payload, payloadLen);
		loaddataLen += payloadLen;
		RtmpBuffer* buffer = new RtmpBuffer(loaddata, loaddataLen);
		if (mPacketVideoHelper->rtmp_buf != NULL){
			delete mPacketVideoHelper->rtmp_buf;
			mPacketVideoHelper->rtmp_buf = NULL;
		}
		mPacketVideoHelper->rtmp_buf = buffer;
		mPacketVideoHelper->timestamp += timestamp;
	}
	break;

	case 5: //IDR
	{
		unsigned char loaddata[MAX_ENCODE_SIZE];
		int loaddataLen = 0;
		if (mPacketVideoHelper->rtmp_buf && mPacketVideoHelper->rtmp_buf->len >0){
			memcpy(loaddata + loaddataLen, mPacketVideoHelper->rtmp_buf->data, mPacketVideoHelper->rtmp_buf->len);
			loaddataLen += mPacketVideoHelper->rtmp_buf->len;
		}
		else
		{
			memcpy(loaddata + loaddataLen, rtmp_header17, sizeof(rtmp_header17));
			loaddataLen += sizeof(rtmp_header17);
		}
		size = htonl(payloadLen);
		memcpy(loaddata + loaddataLen, &size, sizeof(uint32_t));
		loaddataLen += sizeof(uint32_t);
		memcpy(loaddata + loaddataLen, payload, payloadLen);
		loaddataLen += payloadLen;
		RtmpBuffer* buffer = new RtmpBuffer(loaddata, loaddataLen);
		if (mPacketVideoHelper->rtmp_buf != NULL){
			delete mPacketVideoHelper->rtmp_buf;
			mPacketVideoHelper->rtmp_buf = NULL;
		}
		mPacketVideoHelper->rtmp_buf = buffer;
		mPacketVideoHelper->timestamp += timestamp;
	}
	break;
	case 28: // FU-A
	{
		uint8_t *q = payload;
		uint8_t h264_start_bit = q[1] & 0x80;
		uint8_t h264_end_bit   = q[1] & 0x40;
		uint8_t h264_type      = q[1] & 0x1F;
		uint8_t h264_nri       = (q[0] & 0x60) >> 5;
		uint8_t h264_key       = (h264_nri << 5) | h264_type;

		unsigned char fuadata[MAX_ENCODE_SIZE];
		int fuadataLen = 0;
		if (mPacketVideoHelper->fua_buf && mPacketVideoHelper->fua_buf->len > 0)
		{
			memcpy(fuadata + fuadataLen, mPacketVideoHelper->fua_buf->data, mPacketVideoHelper->fua_buf->len);
			fuadataLen += mPacketVideoHelper->fua_buf->len;
		}
		unsigned char rtmpdata[MAX_ENCODE_SIZE];
		int rtmpdataLen = 0;
		if (mPacketVideoHelper->rtmp_buf && mPacketVideoHelper->rtmp_buf->len > 0)
		{
			memcpy(rtmpdata + rtmpdataLen, mPacketVideoHelper->rtmp_buf->data, mPacketVideoHelper->rtmp_buf->len);
			rtmpdataLen += mPacketVideoHelper->rtmp_buf->len;
		}
		if (h264_start_bit){
			memcpy(fuadata + fuadataLen, &h264_key, sizeof(h264_key));
			fuadataLen += sizeof(h264_key);
		}
		memcpy(fuadata + fuadataLen, q + 2, payloadLen - 2);
		fuadataLen += payloadLen - 2;

		if (h264_end_bit){
			//const void * nal_data;
			uint32_t used = fuadataLen;
			uint32_t used_big = htonl(used);

			nalType = fuadata[0] & 0x1f;


			if (mPacketVideoHelper->rtmp_buf == NULL){
				if (nalType == 5){
					memcpy(rtmpdata, rtmp_header17, sizeof(rtmp_header17));
					rtmpdataLen += sizeof(rtmp_header17);
				}
				else{
					memcpy(rtmpdata, rtmp_header27, sizeof(rtmp_header27));
					rtmpdataLen += sizeof(rtmp_header27);
				}
			}
			memcpy(rtmpdata, &used_big, sizeof(uint32_t));
			rtmpdataLen += sizeof(uint32_t);
			if (mPacketVideoHelper->rtmp_buf)
			{
				delete mPacketVideoHelper->rtmp_buf;
				mPacketVideoHelper->rtmp_buf = NULL;
			}
			if (mPacketVideoHelper->fua_buf)
			{
				delete mPacketVideoHelper->fua_buf;
				mPacketVideoHelper->fua_buf = NULL;
			}
			RtmpBuffer* buffer = new RtmpBuffer(rtmpdata, rtmpdataLen);
			mPacketVideoHelper->rtmp_buf = buffer;

		}
	}
	break;
	case 24:{
		// for aggregated SPS and PPSs
		uint8_t *q = payload + 1;
		uint16_t nalu_size = 0;
		int nt = 0;
		int nidx = 0;
		while (nidx < payloadLen - 1){
			// get NALU size
			nalu_size = (q[nidx] << 8) | (q[nidx + 1]);

			nidx += 2;

			if (nalu_size == 0){
				nidx++;
				continue;
			}

			// write NALU data
			nt = q[nidx] & 0x1f;
			switch (nt) {
			case 1:
			{//Non IDR
				unsigned char loaddata[MAX_ENCODE_SIZE];
				int loaddataLen = 0;
				if (mPacketVideoHelper->rtmp_buf && mPacketVideoHelper->rtmp_buf->len > 0){
					memcpy(loaddata + loaddataLen, mPacketVideoHelper->rtmp_buf->data, mPacketVideoHelper->rtmp_buf->len);
					loaddataLen += mPacketVideoHelper->rtmp_buf->len;
				}
				else
				{
					memcpy(loaddata + loaddataLen, rtmp_header27, sizeof(rtmp_header27));
					loaddataLen += sizeof(rtmp_header27);
				}
				size = htonl(nalu_size);
				memcpy(loaddata + loaddataLen, &size, sizeof(uint32_t));
				loaddataLen += sizeof(uint32_t);
				memcpy(loaddata + loaddataLen,  q + nidx, nalu_size);
				loaddataLen += nalu_size;
				RtmpBuffer* buffer = new RtmpBuffer(loaddata, loaddataLen);
				if (mPacketVideoHelper->rtmp_buf != NULL){
					delete mPacketVideoHelper->rtmp_buf;
					mPacketVideoHelper->rtmp_buf = NULL;
				}
				mPacketVideoHelper->rtmp_buf = buffer;

			}
				break;
			case 5:	// IDR
			{
				unsigned char loaddata[MAX_ENCODE_SIZE];
				int loaddataLen = 0;
				if (mPacketVideoHelper->rtmp_buf && mPacketVideoHelper->rtmp_buf->len > 0){
					memcpy(loaddata + loaddataLen, mPacketVideoHelper->rtmp_buf->data, mPacketVideoHelper->rtmp_buf->len);
					loaddataLen += mPacketVideoHelper->rtmp_buf->len;
				}
				else
				{
					memcpy(loaddata + loaddataLen, rtmp_header17, sizeof(rtmp_header17));
					loaddataLen += sizeof(rtmp_header17);
				}
				size = htonl(payloadLen);
				memcpy(loaddata + loaddataLen, &size, sizeof(uint32_t));
				loaddataLen += sizeof(uint32_t);
				memcpy(loaddata + loaddataLen, q + nidx, nalu_size);
				loaddataLen += payloadLen;
				RtmpBuffer* buffer = new RtmpBuffer(loaddata, loaddataLen);
				if (mPacketVideoHelper->rtmp_buf != NULL){
					delete mPacketVideoHelper->rtmp_buf;
					mPacketVideoHelper->rtmp_buf = NULL;
				}
				mPacketVideoHelper->rtmp_buf = buffer;

			}
				break;
			case 7: //sps
			{
				if(mPacketVideoHelper->sps){
					delete mPacketVideoHelper->sps;
					mPacketVideoHelper->sps = NULL;
				}
				RtmpBuffer* buffer = new RtmpBuffer(q + nidx, nalu_size);
				mPacketVideoHelper->sps = buffer;
			}
				break;
			case 8: //pps
			{
				if(mPacketVideoHelper->pps){
					delete mPacketVideoHelper->pps;
					mPacketVideoHelper->pps = NULL;
				}
				RtmpBuffer* buffer = new RtmpBuffer(q + nidx, nalu_size);
				mPacketVideoHelper->pps = buffer;

			}
				break;
			default:

				break;
			}
			nidx += nalu_size;
		}
	}
	break;
	case 6:
		break;
	default:
		break;
	}

	// build the avc seq
	if (mPacketVideoHelper->sps_changed && mPacketVideoHelper->sps != NULL && mPacketVideoHelper->pps != NULL){
		int i = 0;
		uint16_t size;
		uint8_t *sps = mPacketVideoHelper->sps->data;
		unsigned char buf[AMF_MAX_SIZE * 2];

		buf[i++] = 0x17;
		buf[i++] = 0;
		buf[i++] = 0;
		buf[i++] = 0;
		buf[i++] = 0;
		buf[i++] = 1;
		buf[i++] = sps[1];
		buf[i++] = sps[2];
		buf[i++] = sps[3];
		buf[i++] = 0xff;
		buf[i++] = 0xe1;

		// 2 bytes sps size
		size = htons(mPacketVideoHelper->sps->len);
		memcpy(buf + i, &size, 2);
		i += 2;
		//sps data
		memcpy(buf + i, sps, mPacketVideoHelper->sps->len);
		buf[i] = 0x67;
		i += mPacketVideoHelper->sps->len;

		// number of pps
		buf[i++] = 0x01;

		// 2 bytes pps size
		size = htons((mPacketVideoHelper->pps->len));
		memcpy(buf + i, &size, 2);
		i += 2;
		// pps data
		memcpy(buf + i, mPacketVideoHelper->pps->data, mPacketVideoHelper->pps->len);
		buf[i] = 0x68; // set pps header
		i += mPacketVideoHelper->pps->len;

		if(mPacketVideoHelper->avc_conf){
			delete mPacketVideoHelper->avc_conf;
			mPacketVideoHelper->avc_conf = NULL;
		}

		RtmpBuffer* buffer = new RtmpBuffer(buf, i);
		mPacketVideoHelper->avc_conf = buffer;
		mPacketVideoHelper->send_avc = true;
		mPacketVideoHelper->sps_changed = false;
		//mPacketVideoHelper->timestamp = 0;
	}

	if (mPacketVideoHelper->avc_conf){
		mPacketVideoHelper->send = true;
	}
	else
	{
wait_sps:
		if (mPacketVideoHelper->rtmp_buf)
		{
			delete mPacketVideoHelper->rtmp_buf;
			mPacketVideoHelper->rtmp_buf = NULL;
		}
		if (mPacketVideoHelper->fua_buf)
		{
			delete mPacketVideoHelper->fua_buf;
			mPacketVideoHelper->fua_buf = NULL;
		}
		mPacketVideoHelper->send = false;
	}

	FileLog("RtmpClient",
			"RtmpClient::RtmpClient(end"
			"nalType:%d"
			" )",
			nalType
			);
	return result;
}

void RtmpClient::HandleSendH264()
{
	FileLog("RtmpClient",
			"RtmpClient::HandleSendH264(start "
			")"
			);
	static int timestamp = 0;
	static int timestamp2 = 0;
	if(mPacketVideoHelper == NULL || mPacketVideoHelper->rtmp_buf == NULL || mPacketVideoHelper->sps == NULL || mPacketVideoHelper->pps == NULL ){
		FileLog("RtmpClientError",
				"RtmpClient::HandleSendH264( "
				"mPacketVideoHelper is NULL"
				")"
				);
		return;
	}

	if (mPacketVideoHelper->send) {
		uint16_t used = mPacketVideoHelper->rtmp_buf->len;
		const void *rtmp_data = mPacketVideoHelper->rtmp_buf->data;
//		FileLog("RtmpClient",
//				"Jni::HandleSendH264( "
//				"rtmp_buflen:%d,"
//				"spsLen:%d,"
//				"ppsLen:%d,"
//				"send:%d,"
//				"send_avc:%d,"
//				")",
//				 mPacketVideoHelper->rtmp_buf->len,
//				 mPacketVideoHelper->sps->len,
//				 mPacketVideoHelper->pps->len,
//				 mPacketVideoHelper->send,
//				 mPacketVideoHelper->send_avc
//				);

		//switch_buffer_peek_zerocopy(mPacketVideoHelper->rtmp_buf, &rtmp_data);

//		if (!tech_pvt->stream_start_ts) {
//			tech_pvt->stream_start_ts = switch_micro_time_now() / 1000;
//			ts = 0;
//		} else {
//			ts = (switch_micro_time_now() / 1000) - tech_pvt->stream_start_ts;
//		}
//
//#if 0
//		{ /* use timestamp read from the frame */
//			uint32_t timestamp = frame->timestamp & 0xFFFFFFFF;
//			ts = timestamp / 90;
//		}
//#endif
//
//		if (ts == tech_pvt->stream_last_ts) {
//			// switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "dup ts: %" SWITCH_TIME_T_FMT "\n", ts);
//			ts += 1;
//			if (ts == 1) ts = 0;
//		}
//
//		tech_pvt->stream_last_ts = ts;

		if (!rtmp_data) {
			FileLog("RtmpClientError",
					"RtmpClient::HandleSendH264( "
					"rtmp_data is NULL"
					")"
					);
			goto skip;
		}

		if (((uint8_t *)rtmp_data)[0] == 0x17 && mPacketVideoHelper->send_avc) {
			uint8_t *avc_conf = mPacketVideoHelper->avc_conf->data;
			SendVideoData((char*)avc_conf, mPacketVideoHelper->avc_conf->len, mPacketVideoHelper->timestamp);
			//SendVideoData((char*)avc_conf, mPacketVideoHelper->avc_conf->len, 67);
			mPacketVideoHelper->send_avc = false;
		}

		SendVideoData((char*)rtmp_data, mPacketVideoHelper->rtmp_buf->len, mPacketVideoHelper->timestamp);
		//SendVideoData((char*)rtmp_data, mPacketVideoHelper->rtmp_buf->len, 125);

		//int timebg = 0;
		//SendVideoData((char*)rtmp_data, mPacketVideoHelper->rtmp_buf->len, 67);
//		if(state){
//			FileLog("RtmpClient",
//					"RtmpClient::HandleSendH264( "
//					"SendVideoData Fail"
//					")"
//					);
//		}

//		// if dropped_video_frame > N then ask the far end for a new IDR for each N frames
//		if (rsession->dropped_video_frame > 0 && rsession->dropped_video_frame % 90 == 0) {
//			switch_core_session_t *other_session;
//			if (switch_core_session_get_partner(session, &other_session) == SWITCH_STATUS_SUCCESS) {
//				switch_core_session_request_video_refresh(session);
//				switch_core_session_rwunlock(other_session);
//			}
//		}
skip:
		if (mPacketVideoHelper->rtmp_buf)
		{
			delete mPacketVideoHelper->rtmp_buf;
			mPacketVideoHelper->rtmp_buf = NULL;
		}
		if (mPacketVideoHelper->fua_buf)
		{
			delete mPacketVideoHelper->fua_buf;
			mPacketVideoHelper->fua_buf = NULL;
		}
		mPacketVideoHelper->send = false;
	}
	FileLog("RtmpClient",
			"RtmpClient::HandleSendH264(end"
			")"
			);
}

