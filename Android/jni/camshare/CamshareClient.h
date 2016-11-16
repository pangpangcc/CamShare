/*
 * CamshareClient.h
 *
 *  Created on: 2016年10月28日
 *      Author: max
 */

#ifndef CAMSHARE_CAMSHARECLIENT_H_
#define CAMSHARE_CAMSHARECLIENT_H_

#include <string>
using namespace std;

#include <common/KThread.h>

#include <rtmp/RtmpClient.h>
#include "H264Decoder.h"

typedef enum UserType {
	UserType_Man = 0,
	UserType_Lady,
	UserType_Count,
}UserType;

class CamshareClient;
class CamshareClientListener {
public:
	virtual ~CamshareClientListener(){};

	virtual void OnLogin(CamshareClient *client, bool success) = 0;
	virtual void OnMakeCall(CamshareClient *client, bool success) = 0;
	virtual void OnHangup(CamshareClient *client, const string& cause) = 0;
	virtual void OnDisconnect(CamshareClient* client) = 0;
	virtual void OnRecvVideo(CamshareClient* client, const char* data, unsigned int size, unsigned int timestamp) = 0;
	virtual void OnRecvVideoSizeChange(CamshareClient* client, unsigned int width, unsigned int height) = 0;
};

class ClientRunnable;
class CamshareClient : public RtmpClientListener {
	friend class ClientRunnable;

public:
	CamshareClient();
	virtual ~CamshareClient();

	/**
	 * 开始连接并登录流媒体服务器
	 * @param	hostName	服务器地址	example:127.0.0.1
	 * @param	user		用户名		example:CM123456
	 * @param	password	密码			example:123456
	 * @param	site		站点Id		example:1
	 * @param	sid			php登录参数	example:SESSION123456
	 * @param	userType	php登录参数	example:UserType.UserType_Man
	 * @return	成功/失败
	 */
	bool Start(
			const string& hostName,
			const string& user,
			const string& password,
			const string& site,
			const string& sid,
			UserType userType
			);

	/**
	 * 拨号进入会议室
	 * @param	dest		会议室名字	example:P123456
	 * @param	serverId	女士服务器Id	example:PC1
	 * @return	成功/失败
	 */
	bool MakeCall(
			const string& dest,
			const string serverId
			);

	/**
	 *	退出当前会议室
	 *	@return	成功/失败
	 */
	bool Hangup();

	/**
	 * 停止连接或者断开流媒体服务器
	 */
	void Stop();

	/**
	 * 是否正在运行
	 */
	bool IsRunning();

	/**
	 * 设置回调
	 */
	void SetCamshareClientListener(CamshareClientListener *listener);

	/**
	 * 设置录制文件夹
	 */
	void SetRecordFilePath(const string& filePath);

public:
	void OnConnect(RtmpClient* client, const string& sessionId);
	void OnDisconnect(RtmpClient* client);
	void OnLogin(RtmpClient* client, bool success);
	void OnMakeCall(RtmpClient* client, bool success, const string& channelId);
	void OnHangup(RtmpClient* client, const string& channelId, const string& cause);
	void OnCreatePublishStream(RtmpClient* client);
	void OnHeartBeat(RtmpClient* client);
	void OnRecvAudio(RtmpClient* client, const char* data, unsigned int size);
	void OnRecvVideo(RtmpClient* client, const char* data, unsigned int size, unsigned int timestamp);

private:
	RtmpClient mRtmpClient;
	ClientRunnable* mpRunnable;
	KThread mClientThread;
	H264Decoder mH264Decoder;

	CamshareClientListener* mpCamshareClientListener;
	bool mbRunning;

	string hostName;
	string user;
	string password;
	string site;
	string sid;
	int userType;

	string dest;

	unsigned int mFrameWidth;
	unsigned int mFrameHeight;
};

#endif /* CAMSHARE_CAMSHARECLIENT_H_ */
