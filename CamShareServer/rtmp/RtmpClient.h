/*
 * RtmpClient.h
 *
 *  Created on: 2016年4月28日
 *      Author: max
 */

#ifndef RTMP_RTMPCLIENT_H_
#define RTMP_RTMPCLIENT_H_

#include <string.h>

#include <string>
#include <list>
using namespace std;

#include <common/KSafeMap.h>
#include "RtmpPacket.h"

typedef enum RTMP_PACKET_TYPE {
	RTMP_PACKET_TYPE_UNKNOW,
	RTMP_PACKET_TYPE_CONNECTED,
} RTMP_PACKET_TYPE;

typedef enum STAND_INVOKE_TYPE {
	STAND_INVOKE_TYPE_CONNECT,
	STAND_INVOKE_TYPE_PUBLISH,
	STAND_INVOKE_TYPE_PLAY,
	STAND_INVOKE_TYPE_UNKNOW
} STAND_INVOKE_TYPE;
typedef KSafeMap<unsigned int, STAND_INVOKE_TYPE> StandInvokeMap;

typedef list<RtmpPacket*> MediaPacketRecvList;

class RtmpClient;
class ISocketHandler;
/**
 * RtmpClient客户端监听接口类
 */
class RtmpClientListener
{
public:
	RtmpClientListener() {};
	virtual ~RtmpClientListener() {}

public:
	virtual void OnConnect(RtmpClient* client, const string& sessionId) = 0;
	virtual void OnDisconnect(RtmpClient* client) = 0;
	virtual void OnLogin(RtmpClient* client, bool bSuccess) = 0;
	virtual void OnMakeCall(RtmpClient* client, bool bSuccess, const string& channelId) = 0;
	virtual void OnHangup(RtmpClient* client, const string& channelId, const string& cause) = 0;
	virtual void OnCreatePublishStream(RtmpClient* client) = 0;
};

class RtmpClient {
public:
	RtmpClient();
	virtual ~RtmpClient();

	int GetIndex();
	void SetIndex(int i);
	const string& GetUser();
	bool IsConnected();

	void SetRtmpClientListener(RtmpClientListener *listener);
	bool Connect(const string& hostName);
	void Close();
	bool Login(const string& user, const string& password, const string& site, const string& custom);
	bool MakeCall(const string& dest);
	bool CreatePublishStream();
	bool SendVideoData(const char* data, unsigned int len, unsigned int timestamp);

	bool RecvRtmpPacket(RtmpPacket* packet);
	RTMP_PACKET_TYPE ParseRtmpPacket(RtmpPacket* packet);

private:
	void DumpRtmpPacket(RtmpPacket* packet);
	bool SendRtmpPacket(RtmpPacket* packet);

	/**
	 * Stand Invoke Packet
	 * ++miNumberInvokes every time
	 */
	bool SendConnectPacket();
	bool SendCreateStream(STAND_INVOKE_TYPE type);

	/**
	 * Stand Stream Packet
	 */
	bool PublishStream(unsigned int streamId);
	bool SendPublishPacket();

	/**
	 * Custom Invoke Packet
	 */
	bool SendLoginPacket(const string& user, const string& auth, const string& site, const string& custom);
	bool SendMakeCallPacket(const string& dest);

	unsigned int GetTime();

	/**
	 * Do it when TCP connected
	 */
	bool HandShake();

	unsigned short port;
	string hostname;
	string app;

	int mIndex;
	string mUser;
	string mDest;

	bool mbConnected;
	string mSession;
	unsigned int miNumberInvokes;
	unsigned int m_outChunkSize;

	ISocketHandler* m_socketHandler;
	RtmpClientListener* mpRtmpClientListener;

	/**
	 * 已经接收的rtmp媒体包
	 */
	MediaPacketRecvList mMediaPacketRecvList;

	/**
	 * 上次发送包
	 */
	RtmpPacket* mpLastSendPacket;

	/**
	 * 上次接收到的包
	 */
	RtmpPacket* mpLastRecvPacket;

	/**
	 *	Stand Invoke Packet Seq
	 */
	StandInvokeMap mpStandInvokeMap;

	unsigned int mPublishStreamId;
	unsigned int mPlayStreamId;

	unsigned int mTimestamp;
};

#endif /* RTMP_RTMPCLIENT_H_ */
