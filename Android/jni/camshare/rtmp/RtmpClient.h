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

#include <common/KMutex.h>
#include <common/KSafeMap.h>
#include "RtmpPacket.h"

#define	RTMP_CHANNELS	65600

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

struct RTMP2RTP_HELPER_S;

struct RTP2RTMP_HELPER_S;


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
	virtual void OnLogin(RtmpClient* client, bool success) = 0;
	virtual void OnMakeCall(RtmpClient* client, bool success, const string& channelId) = 0;
	virtual void OnHangup(RtmpClient* client, const string& channelId, const string& cause) = 0;
	virtual void OnCreatePublishStream(RtmpClient* client) = 0;
	virtual void OnHeartBeat(RtmpClient* client) = 0;
	virtual void OnRecvAudio(RtmpClient* client, const char* data, unsigned int size) = 0;
	virtual void OnRecvVideo(RtmpClient* client, const char* data, unsigned int size, unsigned int timestamp) = 0;
};

class RtmpClient {
public:
	RtmpClient();
	virtual ~RtmpClient();

	const string& GetUser();
	const string& GetDest();
	bool IsRunning();

	void SetRtmpClientListener(RtmpClientListener *listener);
	bool Connect(const string& hostName);
	void Shutdown();
	void Close();

	bool Login(const string& user, const string& password, const string& site, const string& custom);
	bool MakeCall(const string& dest);
	bool Hangup();
	bool CreatePublishStream();
	bool CreateReceiveStream();

	bool SendVideoData(const char* data, unsigned int len, unsigned int timestamp);
	bool SendHeartBeat();

	bool RecvRtmpPacket(RtmpPacket* packet);
	bool RecvRtmpChunkPacket(RtmpPacket* packet, bool & errorPacket, bool fixVideoPacket = false);
	RTMP_PACKET_TYPE ParseRtmpPacket(RtmpPacket* packet);

	bool IsReadyPacket(RtmpPacket* packet);

	// 组包
	bool RtmpToPacketH264(unsigned char* data, unsigned int len, unsigned int timesp);
	void HandleSendH264();

private:
	void Init();

	void DumpRtmpPacket(RtmpPacket* packet);
	bool SendRtmpPacket(RtmpPacket* packet);

	/**
	 * Standard Invoke Packet
	 * ++miNumberInvokes every time
	 */
	bool SendConnectPacket();
	bool SendCreateStream(STAND_INVOKE_TYPE type);

	/**
	 * Standard Stream Packet
	 */
	bool SendPublishPacket(unsigned int streamId);
	bool SendReceiveVideoPacket(unsigned int streamId);
	bool SendPlayPacket();

	/**
	 * Custom Invoke Packet
	 */
	bool SendLoginPacket(const string& user, const string& auth, const string& site, const string& custom);
	bool SendMakeCallPacket(const string& dest);
	bool SendHangupPacket();
	bool SendActivePacket();

	/**
	 * Standard Acknowledgment Packet
	 */
	bool SendAckPacket();

	/**
	 * GetCurrentTime
	 */
	unsigned int GetTime();

	/**
	 * Do it when TCP connected
	 */
	bool HandShake();

	bool Rtmp2H264(unsigned char* data, unsigned int len);

	unsigned short port;
	string hostName;
	string app;

	string mUser;
	string mDest;

	bool mbRunning;
	bool mbConnected;
	string mSession;
	int miNumberInvokes;

	ISocketHandler* m_socketHandler;
	RtmpClientListener* mpRtmpClientListener;

	/**
	 * 已经接收的rtmp媒体包
	 */
	MediaPacketRecvList mMediaPacketRecvList;

	/**
	 *	Standard Invoke Packet Sequence
	 */
	StandInvokeMap mpStandInvokeMap;

	/**
	 * 状态锁
	 */
	KMutex mClientMutex;

	/**
	 * 收包锁
	 */
	KMutex mRecvMutex;

	/**
	 * 发包锁
	 */
	KMutex mSendMutex;

	/**
	 * 上传流
	 */
	unsigned int mPublishStreamId;
	/**
	 * 下载流
	 */
	unsigned int mReceiveStreamId;

	/**
	 * 发包的timestamp
	 */
	unsigned int mSendTimestamp;

	/**
	 * 发包的chunkSize
	 */
	unsigned int mSendChunkSize;

	/**
	 * 收包的chunkSize
	 */
	unsigned int mRecvChunkSize;

	/**
	 * 收包确认
	 */
	unsigned int mRecv;
	unsigned int mRecvAckSent;
	unsigned int mRecvAckWindow;

	/**
	 * 上次接收到的包
	 */
	RtmpPacket *mChannelsIn[RTMP_CHANNELS];
	RtmpPacket *mChannelsOut[RTMP_CHANNELS];

	/**
	 * 上次收到的视频包
	 */
	RtmpPacket *mpVideoPacket;
	unsigned int mUnknowBytes;

	/**
	 * RTMP视频包转RTP视频包
	 */
	RTMP2RTP_HELPER_S* mReadVideoHelper;

	/**
	 * RTP视频包转RTMP视频包
	 *
	 */
	RTP2RTMP_HELPER_S* mPacketVideoHelper;
};

#endif /* RTMP_RTMPCLIENT_H_ */
